#include "firmwareuploadpage.hpp"

const int MIN_PAGE_WIDTH = 150;
const int MAX_PAGE_WIDTH = 250;

FirmwareUploadPage::FirmwareUploadPage(QWidget *parent)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_drone_type_box(new QComboBox()),
			_firmware_upload_button(new QPushButton()),
			_firmware_upload_status_label(new QLabel()),
			_progress_bar(new QProgressBar()) {
	_layout->setAlignment(Qt::AlignCenter);
	_layout->addWidget(_drone_type_box);
	_layout->addWidget(_firmware_upload_button);
	_layout->addWidget(_firmware_upload_status_label);
	_layout->addWidget(_progress_bar);

	_drone_type_box->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_drone_type_box->setMinimumWidth(MIN_PAGE_WIDTH);
	_drone_type_box->setMaximumWidth(MAX_PAGE_WIDTH);
	_drone_type_box->setVisible(false);

	_firmware_upload_button->setText(tr("Upload firmware"));
	_firmware_upload_button->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_firmware_upload_button->setMinimumWidth(MIN_PAGE_WIDTH);
	_firmware_upload_button->setMaximumWidth(MAX_PAGE_WIDTH);

	_drone_type = DroneType::SingleWing;
	_drone_type_box->addItem(tr("Single wing"),
													 static_cast<int>(DroneType::SingleWing));
	_drone_type_box->addItem(tr("Quadrocopter"),
													 static_cast<int>(DroneType::Quadrocopter));
	_drone_type_box->setVisible(false);

	_firmware_upload_status_label->setAlignment(Qt::AlignCenter);
	_firmware_upload_status_label->setText("");
	_firmware_upload_status_label->setVisible(false);

	_progress_bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	_progress_bar->setAlignment(Qt::AlignCenter);
	_progress_bar->setRange(0, 100);
	_progress_bar->setVisible(false);

	// connections
	connect(_drone_type_box, &QComboBox::currentIndexChanged, this,
					&FirmwareUploadPage::_handleDroneTypeBoxChange);
	connect(_firmware_upload_button, &QPushButton::pressed, this,
					&FirmwareUploadPage::_handleUploadButtonPress);
}

void FirmwareUploadPage::_handleDroneTypeBoxChange(int index) {
	auto drone_type_maybe = _drone_type_box->itemData(index);
	if (drone_type_maybe.canConvert<DroneType>()) {
		_drone_type = drone_type_maybe.value<DroneType>();
		qDebug() << std::format("Drone type: {}\n", static_cast<int>(_drone_type));
	}
}

void FirmwareUploadPage::_handleUploadButtonPress() {
	auto fileContentReady = [this](const QString &file_name,
																 const QByteArray &file_content) {
		if (!file_name.isEmpty()) {
			emit uploadFirmwareStarted(_drone_type);
			_firmware_upload_button->setVisible(false);
			// std::this_thread::sleep_for(std::chrono::seconds(2));
			const auto thread = QThread::create([this, file_content]() {
				const auto firmware_uploader = new FirmwareUploader();

				// TODO: add disconnections
				connect(firmware_uploader, &FirmwareUploader::stateUpdated, this,
								&FirmwareUploadPage::_handleFirmwareUploadStateUpdate);
				connect(firmware_uploader, &FirmwareUploader::flashProgressUpdated,
								this, &FirmwareUploadPage::_handleFlashProgressUpdate);
				connect(firmware_uploader, &FirmwareUploader::eraseProgressUpdated,
								this, &FirmwareUploadPage::_handleEraseProgressUpdate);
				connect(firmware_uploader, &FirmwareUploader::uploadCompleted, this,
								&FirmwareUploadPage::_handleFirmwareUploadCompletion);

				firmware_uploader->upload(file_content);
			});

			thread->start();
		}
	};
	QFileDialog::getOpenFileContent("*.apj", fileContentReady);
}

