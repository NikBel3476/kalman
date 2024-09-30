#include "mavlinkmanager.h"

static const uint8_t SYSTEM_ID = 255;
static const uint8_t COMP_ID = MAV_COMP_ID_MISSIONPLANNER;
static const uint8_t TARGET_SYSTEM_ID = 1;
static const uint8_t TARGET_COMP_ID = MAV_COMP_ID_AUTOPILOT1;

static constexpr auto kSerialWriteTimeout = std::chrono::seconds{5};

MavlinkManager::MavlinkManager(QWidget *parent, QSerialPort *serial)
		: QWidget{parent} {
	_serial = serial;
}

void MavlinkManager::_writeData(const QByteArray &data) {
	const qint64 written = _serial->write(data);
	if (written == data.size()) {
		_bytesToWrite += written;
		_serial_timer->start(kSerialWriteTimeout);
	} else {
		const auto error = tr("Failed to write all data to port %1.\n"
													"Error: %2")
													 .arg(_serial->portName(), _serial->errorString());
		_showWriteError(error);
	}
}

void MavlinkManager::_showWriteError(const QString &message) {
	QMessageBox::warning(this, tr("Warning"), message);
}
