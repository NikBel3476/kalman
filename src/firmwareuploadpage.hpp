#ifndef FIRMWAREUPLOADPAGE_H
#define FIRMWAREUPLOADPAGE_H

#include <QComboBox>
#include <QFileDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include "firmwareuploader.hpp"

enum class DroneType {
	SingleWing,
	Quadrocopter
};

class FirmwareUploadPage : public QWidget {
	Q_OBJECT
public:
	explicit FirmwareUploadPage(QWidget *parent,
															FirmwareUploader *firmware_uploader);

signals:
	void uploadFirmwareStarted(DroneType);

public slots:
	void handleSerialConnection();
	void handleSerialDisconnection();

private slots:
	void _handleDroneTypeBoxChange(int index);
	void _handleUploadButtonPress();
	void _handleFirmwareUploadStateUpdate(FirmwareUploadState new_state);
	void _handleFlashProgressUpdate(uint8_t progress);
	void _handleEraseProgressUpdate(uint8_t progress);
	void _handleFirmwareUploadCompletion(FirmwareUploadResult);

private:
	QVBoxLayout *_layout = nullptr;
	QComboBox *_drone_type_box = nullptr;
	QPushButton *_firmware_upload_button = nullptr;
	QLabel *_firmware_upload_status_label = nullptr;
	QProgressBar *_progress_bar = nullptr;

	DroneType _drone_type;
	FirmwareUploader *_firmware_uploader = nullptr;
};

#endif // FIRMWAREUPLOADPAGE_H
