#include "firmwareuploader.hpp"

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

static constexpr auto kWriteTimeout = std::chrono::seconds{5};
static constexpr auto kOpenTimeout = std::chrono::milliseconds{5000};
static constexpr auto kEraseTimeout = std::chrono::seconds{20};

static constexpr int READ_TIMEOUT_IN_MS = 500;

FirmwareUploader::FirmwareUploader(QObject *parent)
		: QObject{parent} {
	_erase_timer.setSingleShot(true);
	_serial_write_timer.setSingleShot(true);
	_serial_open_timer.setSingleShot(true);

	_serial.setBaudRate(QSerialPort::Baud115200);
	_serial.setDataBits(QSerialPort::Data8);
	_serial.setParity(QSerialPort::NoParity);
	_serial.setStopBits(QSerialPort::OneStop);
	_serial.setFlowControl(QSerialPort::NoFlowControl);

	// connections
	connect(&_serial, &QSerialPort::bytesWritten, this,
					&FirmwareUploader::_handleBytesWritten);
	connect(&_serial, &QSerialPort::errorOccurred, this,
					&FirmwareUploader::_handleError);
}

FirmwareUploader::~FirmwareUploader() {
	qDebug() << "Firmware uploader deleted";
	_closeSerialPort();
}

void FirmwareUploader::upload(const QByteArray &file_content) {
	const auto upload_result = _tryUploadFirmware(file_content);
	emit uploadCompleted(upload_result);
}

void FirmwareUploader::_setUploadState(FirmwareUploadState state) {
	upload_state = state;
	emit stateUpdated(state);
}

void FirmwareUploader::_handleBytesWritten(qint64 bytes) {
	_bytes_to_write -= bytes;
	if (_bytes_to_write == 0) {
		_serial_write_timer.stop();
	}
}

void FirmwareUploader::_handleError(QSerialPort::SerialPortError error) {
	qDebug() << "FIRMWARE THREAD: " << _serial.errorString();
}

