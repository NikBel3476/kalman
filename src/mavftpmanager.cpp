#include "mavftpmanager.hpp"

static constexpr auto APM_FTP_PATH = "/APM";
static constexpr auto LUA_SCRIPTS_FTP_PATH = "/APM/scripts/";
static constexpr auto FTP_PAYLOAD_SIZE = 80;
static constexpr auto FTP_MESSAGE_SIZE = 239;
static constexpr auto kFtpRemoveFileTimeout = std::chrono::milliseconds{1000};
static constexpr auto kFtpWriteFileTimeout = std::chrono::milliseconds{1000};

MavFtpManager::MavFtpManager(QWidget *parent, MavlinkManager *mavlink_manager,
											 Autopilot *autopilot)
		: QObject{parent},
			_mavlink_manager{mavlink_manager},
			_autopilot{autopilot},
			_ftp_remove_file_timer{std::make_unique<QTimer>()},
			_ftp_write_file_timer(std::make_unique<QTimer>()) {
	_ftp_remove_file_timer->setSingleShot(true);
	_ftp_write_file_timer->setSingleShot(true);

	// connections
	connect(_ftp_remove_file_timer.get(), &QTimer::timeout, this,
					&MavFtpManager::_handleFtpRemoveFileTimeout);
	connect(_ftp_write_file_timer.get(), &QTimer::timeout, this,
					&MavFtpManager::_handleFtpWriteFileTimeout);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&MavFtpManager::_handleMavlinkMessageReceive);

	// autopilot connections
	connect(_autopilot, &Autopilot::stateUpdated, this,
					&MavFtpManager::_handleAutopilotStateUpdate);
}

void MavFtpManager::uploadFiles(const std::unordered_map<QString, QByteArray> &files_to_upload) {
	_resetState();
	_files_to_upload = files_to_upload;

	current_ap_ftp_path = APM_FTP_PATH;
	_mavlink_manager->requestListDirectory(std::move(current_ap_ftp_path));
}

void MavFtpManager::_handleAutopilotStateUpdate(const AutopilotState &state) {
	switch (state) {
	case AutopilotState::None: {
		_resetState();
	} break;
	case AutopilotState::Alive:
	case AutopilotState::Flashing:
		break;
	}
}

void MavFtpManager::_handleFtpRemoveFileTimeout() {
	_mavlink_manager->requestResetSessions();
	_resetState();
	emit ftpUploadCompleted(FtpUploadResult::RemoveFilesError);
}

void MavFtpManager::_handleFtpWriteFileTimeout() {
	_mavlink_manager->requestResetSessions();
	const auto file_name = _uploading_file.first;
	_resetState();
	emit ftpUploadCompleted(FtpUploadResult::WriteFileError);
}

void MavFtpManager::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL: {
		// qDebug() << "FTP MESSAGE RECEIVED";
		mavlink_file_transfer_protocol_t ftp_message;
		mavlink_msg_file_transfer_protocol_decode(&mavlink_message, &ftp_message);

		FtpMessage message;
		memcpy(&message, ftp_message.payload, sizeof(FtpMessage));

		if (message.sequence != 0 &&
				message.sequence <= _mavlink_manager->ftp_message_sequence) {
			return;
		};
		_mavlink_manager->ftp_message_sequence = message.sequence;

		_handleFtpMessage(message);
	} break;
	}
}

void MavFtpManager::_handleFtpMessage(const FtpMessage &message) {
	// qDebug() << "PAYLOAD: "
	// 				 << std::string((char *)message.payload, FTP_MESSAGE_SIZE)
	// 				 << "\nOPCODE: " << static_cast<uint8_t>(message.opcode) << "\nOFFSET"
	// 				 << message.offset << "\nSIZE" << message.size;

	switch (message.opcode) {
	case FtpMessage::Opcode::Ack: {
		// qDebug() << "FTP ACK RECEIVED";
		_handleFtpAck(message);
	} break;
	case FtpMessage::Opcode::Nack: {
		// qDebug() << "FTP NACK RECEIVED";
		_handleFtpNack(message);
	} break;
	case FtpMessage::Opcode::BurstReadFile:
	case FtpMessage::Opcode::ReadFile:
	case FtpMessage::Opcode::CreateFile:
	case FtpMessage::Opcode::WriteFile:
	case FtpMessage::Opcode::RemoveFile:
	case FtpMessage::Opcode::TruncateFile:
	case FtpMessage::Opcode::OpenFileRO:
	case FtpMessage::Opcode::OpenFileWO:
	case FtpMessage::Opcode::CalcFileCRC32:
	case FtpMessage::Opcode::ListDirectory:
	case FtpMessage::Opcode::CreateDirectory:
	case FtpMessage::Opcode::RemoveDirectory:
	case FtpMessage::Opcode::Rename:
	case FtpMessage::Opcode::ResetSessions:
	case FtpMessage::Opcode::TerminateSession:
	case FtpMessage::Opcode::None:
		break;
	}
}