void FirmwareUploadPage::_handleFirmwareUploadStateUpdate(
		FirmwareUploadState new_state) {
	switch (new_state) {
	case FirmwareUploadState::Rebooting: {
		_firmware_upload_status_label->setVisible(true);
		_firmware_upload_status_label->setText(tr("Rebooting..."));
	} break;
	case FirmwareUploadState::BootloaderSearching: {
		_firmware_upload_status_label->setVisible(true);
		_firmware_upload_status_label->setText(tr("Searching bootloader..."));
	} break;
	case FirmwareUploadState::Erasing: {
		_firmware_upload_status_label->setVisible(true);
		_firmware_upload_status_label->setText(tr("Erasing..."));
		_progress_bar->setVisible(true);
	} break;
	case FirmwareUploadState::Flashing: {
		_firmware_upload_status_label->setVisible(true);
		_firmware_upload_status_label->setText(tr("Flashing..."));
		_progress_bar->setVisible(true);
	} break;
	case FirmwareUploadState::None: {
		_progress_bar->setVisible(false);
	}
	}
}

void FirmwareUploadPage::_handleFlashProgressUpdate(uint8_t progress) {
	_progress_bar->setValue(progress);
}

void FirmwareUploadPage::_handleEraseProgressUpdate(uint8_t progress) {
	_progress_bar->setValue(progress);
}

void FirmwareUploadPage::_handleFirmwareUploadCompletion(
		FirmwareUploadResult result) {
	switch (result) {
	case FirmwareUploadResult::Ok: {
		_firmware_upload_status_label->setText(tr("Upload successfully completed"));
	} break;
	case FirmwareUploadResult::FirmwareImageNotFound: {
		_firmware_upload_status_label->setText(QString("%1\n%2").arg(
				tr("Upload failed"), tr("Firmware image not found")));
	} break;
	case FirmwareUploadResult::FirmwareSizeNotFound: {
		_firmware_upload_status_label->setText(QString("%1\n%2").arg(
				tr("Upload failed"), tr("Firmware size not found")));
	} break;
	case FirmwareUploadResult::BoardIdNotFound: {
		_firmware_upload_status_label->setText(
				QString("%1\n%2").arg(tr("Upload failed"), tr("Board id not found")));
	} break;
	case FirmwareUploadResult::BootloaderNotFound: {
		_firmware_upload_status_label->setText(
				QString("%1\n%2").arg(tr("Upload failed"), tr("Bootloader not found")));
	} break;
	case FirmwareUploadResult::TooLargeFirmware: {
		_firmware_upload_status_label->setText(
				QString("%1\n%2").arg(tr("Upload failed"), tr("Too large firmware")));
	} break;
	case FirmwareUploadResult::DecodeFail: {
		_firmware_upload_status_label->setText(
				QString("%1\n%2").arg(tr("Upload failed"), tr("Decode fail")));
	} break;
	case FirmwareUploadResult::EraseFail: {
		_firmware_upload_status_label->setText(
				QString("%1\n%2").arg(tr("Upload failed"), tr("Erase fail")));
	} break;
	case FirmwareUploadResult::ProgramFail: {
		_firmware_upload_status_label->setText(
				QString("%1\n%2").arg(tr("Upload failed"), tr("Program fail")));
	} break;
	case FirmwareUploadResult::VerificationFail: {
		_firmware_upload_status_label->setText(
				QString("%1\n%2").arg(tr("Upload failed"), tr("Verification fail")));
	} break;
	case FirmwareUploadResult::UnsupportedBoard: {
		_firmware_upload_status_label->setText(
				QString("%1\n%2").arg(tr("Upload failed"), tr("Unsupported board")));
	} break;
	case FirmwareUploadResult::UnsupportedBootloader: {
		_firmware_upload_status_label->setText(QString("%1\n%2").arg(
				tr("Upload failed"), tr("Unsupported bootloader")));
	} break;
	case FirmwareUploadResult::IncompatibleBoardType: {
		_firmware_upload_status_label->setText(QString("%1\n%2").arg(
				tr("Upload failed"), tr("Incompatible board type")));
	} break;
	case FirmwareUploadResult::SerialPortError: {
		_firmware_upload_status_label->setText(
				QString("%1\n%2").arg(tr("Upload failed"), tr("Serial port error")));
	} break;
	}
	_firmware_upload_button->setVisible(true);
}
