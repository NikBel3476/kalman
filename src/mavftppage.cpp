#include "mavftppage.h"

const int MIN_PAGE_WIDTH = 150;
const int MAX_PAGE_WIDTH = 250;
static constexpr auto lua_scripts_ftp_directory = "/APM/scripts/";
static constexpr auto FTP_PAYLOAD_SIZE = 80;

MavftpPage::MavftpPage(QWidget *parent, MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_upload_lua_button(new QPushButton()),
			_mavlink_manager{mavlink_manager} {
	_layout->setAlignment(Qt::AlignCenter);
	_layout->addWidget(_upload_lua_button);

	_upload_lua_button->setText(tr("Upload lua scripts"));
	_upload_lua_button->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_upload_lua_button->setMinimumWidth(MIN_PAGE_WIDTH);
	_upload_lua_button->setMaximumWidth(MAX_PAGE_WIDTH);

	// connections
	connect(_upload_lua_button, &QPushButton::clicked, this,
					&MavftpPage::_handleUploadLuaButtonClick);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&MavftpPage::_handleMavlinkMessageReceive);
}

void MavftpPage::_handleUploadLuaButtonClick() {
	_files_to_upload.clear();
	_uploading_file.first.clear();
	_uploading_file.second.clear();
	_file_to_crc_check.first.clear();
	_file_to_crc_check.second.clear();
	const auto file_path_list =
			QFileDialog::getOpenFileNames(this, tr("Select lua scripts"), "", "*.lua")
					.toVector();
	if (file_path_list.empty()) {
		return;
	}
	for (const auto &file_name : file_path_list) {
		_files_to_upload[file_name.mid(file_name.lastIndexOf('/') + 1)] = file_name;
	}

	current_ap_ftp_path = "/APM";
	_mavlink_manager->requestListDirectory(current_ap_ftp_path);
}

