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
#include <chrono>
#include <format>
#include <string>

static constexpr auto kHeartbeatTimeout = std::chrono::seconds{3};
static constexpr auto kSerialReconnectTimeout = std::chrono::seconds{5};
static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");

MainWindow::MainWindow(QWidget *parent)
		: QMainWindow(parent),
			_toolbar(new QToolBar()),
			_ports_box(new QComboBox(_toolbar)),
			_action_refresh(new QAction(_toolbar)),
			_action_connect(new QAction(_toolbar)),
			_action_disconnect(new QAction(_toolbar)),
			_action_clear(new QAction(_toolbar)),
			_action_open_settings(new QAction(_toolbar)),
			_action_open_ap_params(new QAction(_toolbar)),
			_action_open_console(new QAction(_toolbar)),
			_action_reboot_ap(new QAction(_toolbar)),
			_action_logout(new QAction(_toolbar)),
			_central_widget(new QStackedWidget()),
			_statusbar(new QStatusBar()),
			_serial_status_label(new QLabel),
			_ap_status_label(new QLabel),
			_console(new Console),
			_authentication_page(new AuthenticationPage()),
			_msg_cal_box(new QMessageBox(this)),
			// _firmware_upload_page(new FirmwareUploadPage(nullptr,
			// _firmware_uploader)),
			_autopilot(new Autopilot(this)),
			_serial(new QSerialPort()),
			_mavlink_manager(new MavlinkManager(this, _serial, _autopilot)),
			_firmware_uploader(
					new FirmwareUploader(nullptr, _serial, _mavlink_manager)),
			_firmware_upload_page{
					new FirmwareUploadPage(nullptr, _firmware_uploader)},
			_autopilot_settings_page(new AutopilotSettingsPage()),
			// _qml_view(new QQuickView(QUrl("qrc:/AuthenticationForm.qml"))),
			// _qml_container(QWidget::createWindowContainer(_qml_view, this)),
			_ap_params_page(new ApParametersPage(this, _mavlink_manager, _autopilot)),
			_serial_write_timer(new QTimer(this)),
			_port_settings{},
			_heartbeat_timer(new QTimer(this)),
			_send_param_timer(new QTimer(this)) {
	setWindowTitle("Autopilot selfcheck");
	setMinimumSize(600, 800);
	setGeometry(QRect(0, 0, 600, 800));

	// Main window content
	addToolBar(_toolbar);
	setCentralWidget(_central_widget);
	setStatusBar(_statusbar);

	// toolbar content
	_toolbar->setMovable(false);
	_toolbar->setVisible(false);
	const auto spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	_toolbar->addWidget(_ports_box);
	_toolbar->addAction(_action_refresh);
	_toolbar->addAction(_action_connect);
	_toolbar->addAction(_action_disconnect);
	_toolbar->addAction(_action_open_settings);
	_toolbar->addAction(_action_open_ap_params);
	_toolbar->addAction(_action_reboot_ap);
	_toolbar->addAction(_action_open_console);
	_toolbar->addAction(_action_clear);
	_toolbar->addWidget(spacer);
	_toolbar->addAction(_action_logout);

	// _ports_box->setEditable(false);

	_action_refresh->setIcon(QIcon(":/images/refresh.png"));
	_action_refresh->setText(tr("Refresh"));
	_action_refresh->setToolTip(tr("Refresh device list"));
	_action_refresh->setEnabled(true);

	_action_connect->setIcon(QIcon(":/images/connect.png"));
	_action_connect->setText(tr("Connect"));
	_action_connect->setToolTip(tr("Connect"));
	_action_connect->setEnabled(false);

	_action_disconnect->setIcon(QIcon(":/images/disconnect.png"));
	_action_disconnect->setText(tr("Disconnect"));
	_action_disconnect->setToolTip(tr("Disconnect"));
	_action_disconnect->setEnabled(false);

	_action_clear->setIcon(QIcon(":/images/clear.png"));
	_action_clear->setText(tr("Clear"));
	_action_clear->setToolTip(tr("Clear console"));
	_action_clear->setEnabled(true);
	_action_clear->setVisible(false);

	_action_open_settings->setIcon(QIcon(":/images/settings.png"));
	_action_open_settings->setText(tr("Settings"));
	_action_open_settings->setToolTip(tr("Settings"));
	_action_open_settings->setEnabled(false);

	_action_open_ap_params->setIcon(QIcon(":/images/parameters.png"));
	_action_open_ap_params->setText(tr("Parameters"));
	_action_open_ap_params->setToolTip(tr("Parameters"));
	_action_open_ap_params->setEnabled(false);

	_action_open_console->setIcon(QIcon(":/images/terminal.png"));
	_action_open_console->setText(tr("Console"));
	_action_open_console->setToolTip(tr("Console"));
	_action_open_console->setEnabled(true);
	_action_open_console->setVisible(false);

	_action_reboot_ap->setIcon(QIcon(":/images/reboot.png"));
	_action_reboot_ap->setText(tr("Reboot"));
	_action_reboot_ap->setToolTip(tr("Reboot"));
	_action_reboot_ap->setEnabled(false);

	_action_logout->setText(tr("Logout"));
	_action_logout->setToolTip(tr("Logout"));
	_action_logout->setEnabled(true);

	// central widget content
	_central_widget->addWidget(_authentication_page);
	_central_widget->addWidget(_firmware_upload_page);
	_central_widget->addWidget(_autopilot_settings_page);
	_central_widget->addWidget(_ap_params_page);

	_central_widget->setCurrentWidget(_authentication_page);

	// _central_widget_layout->addWidget(_authentication_page);
	// _central_widget_layout->addWidget(_console);
	// _central_widget_layout->addWidget(_qml_container);

	// _qml_container->setFocusPolicy(Qt::TabFocus);

	_console->setEnabled(false);

	_msg_cal_box->setWindowTitle(tr("Calibration"));

	_msg_cal_box_button = _msg_cal_box->addButton(QMessageBox::Ok);

	// status bar content
	_statusbar->addWidget(_serial_status_label);
	_statusbar->addWidget(_ap_status_label);
	_statusbar->setVisible(false);

	_serial_status_label->setText(tr("Disconnected"));

	_ap_status_label->setText(tr("Autopilot disconnected"));

	_serial_write_timer->setSingleShot(true);
	_heartbeat_timer->setSingleShot(true);
	_send_param_timer->setSingleShot(true);

	new QShortcut(Qt::CTRL | Qt::Key_H, this, [this]() {
		_action_open_console->setVisible(true);
		_action_clear->setVisible(true);
	});

	// Connections
	initActionsConnections();
	initSerialPortEventsConnections();
	initPortsBoxEventsConnections();

	connect(_msg_cal_box_button, &QPushButton::clicked, this,
					&MainWindow::handleCalibrationDialogButton);

	connect(_serial_write_timer, &QTimer::timeout, this,
					&MainWindow::handleWriteTimeout);

	// heartbeat timer connections
	connect(_heartbeat_timer, &QTimer::timeout, this,
					&MainWindow::handleHeartbeatTimeout);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&MainWindow::_handleMavlinkMessageReceive);
	connect(_mavlink_manager, &MavlinkManager::serialWriteErrorOccured, this,
					&MainWindow::_handleSerialWriteError);

	// authentiation form connections
	connect(_authentication_page, &AuthenticationPage::login, this,
					&MainWindow::_login);

	// firmware upload page connections
	connect(this, &MainWindow::serialConnected, _firmware_upload_page,
					&FirmwareUploadPage::handleSerialConnection);
	connect(this, &MainWindow::serialDisconnected, _firmware_upload_page,
					&FirmwareUploadPage::handleSerialDisconnection);
	connect(_firmware_upload_page, &FirmwareUploadPage::uploadFirmwareStarted,
					this, &MainWindow::handleFirmwareUpload);

	// autopilot settings page connections
	connect(this, &MainWindow::IMUUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleIMUUpdate);
	connect(this, &MainWindow::powerStatusUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handlePowerStatusUpdate);
	connect(this, &MainWindow::mcuStatusUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleMcuStatusUpdate);
	connect(this, &MainWindow::imu2Updated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleImu2Update);
	connect(this, &MainWindow::attitudeUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleAttitudeUpdate);
	connect(this, &MainWindow::globalPositionIntUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleGlobalPositionIntUpdate);
	connect(this, &MainWindow::vfrHudUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleVfrHudUpdate);

	connect(this, &MainWindow::gyroStatusUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleGyroStatusUpdate);
	connect(this, &MainWindow::accelStatusUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleAccelStatusUpdate);
	connect(this, &MainWindow::magStatusUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleMagStatusUpdate);

	connect(_autopilot_settings_page,
					&AutopilotSettingsPage::startAccelCalibration, this,
					&MainWindow::_handleStartAccelCalibration);
	connect(this, &MainWindow::accelerometerCalibrationCompleted,
					_autopilot_settings_page,
					&AutopilotSettingsPage::handleCompleteAccelerometerCalibration);

	connect(_autopilot_settings_page,
					&AutopilotSettingsPage::startLevelCalibration, this,
					&MainWindow::_handleStartLevelCalibration);
	connect(this, &MainWindow::levelCalibrationCompleted,
					_autopilot_settings_page,
					&AutopilotSettingsPage::handleCompleteLevelCalibration);

	connect(_autopilot_settings_page, &AutopilotSettingsPage::startMagCalibration,
					this, &MainWindow::_handleStartMagCalibration);
	connect(_autopilot_settings_page,
					&AutopilotSettingsPage::cancelMagCalibration, this,
					&MainWindow::_handleCancelMagCalibration);
	connect(this, &MainWindow::magCalProgressUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleMagCalProgressUpdate);
	connect(this, &MainWindow::magCalReportUpdated, _autopilot_settings_page,
					&AutopilotSettingsPage::handleMagCalReportUpdate);

	connect(_autopilot_settings_page,
					&AutopilotSettingsPage::startGyroCalibration, this,
					&MainWindow::_handleStartGyroCalibration);
	connect(this, &MainWindow::gyroCalibrationCompleted, _autopilot_settings_page,
					&AutopilotSettingsPage::handleGyroCalibrationComplete);

	// autopilot parameters page connections
	connect(_ap_params_page, &ApParametersPage::parametersWritten, this,
					&MainWindow::_rebootAp);
}

