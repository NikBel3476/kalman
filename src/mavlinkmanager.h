#ifndef MAVLINKMANAGER_H
#define MAVLINKMANAGER_H

#include <QMessageBox>
#include <QObject>
#include <QSerialPort>
#include <QString>
#include <QTimer>

#include <ardupilotmega/mavlink.h>

#include "autopilot.h"

class MavlinkManager : public QObject {
	Q_OBJECT
public:
	explicit MavlinkManager(QObject *, QSerialPort *, const Autopilot *ap);
	void sendCmdLong(uint16_t command, uint8_t confirmation, float param1 = 0.0,
									 float param2 = 0.0, float param3 = 0.0, float param4 = 0.0,
									 float param5 = 0.0, float param6 = 0.0, float param7 = 0.0);
	void sendParamSet(const mavlink_param_value_t &);
	void sendParamRequestList();

signals:
	void mavlinkMessageReceived(mavlink_message_t &);
	void serialWriteErrorOccured(const QString &error_msg);

public slots:
	// void handleApStateUpdate(const AutopilotState&);

private:
	void _handleError(QSerialPort::SerialPortError);
	void _writeData(const QByteArray &);
	void _readData();
	void _handleBytesWritten(qint64);
	// void _showWriteError(const QString &);

	QSerialPort *_serial = nullptr;
	QTimer *_serial_write_timer = nullptr;

	const Autopilot *_autopilot = nullptr;
	uint64_t _bytesToWrite = 0;
	mavlink_message_t _mavlink_message;
	mavlink_status_t _mavlink_status;
};

#endif // MAVLINKMANAGER_H