void MavftpPage::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL: {
		qDebug() << "FTP MESSAGE RECEIVED";
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

void MavftpPage::_handleFtpMessage(const FtpMessage &message) {
	qDebug() << "PAYLOAD: " << std::string((char *)message.payload, 239)
					 << "\nOPCODE: " << static_cast<uint8_t>(message.opcode) << "\nOFFSET"
					 << message.offset << "\nSIZE" << message.size;

	switch (message.opcode) {
	case FtpMessage::Opcode::Ack: {
		qDebug() << "FTP ACK RECEIVED";
		_handleFtpAck(message);
	} break;
	case FtpMessage::Opcode::Nack: {
		qDebug() << "FTP NACK RECEIVED";
		_handleFtpNack(message);
	} break;
	}
}

void MavftpPage::_handleFtpAck(const FtpMessage &message) {
	switch (message.requestOpcode) {
	case FtpMessage::Opcode::ListDirectory: {
		qDebug() << "LIST DIRECTORY ACK";
		const auto payload_str = std::string((char *)message.payload, 239);
		for (const auto &str : QString::fromStdString(payload_str).split('\0')) {
			if (str.startsWith('D') || str.startsWith('F') || str.startsWith('S')) {
				_fs_item_list.push_back(str);
				qDebug() << str;
			}
		}

		_mavlink_manager->ftp_offset += _fs_item_list.size();
		_mavlink_manager->requestListDirectory(current_ap_ftp_path);
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
			std::ifstream file(_files_to_upload[_uploading_file.first].toStdString());
			if (!file.is_open()) {
				_mavlink_manager->requestResetSessions();
				return;
			}
			qDebug() << "UPLOADING FILE: " << _uploading_file.first;
			std::string file_content((std::istreambuf_iterator<char>(file)),
															 std::istreambuf_iterator<char>());
			std::vector<std::string> file_chunks;
			_uploading_file.second = file_content;
			const auto chunk = _uploading_file.second.substr(
					_uploading_chunk_index * FTP_PAYLOAD_SIZE, FTP_PAYLOAD_SIZE);
			const std::vector data(chunk.begin(), chunk.end());
			qDebug() << "DATA: " << data;
			_mavlink_manager->requestWriteFile(data, _file_upload_session,
																				 _uploading_chunk_index *
																						 FTP_PAYLOAD_SIZE);
			_uploading_chunk_index++;
			return;
		}
	} break;
	case FtpMessage::Opcode::WriteFile: {
		qDebug() << "WRITE FILE ACK";
		if (_uploading_chunk_index * FTP_PAYLOAD_SIZE <
				_uploading_file.second.size()) {
			const auto chunk = _uploading_file.second.substr(
					_uploading_chunk_index * FTP_PAYLOAD_SIZE, FTP_PAYLOAD_SIZE);
			const std::vector data(chunk.begin(), chunk.end());
			_mavlink_manager->requestWriteFile(data, _file_upload_session,
																				 _uploading_chunk_index *
																						 FTP_PAYLOAD_SIZE);
			_uploading_chunk_index++;
		} else {
			qDebug() << "FILE " << _uploading_file.first << " UPLOADED";
			_uploading_chunk_index = 0;
			_files_to_upload.erase(_uploading_file.first);
			_file_to_crc_check = _uploading_file;
			_uploading_file.first.clear();
			_uploading_file.second.clear();
			_mavlink_manager->requestResetSessions();
		}
	} break;
	case FtpMessage::Opcode::ResetSessions: {
		qDebug() << "SESSION RESET ACK";
		qDebug() << "FILES TO UPLOAD SIZE: " << _files_to_upload.size();
		if (!_file_to_crc_check.first.isEmpty() &&
				!_file_to_crc_check.second.empty()) {
			const auto ap_file_path =
					lua_scripts_ftp_directory + _file_to_crc_check.first;
			_mavlink_manager->requestCalcFileCrc32(ap_file_path.toStdString());
			return;
		}
		if (!_files_to_upload.empty()) {
			_uploading_file.first = (*_files_to_upload.begin()).first;
			qDebug() << "UPLOADING FILE: " << _uploading_file.first;
			_mavlink_manager->requestCreateFile(
					(lua_scripts_ftp_directory + _uploading_file.first).toStdString());
			return;
		}
		if (_files_upload_success) {
			_files_upload_success = false;
			QMessageBox::information(this, tr("Information"), tr("Files uploaded"));
		}
	} break;
	case FtpMessage::Opcode::CalcFileCRC32: {
		QByteArray crc((char *)message.payload, message.size);
		qDebug() << "CRC: " << crc;
		const auto expected_crc = Crc::crc(_file_to_crc_check.second.length(),
																			 QByteArray(_file_to_crc_check.second));
		qDebug() << "EXPECTED CRC: " << expected_crc;
		_file_to_crc_check.first.clear();
		_file_to_crc_check.second.clear();
		_mavlink_manager->requestResetSessions();
		if (crc != expected_crc) {
			_files_to_upload.clear();
			QMessageBox::warning(
					this, tr("Warning"),
					tr("Verification %1 failed").arg(_file_to_crc_check.first));
			return;
		}
		if (_files_to_upload.empty()) {
			_files_upload_success = true;
		}
	} break;
	}
}

void MavftpPage::_handleFtpNack(const FtpMessage &message) {
	switch (message.requestOpcode) {
	case FtpMessage::Opcode::ListDirectory: {
		qDebug() << "LIST DIRECTORY NACK";
		_mavlink_manager->ftp_offset = 0;
		if (std::find(_fs_item_list.cbegin(), _fs_item_list.cend(), "Dscripts") ==
				_fs_item_list.cend()) {
			_mavlink_manager->requestCreateDirectory(lua_scripts_ftp_directory);
			return;
		}
		if (!_files_to_upload.empty()) {
			_mavlink_manager->requestResetSessions();
		}
	} break;
	case FtpMessage::Opcode::CreateDirectory: {
		qDebug() << "CREATE DIRECTORY NACK";
		QMessageBox::warning(this, tr("Warning"),
												 tr("Failed to create scripts directory"));
	} break;
	case FtpMessage::Opcode::CreateFile: {
		qDebug() << "CREATE FILE NACK";
		_mavlink_manager->requestTerminateSession();
		const auto file_name = _uploading_file.first;
		_files_to_upload.clear();
		QMessageBox::warning(this, tr("Warning"),
												 tr("Failed to create %1 file").arg(file_name));
	} break;
	}
}
