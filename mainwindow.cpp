#include "mainwindow.h"
#include "console.h"

#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QAction>
#include <QToolBar>
#include <string>
#include <format>

#include <chrono>

static constexpr std::chrono::seconds kWriteTimeout = std::chrono::seconds{5};
static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_toolbar(new QToolBar(this)),
      m_ports_box(new QComboBox(m_toolbar)),
      m_action_connect(new QAction(m_toolbar)),
      m_action_disconnect(new QAction(m_toolbar)),
      m_action_clear(new QAction(m_toolbar)),
      m_status(new QLabel),
      m_console(new Console),
      m_timer(new QTimer(this)), m_serial(new QSerialPort(this)),
      m_mavlink_message{}, m_mavlink_status{},
      m_port_settings{} {
  // m_ui->setupUi(this);

  setWindowTitle("Autopilot selfcheck");
  setGeometry(QRect(0, 0, 800, 600));

  addToolBar(m_toolbar);

  m_toolbar->addWidget(m_ports_box);

  m_action_connect->setIcon(QIcon(":/images/connect.png"));
  m_action_connect->setText("Connect");
  m_action_connect->setToolTip("Connect to serial port");
  m_action_connect->setEnabled(false);

  m_action_disconnect->setIcon(QIcon(":/images/disconnect.png"));
  m_action_disconnect->setText("Disconnect");
  m_action_disconnect->setToolTip("Disconnect from serial port");
  m_action_disconnect->setEnabled(false);

  m_action_clear->setIcon(QIcon(":/images/clear.png"));
  m_action_clear->setText("Clear");
  m_action_clear->setToolTip("Clear terminal");
  m_action_clear->setEnabled(true);

  m_toolbar->addAction(m_action_connect);
  m_toolbar->addAction(m_action_disconnect);
  m_toolbar->addAction(m_action_clear);

  m_console->setEnabled(false);
  setCentralWidget(m_console);

  // m_ui->statusBar->addWidget(m_status);

  initActionsConnections();
  initSerialPortEventsConnections();
  initPortsBoxEventsConnections();

  connect(m_timer, &QTimer::timeout, this, &MainWindow::handleWriteTimeout);
  m_timer->setSingleShot(true);

  fillPortsInfo();
}

MainWindow::~MainWindow() = default;

void MainWindow::openSerialPort() {
  const auto p = m_port_settings;
  m_serial->setPortName(p.name);
  m_serial->setBaudRate(p.baudRate);
  m_serial->setDataBits(p.dataBits);
  m_serial->setParity(p.parity);
  m_serial->setStopBits(p.stopBits);
  m_serial->setFlowControl(p.flowControl);

  if (m_serial->open(QIODevice::ReadWrite)) {
    m_console->setEnabled(true);
    // m_console->setLocalEchoEnabled(p.localEchoEnabled);
    m_action_connect->setEnabled(false);
    m_action_disconnect->setEnabled(true);
  } else {
    QMessageBox::critical(this, tr("Error"), m_serial->errorString());

    showStatusMessage(tr("Open error"));
  }
}

void MainWindow::closeSerialPort() {
  if (m_serial->isOpen())
    m_serial->close();
  m_console->setEnabled(false);
  m_action_connect->setEnabled(true);
  m_action_disconnect->setEnabled(false);
  // m_ui->actionConfigure->setEnabled(true);
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
    if (mavlink_parse_char(MAVLINK_COMM_0, byte, &m_mavlink_message,
                           &m_mavlink_status)) {
      const auto msg =
          std::format("ID: {} sequence: {} from component: {} of system: {}\n",
                      std::to_string(m_mavlink_message.msgid),
                      std::to_string(m_mavlink_message.seq),
                      std::to_string(m_mavlink_message.compid),
                      std::to_string(m_mavlink_message.sysid));
      QByteArray data(msg.c_str(), static_cast<uint32_t>(msg.length()));
      m_console->putData(data);

      switch (m_mavlink_message.msgid) {
      case MAVLINK_MSG_ID_SYS_STATUS: {
        mavlink_sys_status_t sys_status;
        mavlink_msg_sys_status_decode(&m_mavlink_message, &sys_status);
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
        mavlink_msg_global_position_int_decode(&m_mavlink_message,
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
  connect(m_action_connect, &QAction::triggered, this,
          &MainWindow::openSerialPort);
  connect(m_action_disconnect, &QAction::triggered, this,
          &MainWindow::closeSerialPort);
  connect(m_action_clear, &QAction::triggered, m_console, &Console::clear);
}

void MainWindow::initSerialPortEventsConnections() {
  connect(m_serial, &QSerialPort::errorOccurred, this,
          &MainWindow::handleError);
  connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
  connect(m_serial, &QSerialPort::bytesWritten, this,
          &MainWindow::handleBytesWritten);
  connect(m_console, &Console::getData, this, &MainWindow::writeData);
}

void MainWindow::initPortsBoxEventsConnections() {
  connect(m_ports_box, &QComboBox::currentIndexChanged, this, &MainWindow::setPortSettings);
}

void MainWindow::setPortSettings(int idx) {
  auto portSettingsMaybe = m_ports_box->itemData(idx);
  if (portSettingsMaybe.canConvert<QStringList>()) {
    auto port_settings_list = portSettingsMaybe.value<QStringList>();
    m_port_settings = Settings {
      .name = port_settings_list[0],
      .baudRate = 57600,
      .dataBits = QSerialPort::Data8,
      .parity = QSerialPort::NoParity,
      .stopBits = QSerialPort::OneStop,
      .flowControl = QSerialPort::NoFlowControl
    };
    m_action_connect->setEnabled(true);
  }
}

void MainWindow::showStatusMessage(const QString &message) {
  m_status->setText(message);
}

void MainWindow::showWriteError(const QString &message) {
  QMessageBox::warning(this, tr("Warning"), message);
}

void MainWindow::fillPortsInfo() {
  m_ports_box->clear();
  const QString blankString = tr(::blankString);
  const auto infos = QSerialPortInfo::availablePorts();

  for (const QSerialPortInfo &info : infos) {
          QStringList list;
          const QString description = info.description();
          const QString manufacturer = info.manufacturer();
          const QString serialNumber = info.serialNumber();
          const auto vendorId = info.vendorIdentifier();
          const auto productId = info.productIdentifier();
          list << info.systemLocation()
                  << (!description.isEmpty() ? description : blankString)
                  << (!manufacturer.isEmpty() ? manufacturer : blankString)
                  << (!serialNumber.isEmpty() ? serialNumber : blankString)
                  << info.systemLocation()
                  << (vendorId ? QString::number(vendorId, 16) : blankString)
                  << (productId ? QString::number(productId, 16) : blankString);

          m_ports_box->addItem(list.constFirst(), list);
  }

  m_ports_box->addItem(tr("Custom"));
}
