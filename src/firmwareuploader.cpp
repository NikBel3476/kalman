#include "firmwareuploader.h"

static constexpr char REBOOT = '0';
static constexpr char INFO_BL_REV = '\x01';
static constexpr char INFO_BOARD_ID = '\x02';
static constexpr char INFO_BOARD_REV = '\x03';
static constexpr char INFO_FLASH_SIZE = '\x04';
static constexpr char INFO_EXTF_SIZE = '\x06';
static constexpr char OK = '\x10';
static constexpr char FAILED = '\x11';
static constexpr char INSYNC = '\x12';
static constexpr char INVALID = '\x13';
static constexpr char BAD_SILICON_REV = '\x14';
static constexpr char EOC = '\x20';
static constexpr char GET_SYNC = '\x21';
static constexpr char GET_DEVICE = '\x22';
static constexpr char CHIP_ERASE = '\x23';
static constexpr char PROG_MULTI = '\x27';
static constexpr char GET_CRC = '\x29';

static constexpr uint8_t BL_REV_MIN = 2;
static constexpr uint8_t BL_REV_MAX = 5;
static constexpr auto NSH_INIT = "\x0d\x0d\x0d";
static constexpr auto NSH_REBOOT_BL = "reboot -b\x0a";
static constexpr auto NSH_REBOOT = "reboot\x0a";

static const uint8_t SYSTEM_ID = 255;
static const uint8_t COMP_ID = MAV_COMP_ID_MISSIONPLANNER;
static const uint8_t TARGET_SYSTEM_ID = 1;
static const uint8_t TARGET_COMP_ID = MAV_COMP_ID_AUTOPILOT1;

static constexpr auto kWriteTimeout = std::chrono::seconds{5};
static constexpr auto kOpenTimeout = std::chrono::milliseconds{1000};
static constexpr auto kEraseTimeout = std::chrono::seconds{20};

static constexpr int READ_TIMEOUT_IN_MS = 2000;

static constexpr std::array<uint32_t, 256> CRCTAB = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
		0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
		0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
		0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
		0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
		0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
		0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
		0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
		0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
		0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
		0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
		0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
		0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
		0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
		0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
		0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
		0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
		0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
		0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
		0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
		0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
		0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

FirmwareUploader::FirmwareUploader(QObject *parent, QSerialPort *serial,
																	 MavlinkManager *mavlink_manager)
		: QObject{parent}, _serial(serial), _mavlink_manager(mavlink_manager),
			_erase_timer(new QTimer(this)), _serial_write_timer(new QTimer(this)),
			_serial_open_timer(new QTimer(this)) {
	_erase_timer->setSingleShot(true);
	_serial_write_timer->setSingleShot(true);
	_serial_open_timer->setSingleShot(true);

	connect(_serial, &QSerialPort::bytesWritten, this,
					&FirmwareUploader::_handleBytesWritten);
}

void FirmwareUploader::upload() {
	auto fileContentReady = [this](const QString &file_name,
																 const QByteArray &file_content) {
		if (!file_name.isEmpty()) {
			qDebug() << "UPLOAD STARTED\n";
			const auto upload_result = _tryUploadFirmware(file_content);
			emit uploadCompleted(upload_result);
		}
	};
	QFileDialog::getOpenFileContent("*.apj", fileContentReady);
}

void FirmwareUploader::_handleBytesWritten(qint64 bytes) {
	_bytes_to_write -= bytes;
	if (_bytes_to_write == 0) {
		_serial_write_timer->stop();
	}
}

