#ifndef FIRMWAREUPLOADPAGE_H
#define FIRMWAREUPLOADPAGE_H

#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

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

private:
	QVBoxLayout *_layout = nullptr;
	QComboBox *_drone_type_box = nullptr;
	QPushButton *_firmware_upload_button = nullptr;

	DroneType _drone_type;
};

#endif // FIRMWAREUPLOADPAGE_H
