#include "mavlinkmanager.h"

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
	mavlink_msg_param_request_list_pack(SYSTEM_ID, COMP_ID, &msg,
																			TARGET_SYSTEM_ID, TARGET_COMP_ID);
	uint8_t buf[14];
	const auto buf_len = mavlink_msg_to_send_buffer(buf, &msg);
	QByteArray data((char *)buf, static_cast<qsizetype>(buf_len));
	_writeData(data);
}

void MavlinkManager::_handleError(QSerialPort::SerialPortError error) {
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
	if (_autopilot->state == AutopilotState::Flashing) {
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