FirmwareUploadResult
FirmwareUploader::_tryUploadFirmware(const QByteArray &firmware_image) {
	switch (_findBootloader()) {
	case FindBootloaderResult::SerialPortError:
	case FindBootloaderResult::SyncFail:
		return FirmwareUploadResult::BootloaderNotFound;
	case FindBootloaderResult::UnsupportedBootloader:
		return FirmwareUploadResult::UnsupportedBootloader;
	case FindBootloaderResult::Ok:
		break;
	}

	qDebug() << std::format(
			"extf_maxsize: {} board_type: {} board_rev: {} fw_maxsize: {}",
			_extf_maxsize, _board_type, _board_rev, _fw_maxsize);
	auto firmware_json = nlohmann::json::parse(firmware_image);
	_firmware.crcpad = QByteArray::fromStdString("\xff\xff\xff\xff");
	if (firmware_json.contains("image")) {
		const std::string image_str{firmware_json["image"]};
		const auto image_bytes = QByteArray::fromStdString(image_str);
		auto base_64 = QByteArray::fromBase64(image_bytes);
		if (base_64.length() == 0) {
			return FirmwareUploadResult::DecodeFail;
		}
		const uint32_t length = base_64.length();
		const char image_size[4] = {
				static_cast<char>(length >> 24), static_cast<char>(length >> 16),
				static_cast<char>(length >> 8), static_cast<char>(length)};
		const auto to_uncompress = base_64.insert(0, image_size, 4);
		_firmware.image = qUncompress(to_uncompress);
	} else {
		return FirmwareUploadResult::FirmwareImageNotFound;
	}
	if (firmware_json.contains("image_size")) {
		_firmware.image_size = firmware_json["image_size"];
	} else {
		return FirmwareUploadResult::FirmwareSizeNotFound;
	}
	if (firmware_json.contains("board_id")) {
		_firmware.board_type = firmware_json["board_id"];
	} else {
		return FirmwareUploadResult::BoardIdNotFound;
	}

	qDebug() << "FIRMWARE BOARD_ID: " << _firmware.board_type
					 << "IMAGE SIZE: " << _firmware.image_size;

	// firmware compatibility check
	if (_board_type != _firmware.board_type) {
		return FirmwareUploadResult::IncompatibleBoardType;
	}
	if (_fw_maxsize < _firmware.image_size) {
		return FirmwareUploadResult::TooLargeFirmware;
	}
	// Pad image to 4-byte length
	while (_firmware.image.length() % 4 != 0) {
		_firmware.image += static_cast<char>(0xFF);
	}
	_sync();
	qDebug() << "FIRMWARE COMPATIBLE WITH BOARD";
	qDebug() << "ERASING...";
	emit stateUpdated(FirmwareUploadState::Erasing);
	switch (_erase()) {
	case EraseResult::Timeout: {
		qDebug() << "ERASE FAILED";
		return FirmwareUploadResult::EraseFail;
	}
	case EraseResult::UnsupportedBoard: {
		return FirmwareUploadResult::UnsupportedBoard;
	}
	case EraseResult::Ok: {
		qDebug() << "ERASE COMPLETED";
	}
	}

	emit stateUpdated(FirmwareUploadState::Flashing);
	qDebug() << "PROGRAM STARTED";
	const auto program_success = _program();
	if (!program_success) {
		return FirmwareUploadResult::ProgramFail;
		qDebug() << "PROGRAM FAIL";
	}
	qDebug() << "PROGRAM COMPLETED";
	qDebug() << "VERIFYING STARTED";
	const auto verify_success = _verify_v3();
	if (!verify_success) {
		qDebug() << "VERIFYING FAILED";
		return FirmwareUploadResult::VerificationFail;
	}
	qDebug() << "VERIFYING COMPLETED";
	_reboot();
	_closeSerialPort();
	emit stateUpdated(FirmwareUploadState::None);
	return FirmwareUploadResult::Ok;
}

bool FirmwareUploader::_openSerialPort() {
	_serial_open_timer->start(kOpenTimeout);
	while (true) {
		if (_serial->open(QIODevice::ReadWrite)) {
			qDebug() << "Serial connected\n";
			return true;
		} else if (!_serial_open_timer->isActive()) {
			return false;
		}
	}
	return false;
}

void FirmwareUploader::_closeSerialPort() {
	if (_serial->isOpen()) {
		_serial->close();
		qDebug() << "Serial disconnected";
	}
}

