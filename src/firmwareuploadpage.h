#ifndef FIRMWAREUPLOADPAGE_H
#define FIRMWAREUPLOADPAGE_H

#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include "firmwareuploader.h"

enum class DroneType { SingleWing, Quadrocopter };

class FirmwareUploadPage : public QWidget {
	Q_OBJECT
public:
	explicit FirmwareUploadPage(QWidget *parent = nullptr);

signals:
	void uploadFirmware(DroneType);

public slots:
	void handleSerialConnection();
	void handleSerialDisconnection();
	void handleDroneTypeBoxChange(int index);
	void handleUploadButtonPress();
	void handleFirmwareUploadStateUpdate(FirmwareUploadState new_state);
	void handleFlashProgressUpdate(uint8_t progress);
	void handleFirmwareUploadComplete(FirmwareUploadResult);

private:
	QVBoxLayout *_layout = nullptr;
	QComboBox *_drone_type_box = nullptr;
	QPushButton *_firmware_upload_button = nullptr;
	QLabel *_firmware_upload_status_label = nullptr;
	QProgressBar *_firmware_upload_progress_bar = nullptr;

	DroneType _drone_type;
};

#endif // FIRMWAREUPLOADPAGE_H