MainWindow::~MainWindow() = default;

void MainWindow::_setApState(AutopilotState state) {
	_autopilot->state = state;
	emit apStateUpdated(state);
}

void MainWindow::_handleSerialWriteError(const QString &error_msg) {
	QMessageBox::warning(this, tr("Warning"), error_msg);
}

void MainWindow::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	const auto msg =
			std::format("ID: {} sequence: {} from component: {} of system: {}\n",
									std::to_string(mavlink_message.msgid),
									std::to_string(mavlink_message.seq),
									std::to_string(mavlink_message.compid),
									std::to_string(mavlink_message.sysid));
	QByteArray data(msg.c_str(), static_cast<uint32_t>(msg.length()));
	_console->putData(data);

	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_HEARTBEAT: {
		mavlink_heartbeat_t heartbeat;
		mavlink_msg_heartbeat_decode(&mavlink_message, &heartbeat);
		const auto heartbeat_str = std::format(
				"HEARTBEAT type: {} autopilot: {} base_mode: {} custom_mode: {} "
				"system_status: {} mavlink_version: {}\n",
				heartbeat.type, heartbeat.autopilot, heartbeat.base_mode,
				heartbeat.custom_mode, heartbeat.system_status,
				heartbeat.mavlink_version);
		QByteArray data(heartbeat_str.c_str(),
										static_cast<uint32_t>(heartbeat_str.length()));
		_console->putData(data);
		_heartbeat_timer->start(kHeartbeatTimeout);
		if (_autopilot->state == AutopilotState::None) {
			_autopilot->state = AutopilotState::Alive;
			_ap_status_label->setText(tr("Autopilot connected"));
			_action_open_settings->setEnabled(true);
			_action_open_ap_params->setEnabled(true);
		}
	} break;
	case MAVLINK_MSG_ID_SYS_STATUS: {
		mavlink_sys_status_t sys_status;
		mavlink_msg_sys_status_decode(&mavlink_message, &sys_status);
		const auto status_str = std::format(
				"sensors present: {:b} sensors enabled: {:b} load: {} errors_comm: "
				"{}\n",
				static_cast<uint32_t>(sys_status.onboard_control_sensors_present),
				static_cast<uint32_t>(sys_status.onboard_control_sensors_enabled),
				std::to_string(sys_status.voltage_battery),
				std::to_string(sys_status.errors_comm));
		QByteArray data(status_str.c_str(),
										static_cast<uint32_t>(status_str.length()));
		_console->putData(data);
		_updateSensorsStatus(sys_status);
	} break;
	case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
		mavlink_global_position_int_t global_position;
		mavlink_msg_global_position_int_decode(&mavlink_message, &global_position);
		const auto coords_str =
				std::format("lon: {} lat: {} alt: {} vx: {} vy: {} vz: {} hdg: {}\n",
										global_position.lon, global_position.lat,
										global_position.alt, global_position.vx, global_position.vy,
										global_position.vz, global_position.hdg);
		QByteArray data(coords_str.c_str(),
										static_cast<uint32_t>(coords_str.length()));
		_console->putData(data);
		emit globalPositionIntUpdated(global_position);
	} break;
	case MAVLINK_MSG_ID_POWER_STATUS: {
		mavlink_power_status_t power_status;
		mavlink_msg_power_status_decode(&mavlink_message, &power_status);
		const auto status_str =
				std::format("Rail voltage: {} mV\n", power_status.Vcc);
		QByteArray data(status_str.c_str(),
										static_cast<uint32_t>(status_str.length()));
		_console->putData(data);
		emit powerStatusUpdated(power_status);
	} break;
	case MAVLINK_MSG_ID_RAW_IMU: {
		mavlink_raw_imu_t raw_imu;
		mavlink_msg_raw_imu_decode(&mavlink_message, &raw_imu);
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
		_console->putData(data);
		emit IMUUpdated(raw_imu);
	} break;
	case MAVLINK_MSG_ID_MCU_STATUS: {
		mavlink_mcu_status_t mcu_status;
		mavlink_msg_mcu_status_decode(&mavlink_message, &mcu_status);
		auto mcu_status_str =
				std::format("MCU_STATUS temp: {} voltage: {} v_min: {} v_max: {}\n",
										mcu_status.MCU_temperature, mcu_status.MCU_voltage,
										mcu_status.MCU_voltage_min, mcu_status.MCU_voltage_max);
		QByteArray data(mcu_status_str.c_str(),
										static_cast<uint32_t>(mcu_status_str.length()));
		_console->putData(data);
		emit mcuStatusUpdated(mcu_status);
	} break;
	case MAVLINK_MSG_ID_COMMAND_ACK: {
		mavlink_command_ack_t cmd_ack;
		mavlink_msg_command_ack_decode(&mavlink_message, &cmd_ack);
		auto cmd_ack_str = std::format(
				"CMD_ACK COMMAND: {} RESULT: {} PROGRESS: {} RES_PRM2: {} "
				"TARGET_SYS: {} TARGET_CMP: {}\n",
				cmd_ack.command, cmd_ack.result, cmd_ack.progress,
				cmd_ack.result_param2, cmd_ack.target_system, cmd_ack.target_component);
		QByteArray data(cmd_ack_str.c_str(),
										static_cast<uint32_t>(cmd_ack_str.length()));
		_console->putData(data);
		_handleCommandAck(cmd_ack);
		qDebug() << cmd_ack_str << '\n';
	} break;
	case MAVLINK_MSG_ID_COMMAND_LONG: {
		mavlink_command_long_t cmd;
		mavlink_msg_command_long_decode(&mavlink_message, &cmd);
		auto cmd_str = std::format(
				"CMD_LONG COMMAND: {} SYSTEM: {} COMPONENT: {} CONFIMATION: {} "
				"param1: {} param2: {} param3: {} param4: {} param5: "
				"{} param6: {} param7: {}\n",
				cmd.command, cmd.target_system, cmd.target_component, cmd.confirmation,
				cmd.param1, cmd.param2, cmd.param3, cmd.param4, cmd.param5, cmd.param6,
				cmd.param7);
		QByteArray data(cmd_str.c_str(), static_cast<uint32_t>(cmd_str.length()));
		_console->putData(data);
		qDebug() << cmd_str << '\n';
		parseCommand(cmd);
	} break;
	case MAVLINK_MSG_ID_PARAM_VALUE: {
		mavlink_param_value_t param_value;
		mavlink_msg_param_value_decode(&mavlink_message, &param_value);
		auto param_value_str = std::format(
				"PARAM_VALUE ID: {} VALUE: {} TYPE: {} COUNT: {} INDEX: {}\n",
				param_value.param_id, param_value.param_value, param_value.param_type,
				param_value.param_count, param_value.param_index);
		QByteArray data(param_value_str.c_str(),
										static_cast<uint32_t>(param_value_str.length()));
		_console->putData(data);
		// _handleApParamReceive(param_value);
	} break;
	case MAVLINK_MSG_ID_STATUSTEXT: {
		mavlink_statustext_t statustext;
		mavlink_msg_statustext_decode(&mavlink_message, &statustext);
		auto statustext_str =
				std::format("ID: {} CHUNK_SEQ: {} SEVERITY: {} TEXT: {}\n",
										std::to_string(statustext.id), statustext.chunk_seq,
										statustext.severity, statustext.text);
		QByteArray data(statustext_str.c_str(),
										static_cast<uint32_t>(statustext_str.length()));
		_console->putData(data);
	} break;
	case MAVLINK_MSG_ID_MAG_CAL_PROGRESS: {
		mavlink_mag_cal_progress_t mag_cal_progress;
		mavlink_msg_mag_cal_progress_decode(&mavlink_message, &mag_cal_progress);
		auto mag_cal_progress_str =
				std::format("MAG_CAL_PROGRESS ATTEMPT: {} PCT: {} STATUS: {}\n",
										mag_cal_progress.attempt, mag_cal_progress.completion_pct,
										mag_cal_progress.cal_status);
		QByteArray data(mag_cal_progress_str.c_str(),
										static_cast<uint32_t>(mag_cal_progress_str.length()));
		_console->putData(data);
		emit magCalProgressUpdated(mag_cal_progress);
		qDebug() << mag_cal_progress_str.c_str() << '\n';
	} break;
	case MAVLINK_MSG_ID_MAG_CAL_REPORT: {
		mavlink_mag_cal_report_t mag_cal_report;
		mavlink_msg_mag_cal_report_decode(&mavlink_message, &mag_cal_report);
		auto mag_cal_report_str =
				std::format("MAG_CAL_REPORT STATUS: {}", mag_cal_report.cal_status);
		QByteArray data(mag_cal_report_str.c_str(),
										static_cast<uint32_t>(mag_cal_report_str.length()));
		_console->putData(data);
		if (_cal_mag_state == CalibrationMagState::InProgress) {
			emit magCalReportUpdated(mag_cal_report);
			_cal_mag_state = CalibrationMagState::None;
		}
		qDebug() << mag_cal_report_str << '\n';
	} break;
	case MAVLINK_MSG_ID_SCALED_IMU: {
		mavlink_scaled_imu_t scaled_imu;
		mavlink_msg_scaled_imu_decode(&mavlink_message, &scaled_imu);
		auto scaled_imu_str =
				std::format("SCALED_IMU xacc: {} yacc: {} zacc: {} xgyro: {} "
										"ygyro: {} zgyro: {} xmag: {} ymag: {} zmag: {}\n",
										scaled_imu.xacc, scaled_imu.yacc, scaled_imu.zacc,
										scaled_imu.xgyro, scaled_imu.ygyro, scaled_imu.zgyro,
										scaled_imu.xmag, scaled_imu.ymag, scaled_imu.zmag);
		QByteArray data(scaled_imu_str.c_str(),
										static_cast<uint32_t>(scaled_imu_str.length()));
		_console->putData(data);
	} break;
	case MAVLINK_MSG_ID_SCALED_IMU2: {
		mavlink_scaled_imu2_t scaled_imu;
		mavlink_msg_scaled_imu2_decode(&mavlink_message, &scaled_imu);
		auto scaled_imu_str =
				std::format("SCALED_IMU2 xacc: {} yacc: {} zacc: {} xgyro: {} "
										"ygyro: {} zgyro: {} xmag: {} ymag: {} zmag: {}\n",
										scaled_imu.xacc, scaled_imu.yacc, scaled_imu.zacc,
										scaled_imu.xgyro, scaled_imu.ygyro, scaled_imu.zgyro,
										scaled_imu.xmag, scaled_imu.ymag, scaled_imu.zmag);
		QByteArray data(scaled_imu_str.c_str(),
										static_cast<uint32_t>(scaled_imu_str.length()));
		emit imu2Updated(scaled_imu);
		_console->putData(data);
	} break;
	case MAVLINK_MSG_ID_SCALED_IMU3: {
		mavlink_scaled_imu3_t scaled_imu;
		mavlink_msg_scaled_imu3_decode(&mavlink_message, &scaled_imu);
		auto scaled_imu_str =
				std::format("SCALED_IMU3 xacc: {} yacc: {} zacc: {} xgyro: {} "
										"ygyro: {} zgyro: {} xmag: {} ymag: {} zmag: {}\n",
										scaled_imu.xacc, scaled_imu.yacc, scaled_imu.zacc,
										scaled_imu.xgyro, scaled_imu.ygyro, scaled_imu.zgyro,
										scaled_imu.xmag, scaled_imu.ymag, scaled_imu.zmag);
		QByteArray data(scaled_imu_str.c_str(),
										static_cast<uint32_t>(scaled_imu_str.length()));
		_console->putData(data);
	} break;
	case MAVLINK_MSG_ID_ATTITUDE: {
		mavlink_attitude_t attitude;
		mavlink_msg_attitude_decode(&mavlink_message, &attitude);
		auto attitude_str = std::format(
				"ATTITUDE roll: {} pitch: {} yaw: {} rollspeed: {} pitchspeed: {} "
				"yawspeed: {}\n",
				attitude.roll, attitude.pitch, attitude.yaw, attitude.rollspeed,
				attitude.pitchspeed, attitude.yawspeed);
		QByteArray data(attitude_str.c_str(),
										static_cast<uint32_t>(attitude_str.length()));
		_console->putData(data);
		emit attitudeUpdated(attitude);
	} break;
	case MAVLINK_MSG_ID_VFR_HUD: {
		mavlink_vfr_hud_t vfr_hud;
		mavlink_msg_vfr_hud_decode(&mavlink_message, &vfr_hud);
		auto vfr_hud_str =
				std::format("VFR_HUD airspeed: {} groundspeed: {} heading: {} "
										"throttle: {} alt: {} climb: {}",
										vfr_hud.airspeed, vfr_hud.groundspeed, vfr_hud.heading,
										vfr_hud.throttle, vfr_hud.alt, vfr_hud.climb);
		QByteArray data(vfr_hud_str.c_str(),
										static_cast<uint32_t>(vfr_hud_str.length()));
		_console->putData(data);
		emit vfrHudUpdated(vfr_hud);
	} break;
	default:
		break;
	}
}