void FirmwareUploader::_writeData(const QByteArray &data) {
	const qint64 written = _serial->write(data);
	if (written == data.size()) {
		qDebug() << "SEND: " << data;
		_bytes_to_write += written;
		_serial_write_timer->start(kWriteTimeout);
	} else {
		const auto error = tr("Failed to write all data to port %1.\n"
													"Error: %2")
													 .arg(_serial->portName(), _serial->errorString());
		QMessageBox::critical(nullptr, tr("Serial port error"), error);
	}
}

SyncResult FirmwareUploader::_sync() {
	_serial->readAll();
	const char data[2] = {GET_SYNC, EOC};
	QByteArray sync_data(data, 2);
	_writeData(sync_data);
	return _getSync();
}

SyncResult FirmwareUploader::_getSync() {
	_serial->flush();
	if (_serial->bytesAvailable() == 0) {
		_serial->waitForReadyRead(READ_TIMEOUT_IN_MS);
	}
	auto data = _serial->read(1);
	if (data.length() < 1) {
		qDebug() << "READ TIMEOUT 1\n";
		return SyncResult::ReadTimeout;
	}
	if (data[0] != INSYNC) {
		qDebug() << "SYNC FAIL 1\n";
		return SyncResult::Fail;
	}
	if (_serial->bytesAvailable() == 0) {
		const auto read_result = _serial->waitForReadyRead(READ_TIMEOUT_IN_MS);
		qDebug() << "READY READ RESULT 2: " << read_result;
	}
	data = _serial->read(1);
	if (data.length() < 1) {
		qDebug() << "READ TIMEOUT 2\n";
		return SyncResult::ReadTimeout;
	}
	if (data[0] == INVALID) {
		qDebug() << "INVALID OPERATION 2\n";
		return SyncResult::InvalidOperation;
	}
	if (data[0] == FAILED) {
		qDebug() << "UPLOAD FAIL 2\n";
		return SyncResult::Fail;
	}
	if (data[0] != OK) {
		qDebug() << "UNEXPECTED RESPONSE 2\n";
		return SyncResult::UnexpectedResponse;
	}
	return SyncResult::Ok;
}

TrySyncResult FirmwareUploader::_trySync() {
	_serial->flush();
	if (_serial->bytesAvailable() == 0) {
		_serial->waitForReadyRead(READ_TIMEOUT_IN_MS);
	}
	auto data = _serial->read(1);
	if (data.length() < 1) {
		return TrySyncResult::ReadTimeout;
	}
	if (data[0] != INSYNC) {
		return TrySyncResult::NotInSync;
	}
	if (_serial->bytesAvailable() == 0) {
		_serial->waitForReadyRead(READ_TIMEOUT_IN_MS);
	}
	data = _serial->read(1);
	if (data.length() < 1) {
		return TrySyncResult::ReadTimeout;
	}
	if (data[0] == BAD_SILICON_REV) {
		return TrySyncResult::BadSiliconRev;
	}
	if (data[0] != OK) {
		return TrySyncResult::NotOk;
	}
	return TrySyncResult::Ok;
}

EraseResult FirmwareUploader::_erase() {
	const char erase_data[2] = {CHIP_ERASE, EOC};
	QByteArray data(erase_data, 2);
	_writeData(data);
	_erase_timer->start(kEraseTimeout);
	while (_erase_timer->isActive()) {
		switch (_trySync()) {
		case TrySyncResult::Ok: {
			_erase_timer->stop();
			return EraseResult::Ok;
		} break;
		case TrySyncResult::ReadTimeout:
		case TrySyncResult::NotInSync:
		case TrySyncResult::NotOk: {

		} break;
		case TrySyncResult::BadSiliconRev: {
			_erase_timer->stop();
			return EraseResult::UnsupportedBoard;
		}
		}
	}
	qDebug() << "ERASE TIMEOUT";
	return EraseResult::Timeout;
}

