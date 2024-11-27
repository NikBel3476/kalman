#ifndef MAVLINKMANAGER_H
#define MAVLINKMANAGER_H

#include <QMessageBox>
#include <QObject>
#include <QSerialPort>
#include <QString>
#include <QTimer>
#include <vector>

#include <ardupilotmega/mavlink.h>

#include "autopilot.hpp"

struct FtpMessage {
#pragma pack(push, 1)
	enum class Opcode : uint8_t {
		None = 0,
		TerminateSession = 1,
		ResetSessions = 2,
		ListDirectory = 3,
		OpenFileRO = 4,
		ReadFile = 5,
		CreateFile = 6,
		WriteFile = 7,
		RemoveFile = 8,
		CreateDirectory = 9,
		RemoveDirectory = 10,
		OpenFileWO = 11,
		TruncateFile = 12,
		Rename = 13,
		CalcFileCRC32 = 14,
		BurstReadFile = 15,
		Ack = 128,
		Nack = 129
	};

	enum class Error : uint8_t {
		None = 0,
		Fail = 1,
		FailErrno = 2,
		InvalidDataSize = 3,
		InvalidSession = 4,
		NoSessionsAvailable = 5,
		EndOfFile = 6,
		UnknownCommand = 7,
		FileExists = 8,
		FileProtected = 9,
		FileNotFound = 10
	};

	uint16_t sequence = 0;
	uint8_t session = 0;
	Opcode opcode = Opcode::None;
	uint8_t size = 0;
	Opcode requestOpcode = Opcode::None;
	uint8_t burstComplete = 0;
	uint8_t padding = 0;
	uint32_t offset = 0;
	uint8_t payload[239] = {0};
};
#pragma pack(pop)

class MavlinkManager : public QObject {
	Q_OBJECT
public:
	explicit MavlinkManager(QObject *, QSerialPort *, const Autopilot *ap);
	void sendCmdLong(uint16_t command, uint8_t confirmation, float param1 = 0.0,
									 float param2 = 0.0, float param3 = 0.0, float param4 = 0.0,
									 float param5 = 0.0, float param6 = 0.0, float param7 = 0.0);
	void sendParamSet(const mavlink_param_value_t &);
	void sendParamRequestList();
	void requestListDirectory(const std::string &path);
	void requestCreateFile(const std::string &file_path);
	void requestCreateDirectory(const std::string &dir_path);
	void requestTerminateSession();
	void requestResetSessions();
	void requestWriteFile(const std::vector<char> &data, uint8_t session,
												uint32_t offset);
	void requestCalcFileCrc32(const std::string &path);

	// TODO: make private, these properties should be changed only by mavlink
	// manager
	uint16_t ftp_message_sequence = 0;
	uint32_t ftp_offset = 0;

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
	void _sendFtpMessage(FtpMessage &message);

	QSerialPort *_serial = nullptr;
	QTimer *_serial_write_timer = nullptr;

	const Autopilot *_autopilot = nullptr;
	uint64_t _bytesToWrite = 0;
	mavlink_message_t _mavlink_message;
	mavlink_status_t _mavlink_status;
};

#endif // MAVLINKMANAGER_H
