#include "firmwareuploadpage.h"

const int MIN_PAGE_WIDTH = 150;
const int MAX_PAGE_WIDTH = 250;

FirmwareUploadPage::FirmwareUploadPage(QWidget *parent)
		: QWidget{parent}, _layout(new QVBoxLayout(this)),
			_drone_type_box(new QComboBox()),
			_firmware_upload_button(new QPushButton()),
			_firmware_upload_status_label(new QLabel()),
			_firmware_upload_progress_bar(new QProgressBar()) {
	_layout->setAlignment(Qt::AlignCenter);
	_layout->addWidget(_drone_type_box);
	_layout->addWidget(_firmware_upload_button);
	_layout->addWidget(_firmware_upload_status_label);
	_layout->addWidget(_firmware_upload_progress_bar);

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
	_firmware_upload_button->setVisible(false);

	connect(_drone_type_box, &QComboBox::currentIndexChanged, this,
					&FirmwareUploadPage::handleDroneTypeBoxChange);
	connect(_firmware_upload_button, &QPushButton::pressed, this,
					&FirmwareUploadPage::handleUploadButtonPress);

	_drone_type = DroneType::SingleWing;
	_drone_type_box->addItem(tr("Single wing"),
													 static_cast<int>(DroneType::SingleWing));
	_drone_type_box->addItem(tr("Quadrocopter"),
													 static_cast<int>(DroneType::Quadrocopter));

	_firmware_upload_status_label->setVisible(false);

	_firmware_upload_progress_bar->setRange(0, 255);
	_firmware_upload_progress_bar->setVisible(false);
}

void FirmwareUploadPage::handleSerialConnection() {
	qDebug() << "Serial connected\n";
	_drone_type_box->setVisible(true);
	_firmware_upload_button->setVisible(true);
}

void FirmwareUploadPage::handleSerialDisconnection() {
	qDebug() << "Serial disconnected\n";
	_drone_type_box->setVisible(false);
	_firmware_upload_button->setVisible(false);
}

void FirmwareUploadPage::handleDroneTypeBoxChange(int index) {
	auto drone_type_maybe = _drone_type_box->itemData(index);
	if (drone_type_maybe.canConvert<DroneType>()) {
		_drone_type = drone_type_maybe.value<DroneType>();
		qDebug() << std::format("Drone type: {}\n", static_cast<int>(_drone_type));
	}
}

void FirmwareUploadPage::handleUploadButtonPress() {
	// TODO: add firmware upload and settings check
	emit uploadFirmware(_drone_type);
}

void FirmwareUploadPage::handleFirmwareUploadStateUpdate(
		FirmwareUploadState new_state) {
	qDebug() << "FIRMWARE UPLOAD STATE UPDATED";
	switch (new_state) {
	case FirmwareUploadState::Rebooting: {
		_firmware_upload_status_label->setVisible(true);
		_firmware_upload_status_label->setText(tr("Rebooting..."));
	} break;
	case FirmwareUploadState::Erasing: {
		_firmware_upload_status_label->setVisible(true);
		_firmware_upload_status_label->setText(tr("Erasing..."));
	} break;
	case FirmwareUploadState::Flashing: {
		_firmware_upload_status_label->setVisible(true);
		_firmware_upload_status_label->setText(tr("Flasing..."));
		// _firmware_upload_progress_bar->setVisible(true);
		// _firmware_upload_progress_bar->setValue();
	} break;
	case FirmwareUploadState::None: {
		_firmware_upload_status_label->setVisible(false);
		_firmware_upload_progress_bar->setVisible(false);
	}
	}
}

void FirmwareUploadPage::handleFlashProgressUpdate(uint8_t progress) {
	_firmware_upload_progress_bar->setValue(progress);
}

void FirmwareUploadPage::handleFirmwareUploadComplete(
		FirmwareUploadResult result) {
	switch (result) {
	case FirmwareUploadResult::Ok: {
		_firmware_upload_status_label->setText(tr("Upload succesfully completed"));
	} break;
	default:
		_firmware_upload_status_label->setText(tr("Upload failed"));
		break;
	}
}