IdentifyResult FirmwareUploader::_identify() {
	switch (_sync()) {
	case SyncResult::Fail:
	case SyncResult::InvalidOperation:
	case SyncResult::ReadTimeout:
	case SyncResult::UnexpectedResponse: {
		qDebug() << "SYNC FAILED\n";
		return IdentifyResult::SyncFail;
	}
	case SyncResult::Ok: {
	}
	}

	_bootloader_rev = _getInfo(INFO_BL_REV);
	qDebug() << "BOOTLOADER REV: " << _bootloader_rev;
	if (_bootloader_rev < BL_REV_MIN || _bootloader_rev > BL_REV_MAX) {
		return IdentifyResult::UnsupportedBootloader;
	}
	_extf_maxsize = _getInfo(INFO_EXTF_SIZE);
	_board_type = _getInfo(INFO_BOARD_ID);
	_board_rev = _getInfo(INFO_BOARD_REV);
	_fw_maxsize = _getInfo(INFO_FLASH_SIZE);
	return IdentifyResult::Ok;
}

uint32_t FirmwareUploader::_getInfo(const char param) {
	const char get_info_data[3] = {GET_DEVICE, param, EOC};
	QByteArray get_info_msg(get_info_data, 3);
	_writeData(get_info_msg);
	if (_serial->bytesAvailable() < 4) {
		_serial->waitForReadyRead(300);
	}
	// little endian format
	const auto raw_data = _serial->read(4);
	qDebug() << "getInfo RAW: " << raw_data;
	_getSync();
	uint32_t uint_result =
			((raw_data[3] << 24) & 0xff000000) | ((raw_data[2] << 16) & 0x00ff0000) |
			((raw_data[1] << 8) & 0x0000ff00) | (raw_data[0] & 0x000000ff);
	qDebug() << std::format("uint bin: {:b}", uint_result);
	qDebug() << "INT RESULT: " << uint_result << "TEST: ";
	return uint_result;
}

FindBootloaderResult FirmwareUploader::_findBootloader() {
	qDebug() << "FINDING BOOTLOADER...";
	// attempt to connect on baud 115200
	_serial->setBaudRate(QSerialPort::Baud115200);
	if (_openSerialPort()) {
		switch (_identify()) {
		case IdentifyResult::UnsupportedBootloader:
			return FindBootloaderResult::UnsupportedBootloader;
		case IdentifyResult::SyncFail:
			break;
		case IdentifyResult::Ok: {
			qDebug() << "BOOTLOADER FOUND on BAUD: " << 115200 << '\n';
			return FindBootloaderResult::Ok;
		}
		}
	} else {
		return FindBootloaderResult::SerialPortError;
	}

	qDebug() << "CANNOT OPEN SERIAL PORT ATTEMPT 1\n";

	_sendReboot();
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	_closeSerialPort();
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	// attempt to connect on baud 57600
	_serial->setBaudRate(QSerialPort::Baud57600);
	if (!_openSerialPort()) {
		qDebug() << "CANNOT OPEN SERIAL PORT ATTEMPT 2\n";
		return FindBootloaderResult::SerialPortError;
	}
	switch (_identify()) {
	case IdentifyResult::UnsupportedBootloader:
		return FindBootloaderResult::UnsupportedBootloader;
	case IdentifyResult::SyncFail:
		return FindBootloaderResult::SyncFail;
	case IdentifyResult::Ok: {
		qDebug() << "BOOTLOADER FOUND on BAUD: " << 57600 << '\n';
		return FindBootloaderResult::Ok;
	}
	}
}

bool FirmwareUploader::_program() {
	const auto groups = _splitLen(_firmware.image, PROG_MULTI_MAX);
	uint8_t progress = 0;
	for (const auto &bytes : groups) {
		switch (_programMulti(bytes)) {
		case SyncResult::Fail:
		case SyncResult::InvalidOperation:
		case SyncResult::ReadTimeout:
		case SyncResult::UnexpectedResponse:
			return false;
		case SyncResult::Ok:
			break;
		}
		emit flashProgressUpdated(progress);
		progress++;
	}
	return true;
}