void MainWindow::handleError(QSerialPort::SerialPortError error) {
	if (_firmware_uploader->upload_state != FirmwareUploadState::None) {
		return;
	}
	switch (error) {
	case QSerialPort::ResourceError: {
		qDebug() << _serial->errorString() << '\n';
		// QMessageBox::critical(this, tr("Critical Error"),
		// _serial->errorString());
		// FIXME: This error happens sometimes and not breaking connection so we do
		// not close the port, the reason is not figured out
		// closeSerialPort();
	} break;
	case QSerialPort::PermissionError: {
		QMessageBox::warning(this, tr("Warning"),
												 tr("No device permissions or it is already in use"));
	} break;
	default: {
		// sometimes error emits with `No error` message
		if (_serial->errorString() != "No error") {
			qDebug() << "Serial error: " << _serial->errorString();
			// QMessageBox::critical(this, tr("Error"), _serial->errorString());
		}
	}
	}
}

void MainWindow::handleBytesWritten(qint64 bytes) {
	_bytesToWrite -= bytes;
	if (_bytesToWrite == 0) {
		_serial_write_timer->stop();
	}
}

void MainWindow::handleWriteTimeout() {
	const QString error = tr("Write operation timed out for port %1.\n"
													 "Error: %2")
														.arg(_serial->portName(), _serial->errorString());
	showWriteError(error);
}

