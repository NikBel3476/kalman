#include "mainwindow.h"
#include "console.h"

#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <format>
#include <string>

#include <chrono>

static constexpr std::chrono::seconds kWriteTimeout = std::chrono::seconds{5};
static constexpr std::chrono::seconds kHeartbeatTimeout =
    std::chrono::seconds{5};
static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");

static const uint8_t SYSTEM_ID = 255;
static const uint8_t COMP_ID = MAV_COMP_ID_MISSIONPLANNER;
static const uint8_t TARGET_SYSTEM_ID = 1;
static const uint8_t TARGET_COMP_ID = MAV_COMP_ID_AUTOPILOT1;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_toolbar(new QToolBar(this)),
      m_ports_box(new QComboBox(m_toolbar)),
      m_action_connect(new QAction(m_toolbar)),
      m_action_disconnect(new QAction(m_toolbar)),
      m_action_clear(new QAction(m_toolbar)), m_statusbar(new QStatusBar(this)),
      m_status(new QLabel), m_console(new Console),
      m_central_widget(new QStackedWidget()),
      m_authentication_form(new AuthenticationForm()),
      m_firmware_upload_page(new FirmwareUploadPage()),
      m_autopilot_settings_page(new AutopilotSettingsPage()),
      // m_qml_view(new QQuickView(QUrl("qrc:/AuthenticationForm.qml"))),
      // m_qml_container(QWidget::createWindowContainer(m_qml_view, this)),
      m_timer(new QTimer(this)), m_serial(new QSerialPort(this)),
      m_mavlink_message{}, m_mavlink_status{}, m_port_settings{},
      m_heartbeat_timer(new QTimer(this)), _msg_cal_box(new QMessageBox(this)) {
  setWindowTitle("Autopilot selfcheck");
  setGeometry(QRect(0, 0, 800, 600));

  // Main window content
  addToolBar(m_toolbar);
  setCentralWidget(m_central_widget);
  setStatusBar(m_statusbar);

  // toolbar content
  m_toolbar->setMovable(false);
  m_toolbar->addWidget(m_ports_box);
  m_toolbar->setVisible(false);

  m_toolbar->addAction(m_action_connect);
  m_toolbar->addAction(m_action_disconnect);
  m_toolbar->addAction(m_action_clear);

  m_action_connect->setIcon(QIcon(":/images/connect.png"));
  m_action_connect->setText(tr("Connect"));
  m_action_connect->setToolTip(tr("Connect"));
  m_action_connect->setEnabled(false);

  m_action_disconnect->setIcon(QIcon(":/images/disconnect.png"));
  m_action_disconnect->setText(tr("Disconnect"));
  m_action_disconnect->setToolTip(tr("Disconnect"));
  m_action_disconnect->setEnabled(false);

  m_action_clear->setIcon(QIcon(":/images/clear.png"));
  m_action_clear->setText(tr("Clear"));
  m_action_clear->setToolTip(tr("Clear"));
  m_action_clear->setEnabled(true);

  // central widget content
  m_central_widget->addWidget(m_authentication_form);
  m_central_widget->addWidget(m_firmware_upload_page);
  // m_central_widget->addWidget(m_console);
  m_central_widget->addWidget(m_autopilot_settings_page);

  m_central_widget->setCurrentWidget(m_authentication_form);

  // m_central_widget_layout->addWidget(m_authentication_form);
  // m_central_widget_layout->addWidget(m_console);
  // m_central_widget_layout->addWidget(m_qml_container);

  // m_qml_container->setFocusPolicy(Qt::TabFocus);

  m_console->setEnabled(false);

  _msg_cal_box->setWindowTitle(tr("Calibration"));

  _msg_cal_box_button = _msg_cal_box->addButton(QMessageBox::Ok);

  // status bar content
  m_statusbar->addWidget(m_status);
  m_statusbar->setVisible(false);
  m_status->setText(tr("Disconnected"));

  m_timer->setSingleShot(true);

  initActionsConnections();
  initSerialPortEventsConnections();
  initPortsBoxEventsConnections();

  connect(_msg_cal_box_button, &QPushButton::clicked, this,
          &MainWindow::handleCalibrationDialogButton);

  connect(m_timer, &QTimer::timeout, this, &MainWindow::handleWriteTimeout);

  // authentiation form connections
  connect(m_authentication_form, &AuthenticationForm::login, this,
          &MainWindow::handleLogin);

  // firmware upload page connections
  connect(this, &MainWindow::autopilotConnected, m_firmware_upload_page,
          &FirmwareUploadPage::handleAutopilotConnection);
  connect(this, &MainWindow::autopilotDisconnected, m_firmware_upload_page,
          &FirmwareUploadPage::handleAutopilotDisconnection);
  connect(m_firmware_upload_page, &FirmwareUploadPage::firmwareUploaded, this,
          &MainWindow::handleFirmwareUpload);

  // autopilot settings page connections
  connect(this, &MainWindow::IMUUpdated, m_autopilot_settings_page,
          &AutopilotSettingsPage::handleIMUUpdate);
  connect(this, &MainWindow::powerStatusUpdated, m_autopilot_settings_page,
          &AutopilotSettingsPage::handlePowerStatusUpdate);
  connect(this, &MainWindow::mcuStatusUpdated, m_autopilot_settings_page,
          &AutopilotSettingsPage::handleMcuStatusUpdate);
  connect(m_autopilot_settings_page, &AutopilotSettingsPage::startCalibration,
          this, &MainWindow::handleStartAccelCalibration);
  connect(m_autopilot_settings_page, &AutopilotSettingsPage::cancelCalibration,
          this, &MainWindow::handleCancelAccelCalibration);

  connect(m_heartbeat_timer, &QTimer::timeout, this,
          &MainWindow::handleHeartbeatTimeout);

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
    m_status->setText(tr("Connected to %1").arg(m_serial->portName()));
    m_heartbeat_timer->start(kHeartbeatTimeout);
    emit autopilotConnected();
  } else {
    QMessageBox::critical(this, tr("Error"), m_serial->errorString());
    showStatusMessage(tr("Open error"));
  }
}