FirmwareUploadResult
FirmwareUploader::_tryUploadFirmware(const QByteArray &firmware_image) {
	_setUploadState(FirmwareUploadState::BootloaderSearching);

	static const auto port_regex = QRegularExpression("((ttyACM)|(COM))\\d+");
	// Send reboot to all available devices
	_serial.setBaudRate(QSerialPort::Baud57600);
	for (const auto &port_info : QSerialPortInfo::availablePorts()) {
		if (port_regex.match(port_info.portName()).hasMatch()) {
			_serial.setPortName(port_info.portName());

			if (_openSerialPort()) {
				qDebug() << "SEND REBOOT TO " << _serial.portName();
				_sendReboot();
				_closeSerialPort();
			}
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds{4000});

	FindBootloaderResult bootloaderSearchResult =
			FindBootloaderResult::SerialPortError;
	for (const auto &port_info : QSerialPortInfo::availablePorts()) {
		if (port_regex.match(port_info.portName()).hasMatch()) {
			_serial.setPortName(port_info.portName());

			bootloaderSearchResult = _findBootloader();
			if (bootloaderSearchResult == FindBootloaderResult::Ok) {
				break;
			}
		}
	}

	switch (bootloaderSearchResult) {
	case FindBootloaderResult::SerialPortError:
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::SerialPortError;
	case FindBootloaderResult::SyncFail:
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::BootloaderNotFound;
	case FindBootloaderResult::UnsupportedBootloader:
		_setUploadState(FirmwareUploadState::None);
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
			_setUploadState(FirmwareUploadState::None);
			return FirmwareUploadResult::DecodeFail;
		}
		const uint32_t length = base_64.length();
		const char image_size[4] = {
				static_cast<char>(length >> 24), static_cast<char>(length >> 16),
				static_cast<char>(length >> 8), static_cast<char>(length)};
		const auto to_uncompress = base_64.insert(0, image_size, 4);
		_firmware.image = qUncompress(to_uncompress);
	} else {
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::FirmwareImageNotFound;
	}
	if (firmware_json.contains("image_size")) {
		_firmware.image_size = firmware_json["image_size"];
	} else {
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::FirmwareSizeNotFound;
	}
	if (firmware_json.contains("board_id")) {
		_firmware.board_type = firmware_json["board_id"];
	} else {
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::BoardIdNotFound;
	}

	qDebug() << "FIRMWARE BOARD_ID: " << _firmware.board_type
					 << "IMAGE SIZE: " << _firmware.image_size;

	// firmware compatibility check
	if (_board_type != _firmware.board_type) {
		qDebug() << "Incompatible board type";
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::IncompatibleBoardType;
	}
	if (_fw_maxsize < _firmware.image_size) {
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::TooLargeFirmware;
	}
	// Pad image to 4-byte length
	while (_firmware.image.length() % 4 != 0) {
		_firmware.image += static_cast<char>(0xFF);
	}
	_sync();
	qDebug() << "FIRMWARE COMPATIBLE WITH BOARD";

	qDebug() << "ERASING...";
	_setUploadState(FirmwareUploadState::Erasing);
	switch (_erase()) {
	case EraseResult::Timeout: {
		qDebug() << "ERASE FAILED";
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::EraseFail;
	}
	case EraseResult::UnsupportedBoard: {
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::UnsupportedBoard;
	}
	case EraseResult::Ok: {
		qDebug() << "ERASE COMPLETED";
	}
	}

	_setUploadState(FirmwareUploadState::Flashing);
	qDebug() << "PROGRAM STARTED";
	const auto program_success = _program();
	if (!program_success) {
		qDebug() << "PROGRAM FAIL";
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::ProgramFail;
	}
	qDebug() << "PROGRAM COMPLETED";
	qDebug() << "VERIFYING STARTED";
	const auto verify_success = _verify_v3();
	if (!verify_success) {
		qDebug() << "VERIFYING FAILED";
		_setUploadState(FirmwareUploadState::None);
		return FirmwareUploadResult::VerificationFail;
	}
	qDebug() << "VERIFYING COMPLETED";
	_reboot();
	_closeSerialPort();
	_setUploadState(FirmwareUploadState::None);
	return FirmwareUploadResult::Ok;
}

bool FirmwareUploader::_openSerialPort() {
	_serial.setDataBits(QSerialPort::Data8);
	_serial.setParity(QSerialPort::NoParity);
	_serial.setStopBits(QSerialPort::OneStop);
	_serial.setFlowControl(QSerialPort::NoFlowControl);
	_serial_open_timer.start(kOpenTimeout);
	while (_serial_open_timer.isActive()) {
		if (_serial.open(QIODevice::ReadWrite)) {
			qDebug() << "Serial connected";
			_serial_open_timer.stop();
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds{200});
		QApplication::processEvents();
	}
	qDebug() << "Serial connect timeout";
	return false;
}

void FirmwareUploader::_closeSerialPort() {
	if (_serial.isOpen()) {
		_serial.close();
		qDebug() << "Serial disconnected";
	}
}

void FirmwareUploader::_writeData(const QByteArray &data) {
	const qint64 written = _serial.write(data);
	if (written == data.size()) {
		// qDebug() << "SEND: " << data;
		_bytes_to_write += written;
		_serial_write_timer.start(kWriteTimeout);
	} else {
		const auto error = tr("Failed to write all data to port %1.\nError: %2")
													 .arg(_serial.portName(), _serial.errorString());
		qDebug() << error;
		// QMessageBox::critical(nullptr, tr("Serial port error"), error);
	}
}

SyncResult FirmwareUploader::_sync() {
	_serial.readAll();
	const char data[2] = {GET_SYNC, EOC};
	QByteArray sync_data(data, 2);
	_writeData(sync_data);
	return _getSync();
}