void MavFtpManager::_handleFtpAck(const FtpMessage &message) {
	switch (message.requestOpcode) {
	case FtpMessage::Opcode::ListDirectory: {
		qDebug() << "LIST DIRECTORY ACK";
		const auto payload_str = std::string((char *)message.payload, FTP_MESSAGE_SIZE);

		for (const auto &str :
				 QString::fromStdString(payload_str).split(QChar('\0'))) {
			if (current_ap_ftp_path == APM_FTP_PATH) {
				if (str.startsWith('D') || str.startsWith('F') || str.startsWith('S')) {
					_fs_item_list.push_back(str);
					qDebug() << str;
				}
			} else if (current_ap_ftp_path == LUA_SCRIPTS_FTP_PATH) {
				if (str.startsWith('F')) {
					_fs_item_list.push_back(str);
					qDebug() << str;
				}
			}
		}

		_mavlink_manager->ftp_offset += _fs_item_list.size();
		_mavlink_manager->requestListDirectory(std::move(current_ap_ftp_path));
	} break;
	case FtpMessage::Opcode::CreateDirectory: {
		qDebug() << "CREATE DIRECTORY ACK";
		if (!_files_to_upload.empty()) {
			_mavlink_manager->requestResetSessions();
			return;
		}
	} break;
	case FtpMessage::Opcode::CreateFile: {
		qDebug() << "CREATE FILE ACK";
		_file_upload_session = message.session;
		if (!_files_to_upload.empty()) {
			qDebug() << "UPLOADING FILE: " << _uploading_file.first;
			const auto chunk = _uploading_file.second.substr(
					_uploading_chunk_index * FTP_PAYLOAD_SIZE, FTP_PAYLOAD_SIZE);
			const std::vector data(chunk.begin(), chunk.end());
			// qDebug() << "DATA: " << data;
			_mavlink_manager->requestWriteFile(data, _file_upload_session,
																				 _uploading_chunk_index *
																						 FTP_PAYLOAD_SIZE);
			_ftp_write_file_timer->start(kFtpWriteFileTimeout);
			_uploading_chunk_index++;
			return;
		}
	} break;
	case FtpMessage::Opcode::WriteFile: {
		// qDebug() << "WRITE FILE ACK";
		if (!_uploading_file.first.isEmpty() && !_uploading_file.second.empty() &&
				_uploading_chunk_index * FTP_PAYLOAD_SIZE <
						_uploading_file.second.size()) {
			const auto chunk = _uploading_file.second.substr(
					_uploading_chunk_index * FTP_PAYLOAD_SIZE, FTP_PAYLOAD_SIZE);
			const std::vector data(chunk.begin(), chunk.end());
			_mavlink_manager->requestWriteFile(data, _file_upload_session,
																				 _uploading_chunk_index *
																						 FTP_PAYLOAD_SIZE);
			_ftp_write_file_timer->start(kFtpWriteFileTimeout);
			_uploading_chunk_index++;
		} else {
			qDebug() << "FILE " << _uploading_file.first << " UPLOADED";
			_ftp_write_file_timer->stop();
			_uploading_chunk_index = 0;
			_files_to_upload.erase(_uploading_file.first);
			_file_to_crc_check = _uploading_file;
			_uploading_file.first.clear();
			_uploading_file.second.clear();
			emit ftpFileUploaded(_files_to_upload.size());
			_mavlink_manager->requestResetSessions();
		}
	} break;
	case FtpMessage::Opcode::ResetSessions: {
		qDebug() << "SESSION RESET ACK";
		qDebug() << "FILES TO UPLOAD SIZE: " << _files_to_upload.size();
		if (!_file_to_crc_check.first.isEmpty() &&
				!_file_to_crc_check.second.empty()) {
			const auto ap_file_path = LUA_SCRIPTS_FTP_PATH + _file_to_crc_check.first;
			_mavlink_manager->requestCalcFileCrc32(ap_file_path.toStdString());
			return;
		}
		if (!_files_to_upload.empty()) {
			_uploading_file = (*_files_to_upload.begin());
			qDebug() << "UPLOADING FILE: " << _uploading_file.first;
			_mavlink_manager->requestCreateFile(
					(LUA_SCRIPTS_FTP_PATH + _uploading_file.first).toStdString());
			return;
		}
		if (_files_upload_success) {
			_resetState();
			emit ftpUploadCompleted(FtpUploadResult::Ok);
		}
	} break;
	case FtpMessage::Opcode::CalcFileCRC32: {
		QByteArray crc((char *)message.payload, message.size);
		qDebug() << "CRC: " << crc;
		const auto expected_crc =
				Crc::crc(_file_to_crc_check.second.length(),
								 QByteArray(_file_to_crc_check.second.c_str(),
														_file_to_crc_check.second.length()));
		qDebug() << "EXPECTED CRC: " << expected_crc;
		_file_to_crc_check.first.clear();
		_file_to_crc_check.second.clear();
		_mavlink_manager->requestResetSessions();
		if (crc != expected_crc) {
			_resetState();
			emit ftpUploadCompleted(FtpUploadResult::CrcVerificationError);
			return;
		}
		if (_files_to_upload.empty()) {
			_files_upload_success = true;
		}
	} break;
	case FtpMessage::Opcode::Ack:
	case FtpMessage::Opcode::Nack:
	case FtpMessage::Opcode::BurstReadFile:
	case FtpMessage::Opcode::ReadFile:
		break;
	case FtpMessage::Opcode::RemoveFile: {
		qDebug() << "REMOVE FILE ACK";
		if (!_fs_item_list.empty()) {
			const auto filename_to_remove =
					_fs_item_list.back().removeFirst().toStdString();
			_fs_item_list.pop_back();
			_mavlink_manager->requestRemoveFile(LUA_SCRIPTS_FTP_PATH +
																					filename_to_remove);
			_ftp_remove_file_timer->start(kFtpRemoveFileTimeout);
			return;
		}
		_ftp_remove_file_timer->stop();
		if (!_files_to_upload.empty()) {
			_mavlink_manager->requestResetSessions();
			return;
		}
	} break;
	case FtpMessage::Opcode::TruncateFile:
	case FtpMessage::Opcode::OpenFileRO:
	case FtpMessage::Opcode::OpenFileWO:
	case FtpMessage::Opcode::RemoveDirectory:
	case FtpMessage::Opcode::Rename:
	case FtpMessage::Opcode::TerminateSession:
	case FtpMessage::Opcode::None:
		break;
	}
}

