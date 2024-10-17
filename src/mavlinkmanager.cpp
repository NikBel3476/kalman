#include "mavlinkmanager.h"

static const uint8_t SYSTEM_ID = 255;
static const uint8_t COMP_ID = MAV_COMP_ID_MISSIONPLANNER;
static const uint8_t TARGET_SYSTEM_ID = 1;
static const uint8_t TARGET_COMP_ID = MAV_COMP_ID_AUTOPILOT1;

static constexpr auto kSerialWriteTimeout = std::chrono::seconds{5};

MavlinkManager::MavlinkManager(QWidget *parent, QSerialPort *serial)
		: QWidget{parent}, _serial{serial}, _serial_write_timer(new QTimer(this)) {

	// serial port connections
	connect(_serial, &QSerialPort::errorOccurred, this,
					&MavlinkManager::_handleError);
	connect(_serial, &QSerialPort::readyRead, this, &MavlinkManager::_readData);
	connect(_serial, &QSerialPort::bytesWritten, this,
					&MavlinkManager::_handleBytesWritten);
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
		const auto error = tr("Failed to write all data to port %1.\n"
													"Error: %2")
													 .arg(_serial->portName(), _serial->errorString());
		_showWriteError(error);
	}
}

void MavlinkManager::_readData() {}

void MavlinkManager::_handleBytesWritten(qint64 bytes) {
	_bytesToWrite -= bytes;
	if (_bytesToWrite == 0)
		_serial_write_timer->stop();
}

void MavlinkManager::_showWriteError(const QString &message) {
	QMessageBox::warning(this, tr("Warning"), message);
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
