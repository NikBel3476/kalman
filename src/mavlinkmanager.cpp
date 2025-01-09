#include "mavlinkmanager.hpp"

static const uint8_t SYSTEM_ID = 255;
static const uint8_t COMP_ID = MAV_COMP_ID_MISSIONPLANNER;
static const uint8_t TARGET_SYSTEM_ID = 1;
static const uint8_t TARGET_COMP_ID = MAV_COMP_ID_AUTOPILOT1;

static constexpr auto kSerialWriteTimeout = std::chrono::seconds{5};

MavlinkManager::MavlinkManager(QObject *parent, QSerialPort *serial,
															 const Autopilot *autopilot)
		: QObject{parent},
			_serial{serial},
			_serial_write_timer(new QTimer(this)),
			_autopilot{autopilot},
			_mavlink_message{},
			_mavlink_status{} {
	// serial port connections
	connect(_serial, &QSerialPort::errorOccurred, this,
					&MavlinkManager::_handleError);
	connect(_serial, &QSerialPort::readyRead, this, &MavlinkManager::_readData);
	connect(_serial, &QSerialPort::bytesWritten, this,
					&MavlinkManager::_handleBytesWritten);
}

void MavlinkManager::sendCmdLong(uint16_t command, uint8_t confirmation,
																 float param1, float param2, float param3,
																 float param4, float param5, float param6,
																 float param7) {
	mavlink_message_t msg;
	mavlink_msg_command_long_pack(SYSTEM_ID, COMP_ID, &msg, TARGET_SYSTEM_ID,
																TARGET_COMP_ID, command, confirmation, param1,
																param2, param3, param4, param5, param6, param7);
	uint8_t buf[44];
	const auto buf_len = mavlink_msg_to_send_buffer(buf, &msg);
	QByteArray data((char *)buf, static_cast<qsizetype>(buf_len));
	_writeData(data);
}

void MavlinkManager::sendParamSet(const mavlink_param_value_t &param) {
	mavlink_message_t msg;
	mavlink_msg_param_set_pack(SYSTEM_ID, COMP_ID, &msg, TARGET_SYSTEM_ID,
														 TARGET_COMP_ID, param.param_id, param.param_value,
														 param.param_type);
	uint8_t buf[35];
	const auto buf_size = mavlink_msg_to_send_buffer(buf, &msg);
	QByteArray data((char *)buf, static_cast<qsizetype>(buf_size));
	_writeData(data);
}

void MavlinkManager::sendParamRequestList() {
	mavlink_message_t msg;
	const auto msg_len = mavlink_msg_param_request_list_pack(
			SYSTEM_ID, COMP_ID, &msg, TARGET_SYSTEM_ID, TARGET_COMP_ID);
	std::vector<uint8_t> buf;
	buf.reserve(msg_len);
	const auto buf_len = mavlink_msg_to_send_buffer(buf.data(), &msg);
	QByteArray data((char *)buf.data(), static_cast<qsizetype>(buf_len));
	_writeData(data);
}

void MavlinkManager::requestListDirectory(const std::string &&path) {
	qDebug() << "REQUEST LIST DIRECTORY: " << path;

	FtpMessage message;
	message.opcode = FtpMessage::Opcode::ListDirectory;
	message.size = path.size();
	message.offset = ftp_offset;
	memcpy(message.payload, path.c_str(), path.length());

	_sendFtpMessage(message);
}

void MavlinkManager::requestCreateFile(const std::string &file_path) {
	qDebug() << "REQUEST CREATE FILE" << file_path.c_str();
	FtpMessage message;
	message.opcode = FtpMessage::Opcode::CreateFile;
	message.size = file_path.length();
	memcpy(message.payload, file_path.c_str(), file_path.length());

	_sendFtpMessage(message);
}

void MavlinkManager::requestCreateDirectory(const std::string &dir_path) {
	qDebug() << "REQUEST CREATE DIRECTORY" << dir_path;
	FtpMessage message;
	message.opcode = FtpMessage::Opcode::CreateDirectory;
	message.size = dir_path.length();
	memcpy(message.payload, dir_path.c_str(), dir_path.length());

	_sendFtpMessage(message);
}

void MavlinkManager::requestTerminateSession() {
	qDebug() << "REQUEST TERMINATE SESSION";
	FtpMessage message;
	message.opcode = FtpMessage::Opcode::TerminateSession;

	_sendFtpMessage(message);
}

void MavlinkManager::requestResetSessions() {
	qDebug() << "REQUEST RESET SESSIONS";
	FtpMessage message;
	message.opcode = FtpMessage::Opcode::ResetSessions;

	_sendFtpMessage(message);
}

