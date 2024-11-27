#ifndef FIRMWAREUPLOADER_H
#define FIRMWAREUPLOADER_H

#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QRegularExpression>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>

#include "crc.hpp"
#include "mavlinkmanager.hpp"

static constexpr uint8_t PROG_MULTI_MAX = 252;

enum class FindBootloaderResult {
	Ok,
	SerialPortError,
	UnsupportedBootloader,
	SyncFail
};

enum class FirmwareUploadResult {
	Ok,
	UnsupportedBoard,
	UnsupportedBootloader,
	FirmwareImageNotFound,
	FirmwareSizeNotFound,
	BoardIdNotFound,
	TooLargeFirmware,
	EraseFail,
	IncompatibleBoardType,
	ProgramFail,
	DecodeFail,
	VerificationFail,
	BootloaderNotFound
};

enum class FirmwareUploadState {
	None,
	Rebooting,
	BootloaderSearching,
	Erasing,
	Flashing
};

enum class SyncResult {
	Ok,
	ReadTimeout,
	Fail,
	InvalidOperation,
	UnexpectedResponse
};

enum class TrySyncResult {
	Ok,
	BadSiliconRev,
	NotInSync,
	NotOk,
	ReadTimeout
};

enum class IdentifyResult {
	Ok,
	UnsupportedBootloader,
	SyncFail
};

enum class EraseResult {
	Ok,
	UnsupportedBoard,
	Timeout
};

struct Firmware {
	QByteArray image;
	QByteArray crcpad;
	uint32_t image_size;
	uint32_t board_type;
};

class FirmwareUploader : public QObject {
	Q_OBJECT
public:
	FirmwareUploadState upload_state = FirmwareUploadState::None;

	explicit FirmwareUploader(QObject *, QSerialPort *, MavlinkManager *);
	void upload(const QByteArray &file_content);

signals:
	void uploadCompleted(FirmwareUploadResult);
	void stateUpdated(FirmwareUploadState);
	void flashProgressUpdated(uint8_t progress);
	void eraseProgressUpdated(uint8_t progress);

private slots:
	void _handleBytesWritten(qint64);

private:
	QSerialPort *_serial = nullptr;
	MavlinkManager *_mavlink_manager = nullptr;
	QTimer *_erase_timer = nullptr;
	QTimer *_serial_write_timer = nullptr;
	QTimer *_serial_open_timer = nullptr;
	int64_t _bytes_to_write = 0;
	uint32_t _bootloader_rev = 0;
	uint32_t _extf_maxsize = 0;
	uint32_t _board_type = 0;
	uint32_t _board_rev = 0;
	uint32_t _fw_maxsize = 0;
	Firmware _firmware;
	QByteArray MAVLINK_REBOOT_ID0 = QByteArray::fromHex(
			"0xfe2145ff004c00004040000000000000000000000000000000000000000000000000f6"
			"00000000cc37");
	QByteArray MAVLINK_REBOOT_ID1 = QByteArray::fromHex(
			"0xfe2172ff004c00004040000000000000000000000000000000000000000000000000f6"
			"00010000536b");

	void _setUploadState(FirmwareUploadState state);

	void _writeData(const QByteArray &);
	FirmwareUploadResult _tryUploadFirmware(const QByteArray &);
	bool _openSerialPort();
	void _closeSerialPort();
	SyncResult _sync();
	SyncResult _getSync();
	TrySyncResult _trySync();
	EraseResult _erase();
	IdentifyResult _identify();
	uint32_t _getInfo(char param);
	FindBootloaderResult _findBootloader();
	bool _program();
	std::vector<QByteArray> _splitLen(QByteArray &, uint8_t);
	SyncResult _programMulti(const QByteArray &bytes);
	void _sendReboot();
	void _reboot();
	bool _verify_v3();
};

#endif // FIRMWAREUPLOADER_H