void MainWindow::_login(const QString &username, const QString &password) {
	qDebug() << "Login event\n" << username << '\n' << password << '\n';
	// TODO: add authentication
	fillPortsInfo();
	_trySerialConnect();
	_central_widget->setCurrentWidget(_firmware_upload_page);
	_toolbar->setVisible(true);
	_statusbar->setVisible(true);
}

void MainWindow::_logout() {
	closeSerialPort();
	_toolbar->setVisible(false);
	_central_widget->setCurrentWidget(_authentication_page);
}

void MainWindow::handleFirmwareUpload(DroneType drone_type) {
	qDebug() << std::format("Upload firmware. Drone type: {}\n",
													static_cast<int>(drone_type));
	closeSerialPort();
	_autopilot->state = AutopilotState::Flashing;
	_heartbeat_timer->stop();
}

void MainWindow::_openConsole() {
	_console->show();
}

void MainWindow::_openApParamsPage() {
	_central_widget->setCurrentWidget(_ap_params_page);
	if (_autopilot->params_state == AutopilotParamsState::None) {
		_mavlink_manager->sendParamRequestList();
		_ap_params_page->clearParamsToUpload();
	}
}

void MainWindow::_openSettingsPage() {
	_central_widget->setCurrentWidget(_autopilot_settings_page);
}

