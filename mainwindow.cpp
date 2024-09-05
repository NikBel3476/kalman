#include "mainwindow.h"
#include "console.h"
#include "settingsdialog.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QMessageBox>
#include <QTimer>

#include <chrono>

static constexpr std::chrono::seconds kWriteTimeout = std::chrono::seconds{5};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_ui(new Ui::MainWindow), m_status(new QLabel),
      m_console(new Console), m_settings(new SettingsDialog(this)),
      m_timer(new QTimer(this)), m_serial(new QSerialPort(this)),
      mavlink_message{}, mavlink_status{} {
  m_ui->setupUi(this);
  m_console->setEnabled(false);
  setCentralWidget(m_console);

  m_ui->actionConnect->setEnabled(true);
  m_ui->actionDisconnect->setEnabled(false);
  m_ui->actionQuit->setEnabled(true);
  m_ui->actionConfigure->setEnabled(true);

  m_ui->statusBar->addWidget(m_status);

  initActionsConnections();

  connect(m_serial, &QSerialPort::errorOccurred, this,
          &MainWindow::handleError);
  connect(m_timer, &QTimer::timeout, this, &MainWindow::handleWriteTimeout);
  m_timer->setSingleShot(true);

  connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
  connect(m_serial, &QSerialPort::bytesWritten, this,
          &MainWindow::handleBytesWritten);
  connect(m_console, &Console::getData, this, &MainWindow::writeData);
}

MainWindow::~MainWindow() {
  delete m_settings;
  delete m_ui;
}

void MainWindow::openSerialPort() {
  const SettingsDialog::Settings p = m_settings->settings();
  m_serial->setPortName(p.name);
  m_serial->setBaudRate(p.baudRate);
  m_serial->setDataBits(p.dataBits);
  m_serial->setParity(p.parity);
  m_serial->setStopBits(p.stopBits);
  m_serial->setFlowControl(p.flowControl);

  if (m_serial->open(QIODevice::ReadWrite)) {
    m_console->setEnabled(true);
    m_console->setLocalEchoEnabled(p.localEchoEnabled);
    m_ui->actionConnect->setEnabled(false);
    m_ui->actionDisconnect->setEnabled(true);
    m_ui->actionConfigure->setEnabled(false);
    showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name, p.stringBaudRate, p.stringDataBits,
                               p.stringParity, p.stringStopBits,
                               p.stringFlowControl));
  } else {
    QMessageBox::critical(this, tr("Error"), m_serial->errorString());

    showStatusMessage(tr("Open error"));
  }
}

void MainWindow::closeSerialPort() {
  if (m_serial->isOpen())
    m_serial->close();
  m_console->setEnabled(false);
  m_ui->actionConnect->setEnabled(true);
  m_ui->actionDisconnect->setEnabled(false);
  m_ui->actionConfigure->setEnabled(true);
  showStatusMessage(tr("Disconnected"));
}

void MainWindow::about() {
  QMessageBox::about(this, tr("About Autopilot selfcheck"),
                     tr("The <b>Autopilot selfcheck</b> intended to automatic "
                        "checking of autopilot status and its peripherals."));
}

void MainWindow::writeData(const QByteArray &data) {
  const qint64 written = m_serial->write(data);
  if (written == data.size()) {
    m_bytesToWrite += written;
    m_timer->start(kWriteTimeout);
  } else {
    const auto error = tr("Failed to write all data to port %1.\n"
                          "Error: %2")
                           .arg(m_serial->portName(), m_serial->errorString());
    showWriteError(error);
  }
}

void MainWindow::readData() {
  const auto data = m_serial->readAll();
  for (auto byte : data) {
    if (mavlink_parse_char(MAVLINK_COMM_0, byte, &mavlink_message,
                           &mavlink_status)) {
      const auto msg =
          std::format("ID: {} sequence: {} from component: {} of system: {}\n",
                      std::to_string(mavlink_message.msgid),
                      std::to_string(mavlink_message.seq),
                      std::to_string(mavlink_message.compid),
                      std::to_string(mavlink_message.sysid));
      QByteArray data(msg.c_str(), static_cast<uint32_t>(msg.length()));
      m_console->putData(data);

      switch (mavlink_message.msgid) {
      case MAVLINK_MSG_ID_SYS_STATUS: {
        mavlink_sys_status_t sys_status;
        mavlink_msg_sys_status_decode(&mavlink_message, &sys_status);
        const auto status_str = std::format(
            "sensors present: {} sensors enabled: {} load: {} errors_comm: "
            "{}\n",
            std::to_string(sys_status.onboard_control_sensors_present),
            std::to_string(sys_status.onboard_control_sensors_enabled),
            std::to_string(sys_status.voltage_battery),
            std::to_string(sys_status.errors_comm));
        QByteArray data(status_str.c_str(),
                        static_cast<uint32_t>(status_str.length()));
        m_console->putData(data);
      } break;
      case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
        mavlink_global_position_int_t global_position;
        mavlink_msg_global_position_int_decode(&mavlink_message,
                                               &global_position);
        const auto coords_str =
            std::format("lon: {} lat: {} alt: {}\n", global_position.lon,
                        global_position.lat, global_position.alt);
        QByteArray data(coords_str.c_str(),
                        static_cast<uint32_t>(coords_str.length()));
        m_console->putData(data);
      } break;
      default:
        break;
      }
    }
  }
}

void MainWindow::handleError(QSerialPort::SerialPortError error) {
  if (error == QSerialPort::ResourceError) {
    QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
    closeSerialPort();
  }
}

void MainWindow::handleBytesWritten(qint64 bytes) {
  m_bytesToWrite -= bytes;
  if (m_bytesToWrite == 0)
    m_timer->stop();
}

void MainWindow::handleWriteTimeout() {
  const QString error = tr("Write operation timed out for port %1.\n"
                           "Error: %2")
                            .arg(m_serial->portName(), m_serial->errorString());
  showWriteError(error);
}

void MainWindow::initActionsConnections() {
  connect(m_ui->actionConnect, &QAction::triggered, this,
          &MainWindow::openSerialPort);
  connect(m_ui->actionDisconnect, &QAction::triggered, this,
          &MainWindow::closeSerialPort);
  connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
  connect(m_ui->actionConfigure, &QAction::triggered, m_settings,
          &SettingsDialog::show);
  connect(m_ui->actionClear, &QAction::triggered, m_console, &Console::clear);
  connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
  connect(m_ui->actionAboutQt, &QAction::triggered, qApp,
          &QApplication::aboutQt);
}

void MainWindow::showStatusMessage(const QString &message) {
  m_status->setText(message);
}

void MainWindow::showWriteError(const QString &message) {
  QMessageBox::warning(this, tr("Warning"), message);
}
