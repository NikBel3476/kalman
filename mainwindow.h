#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QSerialPort>


#include <mavlink/common/mavlink.h>

QT_BEGIN_NAMESPACE

class QLabel;
class QTimer;

QT_END_NAMESPACE

class Console;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  struct Settings {
    QString name;
    qint32 baudRate;
    QSerialPort::DataBits dataBits;
    QSerialPort::Parity parity;
    QSerialPort::StopBits stopBits;
    QSerialPort::FlowControl flowControl;
  };

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
  void initSerialPortEventsConnections();

private:
  void showStatusMessage(const QString &message);
  void showWriteError(const QString &message);
  void fillPortsInfo();
  void setPortSettings(int idx);
  void initPortsBoxEventsConnections();

  QToolBar *m_toolbar = nullptr;
  QComboBox *m_ports_box = nullptr;
  QAction *m_action_connect = nullptr;
  QAction *m_action_disconnect = nullptr;
  QAction *m_action_clear= nullptr;
  QLabel *m_status = nullptr;
  Console *m_console = nullptr;
  qint64 m_bytesToWrite = 0;
  QTimer *m_timer = nullptr;
  QSerialPort *m_serial = nullptr;
  mavlink_message_t m_mavlink_message;
  mavlink_status_t m_mavlink_status;
  Settings m_port_settings;
};

#endif // MAINWINDOW_H
