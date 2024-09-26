#ifndef FIRMWAREUPLOADPAGE_H
#define FIRMWAREUPLOADPAGE_H

#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class FirmwareUploadPage : public QWidget {
	Q_OBJECT
public:
	explicit FirmwareUploadPage(QWidget *parent = nullptr);

signals:
	void firmwareUploaded();
	void goToSettingsPage();

public slots:
	void handleSerialConnection();
	void handleSerialDisconnection();
	void handleDroneTypeBoxChange(int index);
	void handleUploadButtonPress();

private slots:
	void handleSettingsButtonPress();

private:
	QVBoxLayout *m_layout = nullptr;
	QComboBox *m_drone_type_box = nullptr;
	QPushButton *m_firmware_upload_button = nullptr;

	QString m_drone_type;
};

#endif // FIRMWAREUPLOADPAGE_H