SyncResult FirmwareUploader::_getSync() {
	_serial.flush();
	if (_serial.bytesAvailable() == 0) {
		_serial.waitForReadyRead(READ_TIMEOUT_IN_MS);
	}
	auto data = _serial.read(1);
	if (data.length() < 1) {
		qDebug() << "READ TIMEOUT 1";
		return SyncResult::ReadTimeout;
	}
	if (data[0] != INSYNC) {
		qDebug() << "SYNC FAIL 1";
		return SyncResult::Fail;
	}
	if (_serial.bytesAvailable() == 0) {
		const auto read_result = _serial.waitForReadyRead(READ_TIMEOUT_IN_MS);
		qDebug() << "READY READ RESULT 2: " << read_result;
	}
	data = _serial.read(1);
	if (data.length() < 1) {
		qDebug() << "READ TIMEOUT 2";
		return SyncResult::ReadTimeout;
	}
	if (data[0] == INVALID) {
		qDebug() << "INVALID OPERATION 2";
		return SyncResult::InvalidOperation;
	}
	if (data[0] == FAILED) {
		qDebug() << "UPLOAD FAIL 2";
		return SyncResult::Fail;
	}
	if (data[0] != OK) {
		qDebug() << "UNEXPECTED RESPONSE 2";
		return SyncResult::UnexpectedResponse;
	}
	return SyncResult::Ok;
}