std::vector<QByteArray> FirmwareUploader::_splitLen(QByteArray &image,
																										uint8_t bunch_size) {
	std::vector<QByteArray> groups = {};
	for (qsizetype i = 0; i < image.size(); i += bunch_size) {
		// const auto last =  std::min(static_cast<size_t>(image.size()), i +
		// bunch_size); std::array<char, PROG_MULTI_MAX> bunch{};
		// std::copy(image.constBegin() + i, image.constBegin() + last,
		// bunch.begin());
		const auto slice = image.mid(i, bunch_size);
		qDebug() << "SPLIT LEN SLICE SIZE: " << slice.length();
		groups.push_back(slice);
	}
	return groups;
}

SyncResult FirmwareUploader::_programMulti(const QByteArray &bytes) {
	std::vector<char> length_bytes = {static_cast<char>(bytes.length() >> 24),
																		static_cast<char>(bytes.length() >> 16),
																		static_cast<char>(bytes.length() >> 8),
																		static_cast<char>(bytes.length())};
	length_bytes.erase(length_bytes.begin(),
										 std::find_if(length_bytes.begin(), length_bytes.end(),
																	[](const auto &byte) { return byte; }));
	QByteArray data_prog_multi(1, PROG_MULTI);
	QByteArray data_length(length_bytes.data(), length_bytes.size());
	QByteArray data_eoc(1, EOC);
	_writeData(data_prog_multi);
	_writeData(data_length);
	_writeData(bytes);
	_writeData(data_eoc);
	return _getSync();
}

void FirmwareUploader::_sendReboot() {
	qDebug() << "REBOOT ATTEMPT\n";
	// Reboot via mavlink
	const auto result = _serial->flush();
	qDebug() << "REBOOT BYTES WRITTEN: " << result;
	_writeData(MAVLINK_REBOOT_ID1);
	if (_serial->bytesToWrite() > 0) {
		_serial->waitForBytesWritten(300);
	}
	_writeData(MAVLINK_REBOOT_ID0);
	if (_serial->bytesToWrite() > 0) {
		_serial->waitForBytesWritten(300);
	}

	// Reboot via NSH
	_writeData(NSH_INIT);
	_writeData(NSH_REBOOT_BL);
	_writeData(NSH_INIT);
	_writeData(NSH_REBOOT);
	if (_serial->bytesToWrite() > 0) {
		_serial->waitForBytesWritten(300);
	}

	if (_bootloader_rev >= 3) {
		_getSync();
	}
}

void FirmwareUploader::_reboot() {
	const char data[2] = {REBOOT, EOC};
	const QByteArray reboot_data(data, 2);
	_writeData(reboot_data);
	_serial->flush();
}

bool FirmwareUploader::_verify_v3() {
	const auto expected_crc = _crc(_fw_maxsize);
	const char data[2] = {GET_CRC, EOC};
	const QByteArray get_crc_data(data, 2);
	_writeData(get_crc_data);
	if (_serial->bytesAvailable() < 4) {
		_serial->waitForReadyRead(300);
	}
	const auto crc_response = _serial->read(4);
	_getSync();
	const auto crc_response_uint = crc_response.toUInt();
	qDebug() << "RESPONSE BYTES: " << crc_response
					 << "RESPONSE: " << crc_response_uint
					 << "EXPECTED CRC BYTES: " << expected_crc
					 << "EXPECTED CRC: " << expected_crc.toUInt();
	if (crc_response != expected_crc) {
		return false;
	}
	return true;
}

QByteArray FirmwareUploader::_crc(uint32_t padlen) {
	auto state = _crc32(_firmware.image, 0);
	for (size_t _i = _firmware.image.length(); _i < padlen - 1; _i += 4) {
		state = _crc32(_firmware.crcpad, state);
	}
	const char state_bytes[4] = {
			static_cast<char>(state), static_cast<char>(state >> 8),
			static_cast<char>(state >> 16), static_cast<char>(state >> 24)};
	return QByteArray(state_bytes, 4);
}

/// crc32 exposed for use by chibios.py
uint32_t FirmwareUploader::_crc32(QByteArray &bytes, uint32_t initial_state) {
	auto state = initial_state;
	for (const auto &byte : bytes) {
		const auto index = (state ^ byte) & 0xff;
		state = CRCTAB[index] ^ (state >> 8);
	}
	return state;
}