void MavlinkManager::requestWriteFile(const std::vector<char> &file_data,
																			uint8_t session, uint32_t offset) {
	qDebug() << "REQUEST WRITE FILE";
	FtpMessage message;
	message.opcode = FtpMessage::Opcode::WriteFile;
	message.session = session;
	message.offset = offset;
	message.size = file_data.size();
	memcpy(message.payload, file_data.data(), file_data.size());
	qDebug() << "WRITE FILE OFFSET: " << message.offset
					 << "\nPAYLOAD: " << message.payload
					 << "\nPAYLOAD SIZE: " << file_data.size();

	_sendFtpMessage(message);
}

void MavlinkManager::requestCalcFileCrc32(const std::string &path) {
	qDebug() << "REQUEST CALC CRC32";
	FtpMessage message;
	message.opcode = FtpMessage::Opcode::CalcFileCRC32;
	message.size = path.length();
	memcpy(message.payload, path.c_str(), path.length());

	_sendFtpMessage(message);
}

void MavlinkManager::requestRemoveFile(const std::string &path) {
	qDebug() << "REQUEST REMOVE FILE: " << path;
	FtpMessage message;
	message.opcode = FtpMessage::Opcode::RemoveFile;
	message.size = path.length();
	memcpy(message.payload, path.c_str(), path.length());

	_sendFtpMessage(message);
}

void MavlinkManager::_handleError(QSerialPort::SerialPortError error) {
	// unused variable warning suppress
	(void)(error);
	// switch (error) {
	// case QSerialPort::ResourceError: {
	// 	qDebug() << _serial->errorString() << '\n';
	// 	// QMessageBox::critical(this, tr("Critical Error"),
	// 	// _serial->errorString());
	// 	// FIXME: This error happens sometimes and not breaking connection so we
	// do
	// 	// not close the port, the reason is not figured out closeSerialPort();
	// } break;
	// case QSerialPort::PermissionError: {
	// 	QMessageBox::warning(this, tr("Warning"),
	// 											 tr("No device permissions or it is already in use"));
	// } break;
	// default: {
	// 	// sometimes error emits with `No error` message
	// 	if (_serial->errorString() != "No error") {
	// 		QMessageBox::critical(this, tr("Error"), _serial->errorString());
	// 	}
	// }
	// }
}

void MavlinkManager::_writeData(const QByteArray &data) {
	const qint64 written = _serial->write(data);
	if (written == data.size()) {
		_bytesToWrite += written;
		_serial_write_timer->start(kSerialWriteTimeout);
	} else {
		const auto error_msg =
				tr("Failed to write all data to port %1.\nError: %2")
						.arg(_serial->portName(), _serial->errorString());
		// _showWriteError(error_msg);
		emit serialWriteErrorOccured(error_msg);
	}
	_serial->flush();
}

void MavlinkManager::_readData() {
	if (_autopilot->getState() == AutopilotState::Flashing) {
		return;
	}
	const auto data = _serial->readAll();
	for (const auto byte : data) {
		if (mavlink_parse_char(MAVLINK_COMM_0, byte, &_mavlink_message,
													 &_mavlink_status)) {
			emit mavlinkMessageReceived(_mavlink_message);
		}
	}
}

void MavlinkManager::_handleBytesWritten(qint64 bytes) {
	_bytesToWrite -= bytes;
	if (_bytesToWrite == 0) {
		_serial_write_timer->stop();
	}
}

// void MavlinkManager::_showWriteError(const QString &message) {
// 	QMessageBox::warning(this, tr("Warning"), message);
// }

void MavlinkManager::_sendFtpMessage(FtpMessage &message) {
	ftp_message_sequence = (ftp_message_sequence + 1) % 0xFFFF;
	message.sequence = ftp_message_sequence;

	mavlink_file_transfer_protocol_t ftpMessage;
	ftpMessage.target_network = 0;
	ftpMessage.target_system = TARGET_SYSTEM_ID;
	ftpMessage.target_component = TARGET_COMP_ID;
	memcpy(ftpMessage.payload, &message, sizeof(FtpMessage));

	mavlink_message_t mavlink_message;
	const auto msg_len = mavlink_msg_file_transfer_protocol_encode(
			SYSTEM_ID, COMP_ID, &mavlink_message, &ftpMessage);

	std::vector<uint8_t> buf;
	buf.reserve(msg_len);
	const auto buf_len = mavlink_msg_to_send_buffer(buf.data(), &mavlink_message);
	QByteArray data((char *)buf.data(), static_cast<qsizetype>(buf_len));
	_writeData(data);
}
