#ifndef FIRMWAREUPLOADER_H
#define FIRMWAREUPLOADER_H

#include <QFileDialog>
#include <QMessageBox>
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <memory>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>

#include "mavlinkmanager.h"

static constexpr uint8_t PROG_MULTI_MAX = 252;

enum class FirmwareUploadError {
	SyncFail,
	InvalidOperation,
	UploadFail,
	UnexpectedResponse,
	UnsopportedBoard,
	UnsupportedBootloader,
	FirmwareImageNotFound,
	FirmwareSizeNotFound,
	BoardIdNotFound,
	TooLargeFirmware,
	EraseFail,
	ReadTimeout,
	IncompatibleBoardType,
	ProgramFail,
	DecodeFail,
	VerifyFail
};

enum class TrySyncResult { Ok, BadSiliconRev, NotInSync, NotOk, ReadTimeout };

struct Firmware {
	QByteArray image;
	QByteArray crcpad;
	uint32_t image_size;
	uint32_t board_type;
};

class FirmwareUploader : public QObject {
	Q_OBJECT
public:
	explicit FirmwareUploader(QObject *, QSerialPort *, MavlinkManager *);
	void upload();

signals:
	void flashFailed(FirmwareUploadError);
	void firmwareUploaded(bool);

private slots:
	void _handleBytesWritten(qint64);
	void _handleEraseTimeout();

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
			"00000000cc37"); //"\xfe\x21\x72\xff\x00\x4c\x00\x00\x40\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf6\x00\x01\x00\x00\x53\x6b";
	QByteArray MAVLINK_REBOOT_ID1 = QByteArray::fromHex(
			"0xfe2172ff004c00004040000000000000000000000000000000000000000000000000f6"
			"00010000536b"); //"\xfe\x21\x45\xff\x00\x4c\x00\x00\x40\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xf6\x00\x00\x00\x00\xcc\x37";

	void _writeData(const QByteArray &);
	bool _tryUploadFirmware(const QByteArray &);
	bool _openSerialPort();
	void _closeSerialPort();
	bool _sync();
	bool _getSync();
	TrySyncResult _trySync();
	bool _erase();
	bool _identify();
	uint32_t _getInfo(char param);
	bool _findBootloader();
	bool _program();
	std::vector<QByteArray> _splitLen(QByteArray &, uint8_t);
	bool _programMulti(const QByteArray &bytes);
	void _sendReboot();
	void _reboot();
	QByteArray _crc(uint32_t padlen);
	uint32_t _crc32(QByteArray &bytes, uint32_t initial_state = 0);
	bool _verify_v3();
};

#endif // FIRMWAREUPLOADER_H