void MainWindow::handleHeartbeatTimeout() {
	// QMessageBox::critical(this, tr("Error"), tr("Heartbeat error"));
	_autopilot->state = AutopilotState::None;
	_ap_status_label->setText(tr("Autopilot disconnected"));
	_action_open_settings->setEnabled(false);
	_action_open_ap_params->setEnabled(false);
	_autopilot->params_state = AutopilotParamsState::None;
	// closeSerialPort();
}

void MainWindow::_handleStartAccelCalibration() {
	const auto command = MAV_CMD_PREFLIGHT_CALIBRATION;
	const uint8_t confirmation = 0;
	const float param1 = 0;
	const float param2 = 0;
	const float param3 = 0;
	const float param4 = 0;
	const float param5 = 1; // accelerometer calibration
	_mavlink_manager->sendCmdLong(command, confirmation, param1, param2, param3,
																param4, param5);
	_cal_state = CalibrationState::InProgress;
	qDebug("Accel calibration started\n");
}

void MainWindow::_handleStartLevelCalibration() {
	const auto command = MAV_CMD_PREFLIGHT_CALIBRATION;
	const uint8_t confirmation = 0;
	const float param1 = 0;
	const float param2 = 0;
	const float param3 = 0;
	const float param4 = 0;
	const float param5 = 2; // level calibration
	_mavlink_manager->sendCmdLong(command, confirmation, param1, param2, param3,
																param4, param5);
	_cal_lvl_state = CalibrationLevelState::InProgress;
	qDebug("Level calibration started\n");
}

