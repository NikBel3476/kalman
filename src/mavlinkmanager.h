#ifndef MAVLINKMANAGER_H
#define MAVLINKMANAGER_H

#include <QMessageBox>
#include <QSerialPort>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <ardupilotmega/mavlink.h>

class MavlinkManager : public QWidget {
	Q_OBJECT
public:
	explicit MavlinkManager(QWidget *, QSerialPort *);
	void sendCmdLong(uint16_t command, uint8_t confirmation, float param1 = 0.0,
									 float param2 = 0.0, float param3 = 0.0, float param4 = 0.0,
									 float param5 = 0.0, float param6 = 0.0, float param7 = 0.0);

private:
	void _handleError(QSerialPort::SerialPortError);
	void _writeData(const QByteArray &);
	void _readData();
	void _handleBytesWritten(qint64);
	void _showWriteError(const QString &);

	QSerialPort *_serial = nullptr;
	QTimer *_serial_write_timer = nullptr;

	uint64_t _bytesToWrite = 0;
};

#endif // MAVLINKMANAGER_H
