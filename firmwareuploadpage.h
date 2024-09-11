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

public slots:
  void handleAutopilotConnection();
  void handleAutopilotDisconnection();
  void handleDroneTypeBoxChange(int index);
  void handleUploadButtonPress();

private:
  QVBoxLayout *m_layout = nullptr;
  QComboBox *m_drone_type_box = nullptr;
  QPushButton *m_firmware_upload_button = nullptr;

  QString m_drone_type;
};

#endif // FIRMWAREUPLOADPAGE_H
