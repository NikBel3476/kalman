#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QSerialPort>
#include <mavlink/common/mavlink.h>

QT_BEGIN_NAMESPACE

class QLabel;
class QTimer;

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class Console;
class SettingsDialog;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void openSerialPort();
  void closeSerialPort();
  void about();
  void writeData(const QByteArray &data);
  void readData();

  void handleError(QSerialPort::SerialPortError error);
  void handleBytesWritten(qint64 bytes);
  void handleWriteTimeout();

private:
  void initActionsConnections();

private:
  void showStatusMessage(const QString &message);
  void showWriteError(const QString &message);
  void fillPortsInfo();

  Ui::MainWindow *m_ui = nullptr;
  QToolBar *m_toolbar = nullptr;
  QComboBox *m_ports_box = nullptr;
  QAction *m_action_connect = nullptr;
  QAction *m_action_disconnect = nullptr;
  QAction *m_action_clear= nullptr;
  QLabel *m_status = nullptr;
  Console *m_console = nullptr;
  SettingsDialog *m_settings = nullptr;
  qint64 m_bytesToWrite = 0;
  QTimer *m_timer = nullptr;
  QSerialPort *m_serial = nullptr;
  mavlink_message_t mavlink_message;
  mavlink_status_t mavlink_status;
};

#endif // MAINWINDOW_H