void MainWindow::_handleStartMagCalibration() {
	const uint8_t confirmation = 0;
	const auto command = MAV_CMD_DO_START_MAG_CAL;
	const float param1 = 0; // Bitmask (all)
	const float param2 = 1; // Retry on failure
	const float param3 = 1; // Autosave
	const float param4 = 0; // Delay
	const float param5 = 0; // Autoreboot
	_mavlink_manager->sendCmdLong(command, confirmation, param1, param2, param3,
																param4, param5);
	_cal_mag_state = CalibrationMagState::InProgress;
	qDebug("Mag calibration started\n");
}

void MainWindow::_handleCancelMagCalibration() {
	const auto command = MAV_CMD_DO_CANCEL_MAG_CAL;
	const uint8_t confirmation = 0;
	const float param1 = 0; // Bitmask (all)
	_mavlink_manager->sendCmdLong(command, confirmation, param1);
	_cal_mag_state = CalibrationMagState::None;
	qDebug() << "Cancel mag calibration" << '\n';
}

void MainWindow::_handleStartGyroCalibration() {
	const auto command = MAV_CMD_PREFLIGHT_CALIBRATION;
	const uint8_t confirmation = 0;
	const float param1 = 1; // gyroscope calibration
	_mavlink_manager->sendCmdLong(command, confirmation, param1);
	_cal_state = CalibrationState::InProgress;
	_cal_gyro_state = CalibrationState::InProgress;
	qDebug("Gyro calibration started\n");
}

void MainWindow::handleCalibrationDialogButton() {
	if (_cal_state == CalibrationState::None) {
		return;
	}
	float vehicle_position_param;
	switch (_cal_accel_state) {
	case CalibrationAccelState::Level: {
		_cal_accel_state = CalibrationAccelState::LeftSide;
		vehicle_position_param = ACCELCAL_VEHICLE_POS_LEVEL;
	} break;
	case CalibrationAccelState::LeftSide: {
		vehicle_position_param = ACCELCAL_VEHICLE_POS_LEFT;
	} break;
	case CalibrationAccelState::RightSide: {
		vehicle_position_param = ACCELCAL_VEHICLE_POS_RIGHT;
	} break;
	case CalibrationAccelState::NoseDown: {
		vehicle_position_param = ACCELCAL_VEHICLE_POS_NOSEDOWN;
	} break;
	case CalibrationAccelState::NoseUp: {
		vehicle_position_param = ACCELCAL_VEHICLE_POS_NOSEUP;
	} break;
	case CalibrationAccelState::Back: {
		vehicle_position_param = ACCELCAL_VEHICLE_POS_BACK;
	} break;
	case CalibrationAccelState::None:
	default:
		_msg_cal_box->close();
		return;
	}

	const auto command = MAV_CMD_ACCELCAL_VEHICLE_POS;
	const auto confirmation = 0;
	_mavlink_manager->sendCmdLong(command, confirmation, vehicle_position_param);
}

