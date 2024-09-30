#ifndef MAVLINKMANAGER_H
#define MAVLINKMANAGER_H

#include <QMessageBox>
#include <QSerialPort>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "mavlink/ardupilotmega/mavlink.h"

class MavlinkManager : public QWidget {
	Q_OBJECT
public:
	explicit MavlinkManager(QWidget *, QSerialPort *);
	void sendCmdLong(uint16_t);

private:
	void _writeData(const QByteArray &);
	void _showWriteError(const QString &);

	QSerialPort *_serial = nullptr;
	QTimer *_serial_timer = nullptr;

	uint64_t _bytesToWrite = 0;
};

#endif // MAVLINKMANAGER_H