void MainWindow::closeSerialPort() {
  if (m_serial->isOpen())
    m_serial->close();
  // m_console->setEnabled(false);
  m_action_connect->setEnabled(true);
  m_action_disconnect->setEnabled(false);
  showStatusMessage(tr("Disconnected"));
  m_central_widget->setCurrentWidget(m_firmware_upload_page);
  m_heartbeat_timer->stop();
  reset();
  emit autopilotDisconnected();
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
      case MAVLINK_MSG_ID_HEARTBEAT: {
        mavlink_heartbeat_t heartbeat;
        mavlink_msg_heartbeat_decode(&m_mavlink_message, &heartbeat);
        m_heartbeat_timer->start(kHeartbeatTimeout);
      } break;
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
      case MAVLINK_MSG_ID_POWER_STATUS: {
        mavlink_power_status_t power_status;
        mavlink_msg_power_status_decode(&m_mavlink_message, &power_status);
        const auto status_str =
            std::format("Rail voltage: {} mV\n", power_status.Vcc);
        QByteArray data(status_str.c_str(),
                        static_cast<uint32_t>(status_str.length()));
        m_console->putData(data);
        emit powerStatusUpdated(power_status);
      } break;
      case MAVLINK_MSG_ID_RAW_IMU: {
        mavlink_raw_imu_t raw_imu;
        mavlink_msg_raw_imu_decode(&m_mavlink_message, &raw_imu);
        const auto raw_imu_str = std::format(
            "Time: {} xacc: {} yacc: {} zacc: {} xgyro: {} ygyro: {} zgyro: {} "
            "xmag: {} ymag: {} zmag: {} id: {}\n",
            std::to_string(raw_imu.time_usec), std::to_string(raw_imu.xacc),
            std::to_string(raw_imu.yacc), std::to_string(raw_imu.zacc),
            std::to_string(raw_imu.xgyro), std::to_string(raw_imu.ygyro),
            std::to_string(raw_imu.zgyro), std::to_string(raw_imu.xmag),
            std::to_string(raw_imu.ymag), std::to_string(raw_imu.zmag),
            std::to_string(raw_imu.id));
        QByteArray data(raw_imu_str.c_str(),
                        static_cast<uint32_t>(raw_imu_str.length()));
        m_console->putData(data);
        emit IMUUpdated(raw_imu);
      } break;
      case MAVLINK_MSG_ID_MCU_STATUS: {
        mavlink_mcu_status_t mcu_status;
        mavlink_msg_mcu_status_decode(&m_mavlink_message, &mcu_status);
        emit mcuStatusUpdated(mcu_status);
      } break;
      case MAVLINK_MSG_ID_COMMAND_ACK: {
        mavlink_command_ack_t cmd_ack;
        mavlink_msg_command_ack_decode(&m_mavlink_message, &cmd_ack);
        auto cmd_ack_str =
            std::format("COMMAND: {} RESULT: {} PROGRESS: {} RES_PRM2: {} "
                        "TARGET_SYS: {} TARGET_CMP: {}\n",
                        cmd_ack.command, cmd_ack.result, cmd_ack.progress,
                        cmd_ack.result_param2, cmd_ack.target_system,
                        cmd_ack.target_component);
        QByteArray data(cmd_ack_str.c_str(),
                        static_cast<uint32_t>(cmd_ack_str.length()));
        m_console->putData(data);
        // _handleCommandAck(cmd_ack);
        qDebug() << cmd_ack_str << '\n';
      } break;
      case MAVLINK_MSG_ID_COMMAND_LONG: {
        // const auto cmd = std::make_unique<mavlink_command_long_t>();
        mavlink_command_long_t cmd;
        mavlink_msg_command_long_decode(&m_mavlink_message, &cmd);
        auto cmd_str =
            std::format("COMMAND: {} SYSTEM: {} COMPONENT: {} CONFIMATION: {} "
                        "param1: {} param2: {} param3: {} param4: {} param5: "
                        "{} param6: {} param7: {}\n",
                        cmd.command, cmd.target_system, cmd.target_component,
                        cmd.confirmation, cmd.param1, cmd.param2, cmd.param3,
                        cmd.param4, cmd.param5, cmd.param6, cmd.param7);
        QByteArray data(cmd_str.c_str(),
                        static_cast<uint32_t>(cmd_str.length()));
        m_console->putData(data);
        qDebug() << cmd_str << '\n';
        parseCommand(cmd);
      } break;
      case MAVLINK_MSG_ID_PARAM_VALUE: {
        mavlink_param_value_t param_value;
        mavlink_msg_param_value_decode(&m_mavlink_message, &param_value);
        auto param_value_str =
            std::format("ID: {} VALUE: {} TYPE: {} COUNT: {} INDEX: {}\n",
                        param_value.param_id, param_value.param_value,
                        param_value.param_type, param_value.param_count,
                        param_value.param_index);
        QByteArray data(param_value_str.c_str(),
                        static_cast<uint32_t>(param_value_str.length()));
        m_console->putData(data);
      } break;
      case MAVLINK_MSG_ID_STATUSTEXT: {
        mavlink_statustext_t statustext;
        mavlink_msg_statustext_decode(&m_mavlink_message, &statustext);
        auto statustext_str =
            std::format("ID: {} CHUNK_SEQ: {} SEVERITY: {} TEXT: {}\n",
                        std::to_string(statustext.id), statustext.chunk_seq,
                        statustext.severity, statustext.text);
        QByteArray data(statustext_str.c_str(),
                        static_cast<uint32_t>(statustext_str.length()));
        m_console->putData(data);
        qDebug() << statustext_str << '\n';
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
    if (m_serial->errorString() != "Resource temporarily unavailable") {
      closeSerialPort();
    }
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

void MainWindow::handleLogin(const QString &username, const QString &password) {
  qDebug() << "Login event\n" << username << '\n' << password << '\n';
  // TODO: add authentication
  m_central_widget->setCurrentWidget(m_firmware_upload_page);
  m_toolbar->setVisible(true);
  m_statusbar->setVisible(true);
}

void MainWindow::handleFirmwareUpload() {
  m_central_widget->setCurrentWidget(m_autopilot_settings_page);
  m_console->show();
}

void MainWindow::handleHeartbeatTimeout() {
  QMessageBox::critical(this, tr("Error"), tr("Heartbeat error"));
  closeSerialPort();
}

void MainWindow::handleStartAccelCalibration() {
  mavlink_message_t msg;
  const auto command = MAV_CMD_PREFLIGHT_CALIBRATION;
  const auto confirmation = 0;
  const float param1 = 0;
  const float param2 = 0;
  const float param3 = 0;
  const float param4 = 0;
  const float param5 = 1; // accelerometer calibration
  const float param6 = 0;
  const float param7 = 0;

  const auto msg_size = mavlink_msg_command_long_pack(
      SYSTEM_ID, COMP_ID, &msg, TARGET_SYSTEM_ID, TARGET_COMP_ID, command,
      confirmation, param1, param2, param3, param4, param5, param6, param7);

  // auto buf_len = MAVLINK_MAX_PACKET_LEN;
  uint8_t buf[msg_size];
  const auto buf_size = mavlink_msg_to_send_buffer(buf, &msg);
  QByteArray data((char *)buf, buf_size);
  writeData(data);
  _cal_state = CalibrationState::InProgress;
  qDebug("Calibration started\n");
}

void MainWindow::handleCancelAccelCalibration() {
  mavlink_message_t msg;
  auto command = MAV_CMD_PREFLIGHT_CALIBRATION;

  mavlink_msg_command_long_pack(SYSTEM_ID, COMP_ID, &msg, TARGET_SYSTEM_ID,
                                TARGET_COMP_ID, command, 0, 0, 0, 0, 0, 0, 0,
                                0);

  auto buf_len = MAVLINK_MAX_PACKET_LEN;
  uint8_t buf[buf_len];
  mavlink_msg_to_send_buffer(buf, &msg);
  QByteArray data((char *)buf, buf_len);
  writeData(data);
  qDebug("Calibration cancelled\n");
  _cal_state = CalibrationState::None;
}

void MainWindow::handleCalibrationDialogButton() {
  qDebug() << "CLICKED!!!" << "Calibration state: " << (int)_cal_state
           << "Cal_acc_state: " << (int)_cal_accel_state << '\n';
  if (_cal_state == CalibrationState::None) {
    return;
  }
  float vehicle_position_param;
  switch (_cal_accel_state) {
  case CalibrationAccelState::Level: {
    _cal_accel_state = CalibrationAccelState::LeftSide;
    vehicle_position_param = ACCELCAL_VEHICLE_POS_LEVEL;
    qDebug() << "LEVEL REQUEST\n";
  } break;
  case CalibrationAccelState::LeftSide: {
    vehicle_position_param = ACCELCAL_VEHICLE_POS_LEFT;
    qDebug() << "LEFT REQUEST\n";
  } break;
  case CalibrationAccelState::RightSide: {
    vehicle_position_param = ACCELCAL_VEHICLE_POS_RIGHT;
    qDebug() << "RIGHT REQUEST\n";
  } break;
  case CalibrationAccelState::NoseUp: {
    vehicle_position_param = ACCELCAL_VEHICLE_POS_NOSEUP;
    qDebug() << "NOSEUP REQUEST\n";
  } break;
  case CalibrationAccelState::NoseDown: {
    vehicle_position_param = ACCELCAL_VEHICLE_POS_NOSEDOWN;
    qDebug() << "NOSEDOWN REQUEST\n";
  } break;
  case CalibrationAccelState::Back: {
    vehicle_position_param = ACCELCAL_VEHICLE_POS_BACK;
    qDebug() << "BACK REQUEST\n";
  } break;
  case CalibrationAccelState::None:
  default:
    qDebug() << "END REQUEST\n";
    _msg_cal_box->close();
    return;
  }
  mavlink_message_t msg;
  auto msg_len = mavlink_msg_command_long_pack(
      SYSTEM_ID, COMP_ID, &msg, TARGET_SYSTEM_ID, TARGET_COMP_ID,
      MAV_CMD_ACCELCAL_VEHICLE_POS, 0, vehicle_position_param, 0, 0, 0, 0, 0,
      0);
  // auto buf_len = MAVLINK_MAX_PACKET_LEN;
  // mavlink_msg_command_ack_pack(SYSTEM_ID, COMP_ID, &msg, 0, 1, 0, 0,
  // TARGET_SYSTEM_ID, TARGET_COMP_ID); const auto buf_len =
  // MAVLINK_MAX_PACKET_LEN; const auto msg_len =
  // mavlink_msg_command_ack_pack(SYSTEM_ID, COMP_ID, &msg, 0,
  // MAV_RESULT_TEMPORARILY_REJECTED, 0, 0, 0, 0);
  uint8_t buf[msg_len];
  const auto buf_len = mavlink_msg_to_send_buffer(buf, &msg);
  QByteArray data((char *)buf, buf_len);
  writeData(data);
}

void MainWindow::_handleCommandAck(mavlink_command_ack_t &cmd) {
  switch (cmd.command) {
  case MAV_CMD_PREFLIGHT_CALIBRATION: {
    switch (cmd.result) {
    case MAV_RESULT_ACCEPTED: {
      _cal_state = CalibrationState::InProgress;
      _cal_accel_state = CalibrationAccelState::Level;
      _msg_cal_box->setText(
          tr("Place vehicle in level position and then press OK"));
      _msg_cal_box->show();
    } break;
    default:
      break;
    }
  } break;
  case MAV_CMD_ACCELCAL_VEHICLE_POS: {
    switch (cmd.result) {
    case MAV_RESULT_ACCEPTED: {
      switch (_cal_accel_state) {
      case CalibrationAccelState::None: {
        _cal_state = CalibrationState::InProgress;
        _cal_accel_state = CalibrationAccelState::Level;
        _msg_cal_box->setText(
            tr("Place vehicle in level position and then press OK"));
        _msg_cal_box->show();
      }
      case CalibrationAccelState::Level: {
        _cal_accel_state = CalibrationAccelState::LeftSide;
        _msg_cal_box->setText(
            tr("Place vehicle on the left side and then press OK"));
        _msg_cal_box->show();
      } break;
      case CalibrationAccelState::LeftSide: {
        _cal_accel_state = CalibrationAccelState::RightSide;
        _msg_cal_box->setText(
            tr("Place vehicle on the right side and then press OK"));
        _msg_cal_box->show();
      } break;
      case CalibrationAccelState::RightSide: {
        _cal_accel_state = CalibrationAccelState::NoseUp;
        _msg_cal_box->setText(
            tr("Place vehicle in noseup position and then press OK"));
        _msg_cal_box->show();
      } break;
      case CalibrationAccelState::NoseUp: {
        _cal_accel_state = CalibrationAccelState::NoseDown;
        _msg_cal_box->setText(
            tr("Place vehicle in nosedown position and then press OK"));
        _msg_cal_box->show();
      } break;
      case CalibrationAccelState::NoseDown: {
        _cal_accel_state = CalibrationAccelState::Back;
        _msg_cal_box->setText(
            tr("Place vehicle on the back side and then press OK"));
        _msg_cal_box->show();
      } break;
      case CalibrationAccelState::Back:
      default:
        break;
      }
    } break;
    case MAV_RESULT_FAILED: {
      _cal_state = CalibrationState::None;
      _cal_accel_state = CalibrationAccelState::None;
      _msg_cal_box->setText("Calibration failed");
      _msg_cal_box->show();
    } break;
    default:
      break;
    }
  } break;
  default:
    break;
  }
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
  connect(m_ports_box, &QComboBox::currentIndexChanged, this,
          &MainWindow::setPortSettings);
}

void MainWindow::setPortSettings(int index) {
  auto portSettingsMaybe = m_ports_box->itemData(index);
  if (portSettingsMaybe.canConvert<QStringList>()) {
    auto port_settings_list = portSettingsMaybe.value<QStringList>();
    m_port_settings = Settings{.name = port_settings_list[0],
                               .baudRate = 57600,
                               .dataBits = QSerialPort::Data8,
                               .parity = QSerialPort::NoParity,
                               .stopBits = QSerialPort::OneStop,
                               .flowControl = QSerialPort::NoFlowControl};
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
    list << info.portName()
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

void MainWindow::parseCommand(const mavlink_command_long_t &cmd) {
  switch (cmd.command) {
  case MAV_CMD_ACCELCAL_VEHICLE_POS: {
    if (_cal_state == CalibrationState::InProgress) {
      switch (static_cast<ACCELCAL_VEHICLE_POS>(static_cast<int>(cmd.param1))) {
      case ACCELCAL_VEHICLE_POS_LEVEL: {
        _cal_accel_state = CalibrationAccelState::Level;
        _msg_cal_box->setText(
            tr("Place vehicle in level position and then press OK"));
        // _msg_cal_box->show();
        qDebug() << "LEVEL RESPONSE\n";
      } break;
      case ACCELCAL_VEHICLE_POS_LEFT: {
        _cal_accel_state = CalibrationAccelState::LeftSide;
        _msg_cal_box->setText(
            tr("Place vehicle on the left side and then press OK"));
        // _msg_cal_box->show();
        qDebug() << "LEFT RESPONSE\n";
      } break;
      case ACCELCAL_VEHICLE_POS_RIGHT: {
        _cal_accel_state = CalibrationAccelState::RightSide;
        _msg_cal_box->setText(
            tr("Place vehicle on the right side and then press OK"));
        // _msg_cal_box->show();
        qDebug() << "RIGHT RESPONSE\n";
      } break;
      case ACCELCAL_VEHICLE_POS_NOSEUP: {
        _cal_accel_state = CalibrationAccelState::NoseUp;
        _msg_cal_box->setText(
            tr("Place vehicle in noseup position and then press OK"));
        // _msg_cal_box->show();
        qDebug() << "NOSEUP RESPONSE\n";
      } break;
      case ACCELCAL_VEHICLE_POS_NOSEDOWN: {
        _cal_accel_state = CalibrationAccelState::NoseDown;
        _msg_cal_box->setText(
            tr("Place vehicle in nosedown position and then press OK"));
        // _msg_cal_box->show();
        qDebug() << "NOSEDOWN RESPONSE\n";
      } break;
      case ACCELCAL_VEHICLE_POS_BACK: {
        _cal_accel_state = CalibrationAccelState::Back;
        _msg_cal_box->setText(
            tr("Place vehicle on the back side and then press OK"));
        // _msg_cal_box->show();
        qDebug() << "BACK RESPONSE\n";
      } break;
      case ACCELCAL_VEHICLE_POS_SUCCESS: {
        _cal_state = CalibrationState::None;
        _cal_accel_state = CalibrationAccelState::None;
        _msg_cal_box->setText(tr("Calibration completed"));
        reset();
        qDebug() << "SUCCESS RESPONSE\n";
      } break;
      case ACCELCAL_VEHICLE_POS_FAILED: {
        _cal_state = CalibrationState::None;
        _cal_accel_state = CalibrationAccelState::None;
        _msg_cal_box->setText(tr("Calibration failed"));
        reset();
        qDebug() << "FAIL RESPONSE\n";
      } break;
      case ACCELCAL_VEHICLE_POS_ENUM_END:
        break;
      }
      _msg_cal_box->show();
    }
  } break;
  default:
    break;
  }
}

void MainWindow::reset() {
  _cal_state = CalibrationState::None;
  _cal_accel_state = CalibrationAccelState::None;
}