void MainWindow::_handleCommandAck(mavlink_command_ack_t &cmd) {
	switch (cmd.command) {
	case MAV_CMD_PREFLIGHT_CALIBRATION: {
		switch (cmd.result) {
		case MAV_RESULT_ACCEPTED: {
			if (_cal_lvl_state == CalibrationLevelState::InProgress) {
				_cal_lvl_state = CalibrationLevelState::None;
				emit levelCalibrationCompleted();
			}
			if (_cal_gyro_state == CalibrationState::InProgress) {
				_cal_gyro_state = CalibrationState::None;
				emit gyroCalibrationCompleted();
			}
		} break;
		default:
			break;
		}
	} break;
	case MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN: {
		switch (cmd.result) {
		case MAV_RESULT_ACCEPTED: {
			closeSerialPort();
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
	connect(_action_refresh, &QAction::triggered, this,
					&MainWindow::fillPortsInfo);
	connect(_action_connect, &QAction::triggered, this,
					&MainWindow::openSerialPort);
	connect(_action_disconnect, &QAction::triggered, this,
					&MainWindow::closeSerialPort);
	connect(_action_clear, &QAction::triggered, _console, &Console::clear);
	connect(_action_open_settings, &QAction::triggered, this,
					&MainWindow::_openSettingsPage);
	connect(_action_open_ap_params, &QAction::triggered, this,
					&MainWindow::_openApParamsPage);
	connect(_action_open_console, &QAction::triggered, this,
					&MainWindow::_openConsole);
	connect(_action_reboot_ap, &QAction::triggered, this, &MainWindow::_rebootAp);
	connect(_action_logout, &QAction::triggered, this, &MainWindow::_logout);
}

void MainWindow::initSerialPortEventsConnections() {
	connect(_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
	connect(_serial, &QSerialPort::bytesWritten, this,
					&MainWindow::handleBytesWritten);
}

void MainWindow::initPortsBoxEventsConnections() {
	connect(_ports_box, &QComboBox::currentIndexChanged, this,
					&MainWindow::setPortSettings);
}

void MainWindow::setPortSettings(int index) {
	auto portSettingsMaybe = _ports_box->itemData(index);
	if (portSettingsMaybe.canConvert<QStringList>()) {
		auto port_settings_list = portSettingsMaybe.value<QStringList>();
		_port_settings = Settings{.name = port_settings_list.at(0),
															.baudRate = 115200,
															.dataBits = QSerialPort::Data8,
															.parity = QSerialPort::NoParity,
															.stopBits = QSerialPort::OneStop,
															.flowControl = QSerialPort::NoFlowControl};
		_action_connect->setEnabled(true);
	}
}

void MainWindow::showStatusMessage(const QString &message) {
	_serial_status_label->setText(message);
}

void MainWindow::showWriteError(const QString &message) {
	QMessageBox::warning(this, tr("Warning"), message);
}

void MainWindow::fillPortsInfo() {
	if (_serial_port_state == SerialPortState::Connected) {
		return;
	}
	_ports_box->clear();
	const auto blankString = tr(::blankString);
	const auto infos = QSerialPortInfo::availablePorts();

	for (const auto &info : infos) {
		// qDebug() <<
		// 	"DESCRIPTION: " << info.description() <<
		// 	"PORTNAME: " << info.portName() <<
		// 	"MANUFACTURER: " <<	info.manufacturer() <<
		// 	"PRODUCT_ID: " <<	info.productIdentifier() <<
		// 	"SERIAL_NUMBER: " <<	info.serialNumber() <<
		// 	"VENDOR_ID: " <<	info.vendorIdentifier() <<
		// 	"SYSTEM_LOCATION: " << info.systemLocation();
		QStringList list;
		const auto description = info.description();
		const auto manufacturer = info.manufacturer();
		const auto serialNumber = info.serialNumber();
		const auto vendorId = info.vendorIdentifier();
		const auto productId = info.productIdentifier();
		list << info.portName()
				 << (!description.isEmpty() ? description : blankString)
				 << (!manufacturer.isEmpty() ? manufacturer : blankString)
				 << (!serialNumber.isEmpty() ? serialNumber : blankString)
				 << info.systemLocation()
				 << (vendorId ? QString::number(vendorId, 16) : blankString)
				 << (productId ? QString::number(productId, 16) : blankString);

		_ports_box->addItem(list.constFirst(), list);
	}
	// _ports_box->addItem(tr("Custom"));
}

void MainWindow::_trySerialConnect() {
	static const auto port_regex = QRegularExpression("((ttyACM)|(COM))\\d+");
	for (int i = 0; i < _ports_box->count(); i++) {
		setPortSettings(i);
		if (port_regex.match(_port_settings.name).hasMatch()) {
			openSerialPort();
			_heartbeat_timer->start(kHeartbeatTimeout);
			while (_heartbeat_timer->isActive()) {
				if (_autopilot->state == AutopilotState::Alive) {
					_ports_box->setCurrentIndex(i);
					return;
				}
				QApplication::processEvents();
			}
		}
	}
}

void MainWindow::openSerialPort() {
	const auto p = _port_settings;
	_serial->setPortName(p.name);
	_serial->setBaudRate(p.baudRate);
	_serial->setDataBits(p.dataBits);
	_serial->setParity(p.parity);
	_serial->setStopBits(p.stopBits);
	_serial->setFlowControl(p.flowControl);

	if (_serial->open(QIODevice::ReadWrite)) {
		_console->setEnabled(true);
		_ports_box->setEnabled(false);
		_action_refresh->setEnabled(false);
		_action_connect->setEnabled(false);
		_action_disconnect->setEnabled(true);
		_action_reboot_ap->setEnabled(true);

		_serial_status_label->setText(
				tr("Connected to %1").arg(_serial->portName()));
		_heartbeat_timer->start(kHeartbeatTimeout);
		emit serialConnected();
		_serial_port_state = SerialPortState::Connected;
	} else {
		// QMessageBox::critical(this, tr("Error"), _serial->errorString());
		showStatusMessage(tr("Open error"));
	}
}

void MainWindow::closeSerialPort() {
	if (_serial->isOpen()) {
		_serial->close();
	}
	// _console->setEnabled(false);
	_ports_box->setEnabled(true);
	_action_refresh->setEnabled(true);
	_action_connect->setEnabled(true);
	_action_disconnect->setEnabled(false);
	_action_open_settings->setEnabled(false);
	_action_open_ap_params->setEnabled(false);
	_action_reboot_ap->setEnabled(false);
	showStatusMessage(tr("Disconnected"));
	_central_widget->setCurrentWidget(_firmware_upload_page);
	_heartbeat_timer->stop();
	_autopilot->state = AutopilotState::None;
	_ap_status_label->setText(tr("Autopilot disconnected"));
	reset();
	_serial_port_state = SerialPortState::Disconnected;
	fillPortsInfo();
	emit serialDisconnected();
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
				emit accelerometerCalibrationCompleted(CalibrationResult::Success);
				qDebug() << "SUCCESS RESPONSE\n";
			} break;
			case ACCELCAL_VEHICLE_POS_FAILED: {
				_cal_state = CalibrationState::None;
				_cal_accel_state = CalibrationAccelState::None;
				_msg_cal_box->setText(tr("Calibration failed"));
				emit accelerometerCalibrationCompleted(CalibrationResult::Failed);
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

void MainWindow::_updateSensorsStatus(mavlink_sys_status_t sys_status) {
	auto current_gyro_status = SensorStatus::NotFound;
	if (sys_status.onboard_control_sensors_present &
			MAV_SYS_STATUS_SENSOR_3D_GYRO) {
		if (sys_status.onboard_control_sensors_enabled &
				MAV_SYS_STATUS_SENSOR_3D_GYRO) {
			if (sys_status.onboard_control_sensors_health &
					MAV_SYS_STATUS_SENSOR_3D_GYRO) {
				current_gyro_status = SensorStatus::Enabled;
			} else {
				current_gyro_status = SensorStatus::Error;
			}
		} else {
			current_gyro_status = SensorStatus::Disabled;
		}
	}
	if (current_gyro_status != _gyro_status) {
		_gyro_status = current_gyro_status;
		emit gyroStatusUpdated(_gyro_status);
	}

	auto current_accel_status = SensorStatus::NotFound;
	if (sys_status.onboard_control_sensors_present &
			MAV_SYS_STATUS_SENSOR_3D_ACCEL) {
		if (sys_status.onboard_control_sensors_enabled &
				MAV_SYS_STATUS_SENSOR_3D_ACCEL) {
			if (sys_status.onboard_control_sensors_health &
					MAV_SYS_STATUS_SENSOR_3D_ACCEL) {
				current_accel_status = SensorStatus::Enabled;
			} else {
				current_accel_status = SensorStatus::Error;
			}
		} else {
			current_accel_status = SensorStatus::Disabled;
		}
	}
	if (current_accel_status != _accel_status) {
		_accel_status = current_accel_status;
		emit accelStatusUpdated(_accel_status);
	}

	auto current_mag_status = SensorStatus::NotFound;
	if (sys_status.onboard_control_sensors_present &
			MAV_SYS_STATUS_SENSOR_3D_MAG) {
		if (sys_status.onboard_control_sensors_enabled &
				MAV_SYS_STATUS_SENSOR_3D_MAG) {
			if (sys_status.onboard_control_sensors_health &
					MAV_SYS_STATUS_SENSOR_3D_MAG) {
				current_mag_status = SensorStatus::Enabled;
			} else {
				current_mag_status = SensorStatus::Error;
			}
		} else {
			current_mag_status = SensorStatus::Disabled;
		}
	}
	if (current_mag_status != _mag_status) {
		_mag_status = current_accel_status;
		emit magStatusUpdated(_mag_status);
	}
}

void MainWindow::_rebootAp() {
	const auto command = MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN;
	const uint8_t confirmation = 0;
	const float param1 = 1;
	_mavlink_manager->sendCmdLong(command, confirmation, param1);
	_autopilot->state = AutopilotState::None;
	_autopilot->params_state = AutopilotParamsState::None;
	_ap_status_label->setText(tr("Autopilot disconnected"));
	_heartbeat_timer->stop();
}

void MainWindow::_handleFirmwareUploadCompletion(FirmwareUploadResult result) {
	auto error_title = QString();
	auto error_msg = QString();
	switch (result) {
	// Firmware file errors
	case FirmwareUploadResult::FirmwareImageNotFound: {
		error_title = tr("Firmware file error");
		error_msg = tr("Firmware image not found");
	} break;
	case FirmwareUploadResult::FirmwareSizeNotFound: {
		error_title = tr("Firmware file error");
		error_msg = tr("Firmware size not found");
	} break;
	case FirmwareUploadResult::BoardIdNotFound: {
		error_title = tr("Firmware file error");
		error_msg = tr("Board id not found");
	} break;
	case FirmwareUploadResult::DecodeFail: {
		error_title = tr("Firmware file error");
		error_msg = tr("Image decoding error");
	} break;
	// Bootloader errors
	case FirmwareUploadResult::BootloaderNotFound: {
		error_title = tr("Bootloader error");
		error_msg = tr("Bootloader not found");
	} break;
	case FirmwareUploadResult::IncompatibleBoardType: {
		error_title = tr("Bootloader error");
		error_msg = tr("Incompatible board");
	} break;
	case FirmwareUploadResult::UnsupportedBoard: {
		error_title = tr("Bootloader error");
		error_msg = tr("Unsupported board");
	} break;
	case FirmwareUploadResult::UnsupportedBootloader: {
		error_title = tr("Bootloader error");
		error_msg = tr("Unsupported bootloader");
	} break;
	// Erase errors
	case FirmwareUploadResult::EraseFail: {
		error_title = tr("Erase error");
		error_msg = tr("Board erase failed");
	} break;
	// Flashing errors
	case FirmwareUploadResult::ProgramFail: {
		error_title = tr("Program error");
		error_msg = tr("Firmware program failed");
	} break;
	case FirmwareUploadResult::TooLargeFirmware: {
		error_title = tr("Program error");
		error_msg = tr("Too large firmware size");
	} break;
	// Verification errors
	case FirmwareUploadResult::VerificationFail: {
		error_title = tr("Verification error");
		error_msg = tr("Firmware verification failed");
	} break;
	// Successful upload
	case FirmwareUploadResult::Ok: {
		return;
	}
	}
	if (!error_title.isEmpty() || !error_msg.isEmpty()) {
		QMessageBox::critical(this, tr("Firmware upload error"), error_msg);
	}
	_autopilot->state = AutopilotState::None;
	_heartbeat_timer->start(kHeartbeatTimeout);
}