TrySyncResult FirmwareUploader::_trySync() {
	_serial.flush();
	if (_serial.bytesAvailable() == 0) {
		_serial.waitForReadyRead(READ_TIMEOUT_IN_MS);
	}
	auto data = _serial.read(1);
	if (data.length() < 1) {
		return TrySyncResult::ReadTimeout;
	}
	if (data[0] != INSYNC) {
		return TrySyncResult::NotInSync;
	}
	if (_serial.bytesAvailable() == 0) {
		_serial.waitForReadyRead(READ_TIMEOUT_IN_MS);
	}
	data = _serial.read(1);
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
	_erase_timer.start(kEraseTimeout);
	while (_erase_timer.isActive()) {
		switch (_trySync()) {
		case TrySyncResult::Ok: {
			_erase_timer.stop();
			return EraseResult::Ok;
		} break;
		case TrySyncResult::ReadTimeout:
		case TrySyncResult::NotInSync:
		case TrySyncResult::NotOk: {
			const auto erase_timeout_in_ms =
					std::chrono::duration_cast<std::chrono::milliseconds>(kEraseTimeout)
							.count();
			const double progress = static_cast<float>(erase_timeout_in_ms -
																								 _erase_timer.remainingTime()) /
															static_cast<float>(erase_timeout_in_ms) * 100.0;
			emit eraseProgressUpdated(static_cast<uint8_t>(std::round(progress)));
		} break;
		case TrySyncResult::BadSiliconRev: {
			_erase_timer.stop();
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
		qDebug() << "SYNC FAILED";
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
	if (_serial.bytesAvailable() < 4) {
		_serial.waitForReadyRead(300);
	}
	// little endian format
	const auto raw_data = _serial.read(4);
	_getSync();
	uint32_t uint_result =
			((raw_data[3] << 24) & 0xff000000) | ((raw_data[2] << 16) & 0x00ff0000) |
			((raw_data[1] << 8) & 0x0000ff00) | (raw_data[0] & 0x000000ff);
	return uint_result;
}

FindBootloaderResult FirmwareUploader::_findBootloader() {
	qDebug() << "SEARCHING BOOTLOADER...";

	// attempt to connect on baud 115200
	_serial.setBaudRate(QSerialPort::Baud115200);
	qDebug() << "Try connect to " << _serial.portName() << "on baud "
					 << _serial.baudRate();
	if (_openSerialPort()) {
		switch (_identify()) {
		case IdentifyResult::UnsupportedBootloader:
			_closeSerialPort();
			return FindBootloaderResult::UnsupportedBootloader;
		case IdentifyResult::SyncFail:
			break;
		case IdentifyResult::Ok: {
			qDebug() << "BOOTLOADER FOUND on BAUD: " << 115200 << '\n';
			return FindBootloaderResult::Ok;
		}
		}
	} /*else {
		return FindBootloaderResult::SerialPortError;
	}*/

	_closeSerialPort();
	std::this_thread::sleep_for(std::chrono::milliseconds{500});

	// attempt to connect on baud 57600
	_serial.setBaudRate(QSerialPort::Baud57600);
	qDebug() << "Try connect to " << _serial.portName() << "on baud "
					 << _serial.baudRate();
	if (!_openSerialPort()) {
		return FindBootloaderResult::SerialPortError;
	}
	switch (_identify()) {
	case IdentifyResult::UnsupportedBootloader:
		_closeSerialPort();
		return FindBootloaderResult::UnsupportedBootloader;
	case IdentifyResult::SyncFail:
		_closeSerialPort();
		return FindBootloaderResult::SyncFail;
	case IdentifyResult::Ok: {
		qDebug() << "BOOTLOADER FOUND on BAUD: " << 57600 << '\n';
		return FindBootloaderResult::Ok;
	}
	}
	// unreachable code, gcc warning suppress
	return FindBootloaderResult::SerialPortError;
}

bool FirmwareUploader::_program() {
	const auto groups = _splitLen(_firmware.image, PROG_MULTI_MAX);
	size_t uploaded = 0;
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
		const double progress = static_cast<float>(uploaded) /
														static_cast<float>(groups.size()) * 100.0;
		emit flashProgressUpdated(static_cast<uint8_t>(std::round(progress)));
		uploaded++;
	}
	return true;
}

std::vector<QByteArray> FirmwareUploader::_splitLen(QByteArray &image,
																										uint8_t bunch_size) {
	std::vector<QByteArray> groups = {};
	for (qsizetype i = 0; i < image.size(); i += bunch_size) {
		const auto slice = image.mid(i, bunch_size);
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
	QByteArray data_length(length_bytes.data(),
												 static_cast<qsizetype>(length_bytes.size()));
	QByteArray data_eoc(1, EOC);
	_writeData(data_prog_multi);
	_writeData(data_length);
	_writeData(bytes);
	_writeData(data_eoc);
	return _getSync();
}

void FirmwareUploader::_sendReboot() {
	qDebug() << "REBOOT ATTEMPT";
	// Reboot via mavlink
	_serial.flush();
	_writeData(MAVLINK_REBOOT_ID1);
	if (_serial.bytesToWrite() > 0) {
		_serial.waitForBytesWritten(300);
	}
	_writeData(MAVLINK_REBOOT_ID0);
	if (_serial.bytesToWrite() > 0) {
		_serial.waitForBytesWritten(300);
	}

	// Reboot via NSH
	_writeData(NSH_INIT);
	_writeData(NSH_REBOOT_BL);
	_writeData(NSH_INIT);
	_writeData(NSH_REBOOT);
	if (_serial.bytesToWrite() > 0) {
		_serial.waitForBytesWritten(300);
	}

	if (_bootloader_rev >= 3) {
		_getSync();
	}
}

void FirmwareUploader::_reboot() {
	const char data[2] = {REBOOT, EOC};
	const QByteArray reboot_data(data, 2);
	_writeData(reboot_data);
	_serial.flush();
}

bool FirmwareUploader::_verify_v3() {
	const auto expected_crc = Crc::crc(_fw_maxsize, _firmware.image);
	const char data[2] = {GET_CRC, EOC};
	const QByteArray get_crc_data(data, 2);
	_writeData(get_crc_data);
	if (_serial.bytesAvailable() < 4) {
		_serial.waitForReadyRead(300);
	}
	const auto crc_response = _serial.read(4);
	_getSync();
	if (crc_response != expected_crc) {
		return false;
	}
	return true;
}
