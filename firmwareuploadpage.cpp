#include "firmwareuploadpage.h"

const int MIN_PAGE_WIDTH = 150;
const int MAX_PAGE_WIDTH = 250;

FirmwareUploadPage::FirmwareUploadPage(QWidget *parent)
		: QWidget{parent}, m_layout(new QVBoxLayout(this)),
			m_drone_type_box(new QComboBox()),
			m_firmware_upload_button(new QPushButton()) {
	m_layout->setAlignment(Qt::AlignCenter);
	m_layout->addWidget(m_drone_type_box);
	m_layout->addWidget(m_firmware_upload_button);

	m_drone_type_box->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	m_drone_type_box->setMinimumWidth(MIN_PAGE_WIDTH);
	m_drone_type_box->setMaximumWidth(MAX_PAGE_WIDTH);
	m_drone_type_box->setVisible(false);

	m_firmware_upload_button->setText(tr("Upload firmware"));
	m_firmware_upload_button->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	m_firmware_upload_button->setMinimumWidth(MIN_PAGE_WIDTH);
	m_firmware_upload_button->setMaximumWidth(MAX_PAGE_WIDTH);
	m_firmware_upload_button->setVisible(false);

	connect(m_drone_type_box, &QComboBox::currentIndexChanged, this,
					&FirmwareUploadPage::handleDroneTypeBoxChange);
	connect(m_firmware_upload_button, &QPushButton::pressed, this,
					&FirmwareUploadPage::handleUploadButtonPress);

	m_drone_type_box->addItem(tr("Single wing"), "Singe wing");
	m_drone_type_box->addItem(tr("Quadrocopter"), "Quadrocopter");
}

void FirmwareUploadPage::handleAutopilotConnection() {
	qDebug() << "Autopilot connected\n";
	m_drone_type_box->setVisible(true);
	m_firmware_upload_button->setVisible(true);
}

void FirmwareUploadPage::handleAutopilotDisconnection() {
	qDebug() << "Autopilot disconnected\n";
	m_drone_type_box->setVisible(false);
	m_firmware_upload_button->setVisible(false);
}

void FirmwareUploadPage::handleDroneTypeBoxChange(int index) {
	auto drone_type_maybe = m_drone_type_box->itemData(index);
	if (drone_type_maybe.canConvert<QString>()) {
		m_drone_type = drone_type_maybe.value<QString>();
	}
}

void FirmwareUploadPage::handleUploadButtonPress() {
	qDebug() << "Firmware upload\n" << "Drone type: " << m_drone_type;
	// TODO: add firmware upload and settings check
	emit firmwareUploaded();
}