void MavFtpManager::_handleFtpNack(const FtpMessage &message) {
	switch (message.requestOpcode) {
	case FtpMessage::Opcode::ListDirectory: {
		qDebug() << "LIST DIRECTORY NACK";
		_mavlink_manager->ftp_offset = 0;

		if (current_ap_ftp_path == APM_FTP_PATH) {
			if (std::find(_fs_item_list.cbegin(), _fs_item_list.cend(), "Dscripts") ==
					_fs_item_list.cend()) {
				_mavlink_manager->requestCreateDirectory(LUA_SCRIPTS_FTP_PATH);
				_fs_item_list.clear();
				return;
			} else {
				_fs_item_list.clear();
				current_ap_ftp_path = LUA_SCRIPTS_FTP_PATH;
				_mavlink_manager->requestListDirectory(std::move(current_ap_ftp_path));
				return;
			}
			if (!_files_to_upload.empty()) {
				_mavlink_manager->requestResetSessions();
				return;
			}
		} else if (current_ap_ftp_path == LUA_SCRIPTS_FTP_PATH) {
			if (!_fs_item_list.empty()) {
				const auto filename_to_remove =
						_fs_item_list.back().removeFirst().toStdString();
				_fs_item_list.pop_back();
				_mavlink_manager->requestRemoveFile(LUA_SCRIPTS_FTP_PATH +
																						filename_to_remove);
				_ftp_write_file_timer->start(kFtpWriteFileTimeout);
				return;
			}
			if (!_files_to_upload.empty()) {
				_mavlink_manager->requestResetSessions();
				return;
			}
		}
	} break;
	case FtpMessage::Opcode::CreateDirectory: {
		qDebug() << "CREATE DIRECTORY NACK";
		emit ftpUploadCompleted(FtpUploadResult::CreateDirectoryError);
		_resetState();
	} break;
	case FtpMessage::Opcode::CreateFile: {
		qDebug() << "CREATE FILE NACK";
		_mavlink_manager->requestTerminateSession();
		const auto file_name = _uploading_file.first;
		_files_to_upload.clear();
		emit ftpUploadCompleted(FtpUploadResult::CreateFileError);
		_resetState();
	} break;
	case FtpMessage::Opcode::Ack:
	case FtpMessage::Opcode::Nack:
	case FtpMessage::Opcode::BurstReadFile:
	case FtpMessage::Opcode::ReadFile:
		break;
	case FtpMessage::Opcode::WriteFile: {
		qDebug() << "WRITE FILE NACK";
		const auto file_name = _uploading_file.first;
		emit ftpUploadCompleted(FtpUploadResult::WriteFileError);
		_resetState();
	} break;
	case FtpMessage::Opcode::RemoveFile: {
		qDebug() << "REMOVE FILE NACK";
		emit ftpUploadCompleted(FtpUploadResult::RemoveFilesError);
		_resetState();
	} break;
	case FtpMessage::Opcode::TruncateFile:
	case FtpMessage::Opcode::OpenFileRO:
	case FtpMessage::Opcode::OpenFileWO:
	case FtpMessage::Opcode::CalcFileCRC32:
	case FtpMessage::Opcode::RemoveDirectory:
	case FtpMessage::Opcode::Rename:
	case FtpMessage::Opcode::ResetSessions:
	case FtpMessage::Opcode::TerminateSession:
	case FtpMessage::Opcode::None:
		break;
	}
}

void MavFtpManager::_resetState() {
	_ftp_remove_file_timer->stop();
	_ftp_write_file_timer->stop();
	_uploading_chunk_index = 0;
	_fs_item_list.clear();
	_files_to_upload.clear();
	_uploading_file.first.clear();
	_uploading_file.second.clear();
	_file_to_crc_check.first.clear();
	_file_to_crc_check.second.clear();
}
