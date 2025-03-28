#include "mainwindow.hpp"
#include "console.hpp"

static constexpr QSize WINDOW_MIN_SIZE = QSize(600, 850);
static constexpr auto kHeartbeatTimeout = std::chrono::seconds{7};
static constexpr auto kSerialReconnectTimeout = std::chrono::seconds{5};
static constexpr auto kSerialReconnectDelayTimeout = std::chrono::seconds{2};
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
			_action_open_mavftp_page(new QAction(_toolbar)),
			_action_open_console(new QAction(_toolbar)),
			_action_open_firmware_page(new QAction(_toolbar)),
			_action_open_full_setup_page(new QAction(_toolbar)),
			_action_reboot_ap(new QAction(_toolbar)),
			_action_about(new QAction(_toolbar)),
			_action_logout(new QAction(_toolbar)),
			_central_widget(new QStackedWidget()),
			_statusbar(new QStatusBar()),
			_serial_status_label(new QLabel),
			_ap_status_label(new QLabel()),
			_ap_os_label(new QLabel()),
			_ap_name_label(new QLabel()),
			_autopilot(new Autopilot(this)),
			_serial(new QSerialPort()),
			_mavlink_manager(new MavlinkManager(this, _serial, _autopilot)),
			_parameters_manager(
					new ParametersManager(this, _mavlink_manager, _autopilot)),
			_mav_ftp_manager(new MavFtpManager(this, _mavlink_manager, _autopilot)),
			_console(new Console(nullptr, _mavlink_manager)),
			_authentication_page(new AuthenticationPage()),
			_firmware_upload_page(new FirmwareUploadPage(this)),
			_autopilot_settings_page(
					new AutopilotSettingsPage(this, _mavlink_manager)),
			// _qml_view(new QQuickView(QUrl("qrc:/AuthenticationForm.qml"))),
			// _qml_container(QWidget::createWindowContainer(_qml_view, this)),
			_ap_params_page(
					new ApParametersPage(this, _autopilot, _parameters_manager)),
			_mavftp_page(new MavftpPage(this, _autopilot, _mav_ftp_manager)),
			_full_setup_page(new FullSetupPage(this, _autopilot, _parameters_manager,
																				 _mav_ftp_manager)),
			_serial_write_timer(new QTimer(this)),
			_port_settings{},
			_heartbeat_timer(new QTimer(this)),
			_send_param_timer(new QTimer(this)),
			_serial_reconnect_timer(new QTimer(this)),
			_serial_reconnect_delay_timer(new QTimer(this)) {
	setWindowTitle(tr("Autopilot selfcheck"));
	setMinimumSize(WINDOW_MIN_SIZE);
	setGeometry(QRect(QPoint(0, 0), WINDOW_MIN_SIZE));

	// Main window content
	addToolBar(_toolbar);
	setCentralWidget(_central_widget);
	setStatusBar(_statusbar);

	// toolbar content
	_toolbar->setMovable(false);
	_toolbar->setVisible(true);
	const auto spacer = new QWidget();
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	_toolbar->addWidget(_ports_box);
	_toolbar->addAction(_action_refresh);
	_toolbar->addAction(_action_connect);
	_toolbar->addAction(_action_disconnect);
	_toolbar->addAction(_action_open_settings);
	_toolbar->addAction(_action_open_ap_params);
	_toolbar->addAction(_action_open_mavftp_page);
	_toolbar->addAction(_action_open_firmware_page);
	_toolbar->addAction(_action_open_full_setup_page);
	_toolbar->addAction(_action_reboot_ap);
	_toolbar->addAction(_action_open_console);
	_toolbar->addAction(_action_clear);
	_toolbar->addAction(_action_about);
	_toolbar->addWidget(spacer);
	// _toolbar->addAction(_action_logout);

	// _ports_box->setEditable(false);
	_ports_box->setMinimumWidth(100);

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
	_action_open_settings->setText(tr("Calibration"));
	_action_open_settings->setToolTip(tr("Calibration"));
	_action_open_settings->setEnabled(false);

	_action_open_ap_params->setIcon(QIcon(":/images/parameters.png"));
	_action_open_ap_params->setText(tr("Parameters"));
	_action_open_ap_params->setToolTip(tr("Parameters"));
	_action_open_ap_params->setEnabled(false);

	_action_open_firmware_page->setIcon(QIcon(":/images/firmware.png"));
	_action_open_firmware_page->setText(tr("Upload firmware"));
	_action_open_firmware_page->setToolTip(tr("Upload firmware"));
	_action_open_firmware_page->setEnabled(true);

	_action_open_full_setup_page->setIcon(QIcon(":/images/full_setup.png"));
	_action_open_full_setup_page->setText(tr("Upload from archive"));
	_action_open_full_setup_page->setText(tr("Upload from archive"));
	_action_open_full_setup_page->setEnabled(true);

	_action_about->setIcon(QIcon(":/images/about.png"));
	_action_about->setText(tr("About"));
	_action_about->setToolTip(tr("About"));
	_action_about->setEnabled(true);

	_action_reboot_ap->setIcon(QIcon(":/images/reboot.png"));
	_action_reboot_ap->setText(tr("Reboot"));
	_action_reboot_ap->setToolTip(tr("Reboot"));
	_action_reboot_ap->setEnabled(false);

	_action_open_mavftp_page->setIcon(QIcon(":/images/folder.png"));
	_action_open_mavftp_page->setText("Mavftp");
	_action_open_mavftp_page->setToolTip("Mavftp");
	_action_open_mavftp_page->setEnabled(false);

	_action_open_console->setIcon(QIcon(":/images/terminal.png"));
	_action_open_console->setText(tr("Console"));
	_action_open_console->setToolTip(tr("Console"));
	_action_open_console->setEnabled(true);
	_action_open_console->setVisible(false);

	_action_logout->setText(tr("Logout"));
	_action_logout->setToolTip(tr("Logout"));
	_action_logout->setEnabled(true);

	// central widget content
	_central_widget->addWidget(_authentication_page);
	_central_widget->addWidget(_firmware_upload_page);
	_central_widget->addWidget(_autopilot_settings_page);
	_central_widget->addWidget(_ap_params_page);
	_central_widget->addWidget(_mavftp_page);
	_central_widget->addWidget(_full_setup_page);

	_central_widget->setCurrentWidget(_firmware_upload_page);

	// _central_widget_layout->addWidget(_authentication_page);
	// _central_widget_layout->addWidget(_console);
	// _central_widget_layout->addWidget(_qml_container);

	// _qml_container->setFocusPolicy(Qt::TabFocus);

	_console->setEnabled(false);

	// status bar content
	_statusbar->addWidget(_serial_status_label);
	_statusbar->addWidget(_ap_status_label);
	_statusbar->addPermanentWidget(_ap_os_label);
	_statusbar->addPermanentWidget(_ap_name_label);

	_serial_status_label->setText(tr("Disconnected"));

	_ap_status_label->setText(tr("Autopilot disconnected"));

	_serial_write_timer->setSingleShot(true);
	_heartbeat_timer->setSingleShot(true);
	_send_param_timer->setSingleShot(true);
	_serial_reconnect_timer->setSingleShot(true);
	_serial_reconnect_delay_timer->setSingleShot(true);

	new QShortcut(Qt::CTRL | Qt::Key_H, this, [this]() {
		_action_open_console->setVisible(true);
		_action_clear->setVisible(true);
	});

	// Connections
	initSerialPortEventsConnections();
	initPortsBoxEventsConnections();

	connect(this, &MainWindow::autopilotConnected, this,
					&MainWindow::_handleAutopilotConnection);

	// actions connections
	connect(_action_refresh, &QAction::triggered, this,
					&MainWindow::fillPortsInfo);
	connect(_action_connect, &QAction::triggered, this,
					&MainWindow::openSerialPort);
	connect(_action_disconnect, &QAction::triggered, this,
					&MainWindow::_handleDisconnectActionTrigger);
	connect(_action_clear, &QAction::triggered, _console, &Console::clear);
	connect(_action_open_settings, &QAction::triggered, this,
					&MainWindow::_openSettingsPage);
	connect(_action_open_ap_params, &QAction::triggered, this,
					&MainWindow::_openApParamsPage);
	connect(_action_open_mavftp_page, &QAction::triggered, this,
					&MainWindow::_openMavftpPage);
	connect(_action_open_firmware_page, &QAction::triggered, this,
					&MainWindow::_openFirmwareUploadPage);
	connect(_action_open_full_setup_page, &QAction::triggered, this,
					&MainWindow::_openFullSetupPage);
	connect(_action_reboot_ap, &QAction::triggered, this,
					&MainWindow::_handleRebootActionTrigger);
	connect(_action_open_console, &QAction::triggered, this,
					&MainWindow::_openConsole);
	connect(_action_about, &QAction::triggered, this,
					&MainWindow::_handleAboutActionTrigger);
	// connect(_action_logout, &QAction::triggered, this, &MainWindow::_logout);

	// timers connections
	connect(_serial_write_timer, &QTimer::timeout, this,
					&MainWindow::handleWriteTimeout);
	connect(_serial_reconnect_timer, &QTimer::timeout, this,
					&MainWindow::handleSerialReconnectTimeout);
	connect(_serial_reconnect_delay_timer, &QTimer::timeout, this,
					&MainWindow::_trySerialConnect);
	connect(_heartbeat_timer, &QTimer::timeout, this,
					&MainWindow::handleHeartbeatTimeout);

	// autopilot connections
	connect(_autopilot, &Autopilot::stateUpdated, this,
					&MainWindow::_handleAutopilotStateUpdate);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&MainWindow::_handleMavlinkMessageReceive);
	connect(_mavlink_manager, &MavlinkManager::serialWriteErrorOccured, this,
					&MainWindow::_handleSerialWriteError);

	// parameters manager connections
	connect(this, &MainWindow::autopilotConnected, _parameters_manager,
					&ParametersManager::handleAutopilotConnection);
	connect(_parameters_manager, &ParametersManager::parametersWritten, this,
					&MainWindow::_handleApParametersWrite);
	connect(_parameters_manager, &ParametersManager::paramsResetRequest, this,
					&MainWindow::_handleParamsResetRequest);

	// authentiation form connections
	// connect(_authentication_page, &AuthenticationPage::login, this,
	// 				&MainWindow::_login);

	// firmware upload page connections
	connect(_firmware_upload_page, &FirmwareUploadPage::uploadFirmwareStarted,
					this, [this]() {
						_handleFirmwareUploadStart();
						_central_widget->setCurrentWidget(_firmware_upload_page);
					});
	connect(_firmware_upload_page,
					&FirmwareUploadPage::uploadFirmwareSuccsessfullyCompleted, this,
					&MainWindow::_handleFirmwareUploadCompletion);

	// full setup page connections
	connect(_full_setup_page, &FullSetupPage::firmwareUploadStarted, this,
					&MainWindow::_handleFirmwareUploadStart);
	connect(_full_setup_page,
					&FullSetupPage::uploadFirmwareSuccsessfullyCompleted, this,
					&MainWindow::_handleFirmwareUploadCompletion);
	connect(_full_setup_page, &FullSetupPage::fullSetupCompleted, this,
					&MainWindow::_handleFullSetupCompletion);
	connect(this, &MainWindow::autopilotConnected, _full_setup_page,
					&FullSetupPage::handleAutopilotConnection);

	_trySerialConnect();
}

MainWindow::~MainWindow() = default;

void MainWindow::_handleSerialWriteError(const QString &error_msg) {
	QMessageBox::warning(this, tr("Warning"), error_msg);
}

void MainWindow::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_HEARTBEAT: {
		mavlink_heartbeat_t heartbeat;
		mavlink_msg_heartbeat_decode(&mavlink_message, &heartbeat);
		_heartbeat_timer->start(kHeartbeatTimeout);
		if (_autopilot->getState() == AutopilotState::None) {
			emit autopilotConnected();
		}
	} break;
	case MAVLINK_MSG_ID_STATUSTEXT: {
		mavlink_statustext_t statustext;
		mavlink_msg_statustext_decode(&mavlink_message, &statustext);

		if (statustext.severity == MAV_SEVERITY_INFO) {
			const uint8_t statustext_max_length = 50;
			const auto text =
					QString::fromUtf8(statustext.text, statustext_max_length);
			static const auto ap_name_regex =
					QRegularExpression("(Finco|Ardu).*\\sV.*");
			if (ap_name_regex.match(text).hasMatch()) {
				_ap_name_label->setText(text.left(text.indexOf(QChar('\0'))));
			}
			static const auto ap_os_regex = QRegularExpression("ChibiOS.*");
			if (ap_os_regex.match(text).hasMatch()) {
				_ap_os_label->setText(text.left(text.indexOf(QChar('\0'))));
			}
		}
	} break;
	default:
		break;
	}
}

void MainWindow::_handleApParametersWrite() {
	_rebootAp();
	_serial_reconnect_delay_timer->start(kSerialReconnectDelayTimeout);
	// _central_widget->setCurrentWidget(_ap_params_page);
}

void MainWindow::_handleDisconnectActionTrigger() {
	_autopilot->setParamsState(AutopilotParamsState::None);
	_autopilot->setParamsSendState(AutopilotParamsUploadState::None);
	_disconnect();
	_central_widget->setCurrentWidget(_firmware_upload_page);
}

void MainWindow::_handleRebootActionTrigger() {
	_autopilot->setParamsState(AutopilotParamsState::None);
	_autopilot->setParamsSendState(AutopilotParamsUploadState::None);
	_rebootAp();
	_central_widget->setCurrentWidget(_firmware_upload_page);
	_serial_reconnect_delay_timer->start(kSerialReconnectDelayTimeout);
}

void MainWindow::_handleAboutActionTrigger() {
	QMessageBox::about(this, tr("About"),
										 tr("Autopilot selfcheck v%1").arg(VERSION) + '\n' +
												 tr("Contacts for suggestions and bug reports: %1")
														 .arg("belkinnv@supercam.aero"));
}

void MainWindow::handleError(QSerialPort::SerialPortError error) {
	switch (error) {
	case QSerialPort::ResourceError: {
		qDebug() << "MAIN THREAD: " << _serial->errorString();
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
		qDebug() << "MAIN THREAD: " << _serial->errorString();
		// sometimes error emits with `No error` message
		// if (_serial->errorString() != "No error") {
		// 	qDebug() << "Serial error: " << _serial->errorString();
		// 	// QMessageBox::critical(this, tr("Error"), _serial->errorString());
		// }
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
	const QString error = tr("Write operation timed out for port %1.\nError: %2")
														.arg(_serial->portName(), _serial->errorString());
	showWriteError(error);
}

void MainWindow::_login(const QString &username, const QString &password) {
	qDebug() << "Login event\n" << username << '\n' << password << '\n';
	// TODO: add authentication
	// fillPortsInfo();
	_trySerialConnect();
	_central_widget->setCurrentWidget(_firmware_upload_page);
	_toolbar->setVisible(true);
}

void MainWindow::_logout() {
	_disconnect();
	_toolbar->setVisible(false);
	_central_widget->setCurrentWidget(_authentication_page);
}

void MainWindow::_openConsole() {
	_console->show();
}

void MainWindow::_openApParamsPage() {
	_central_widget->setCurrentWidget(_ap_params_page);
	if (_autopilot->getParamsState() == AutopilotParamsState::None) {
		_ap_params_page->updateApParameters();
	}
}

void MainWindow::_openMavftpPage() {
	_central_widget->setCurrentWidget(_mavftp_page);
}

void MainWindow::_openSettingsPage() {
	_central_widget->setCurrentWidget(_autopilot_settings_page);
}

void MainWindow::_openFirmwareUploadPage() {
	_central_widget->setCurrentWidget(_firmware_upload_page);
}

void MainWindow::_openFullSetupPage() {
	_central_widget->setCurrentWidget(_full_setup_page);
}

void MainWindow::handleHeartbeatTimeout() {
	// QMessageBox::critical(this, tr("Error"), tr("Heartbeat error"));
	_autopilot->setState(AutopilotState::None);
	_autopilot->setParamsState(AutopilotParamsState::None);
	_disconnect();
	_central_widget->setCurrentWidget(_firmware_upload_page);
	// _current_port_box_index = 0;
	_serial_reconnect_delay_timer->start(kSerialReconnectDelayTimeout);
	// closeSerialPort();
}

void MainWindow::handleSerialReconnectTimeout() {
	_disconnect();
	// if (_autopilot->getParamsSendState() != AutopilotParamsSendState::Sending)
	// { 	_central_widget->setCurrentWidget(_firmware_upload_page);
	// }
	if (_current_port_box_index < _ports_box->count() - 1) {
		_current_port_box_index++;
		qDebug() << "INCREASE PORT INDEX";
		// _trySerialConnect();
	} else {
		qDebug() << "RESET PORT INDEX";
		_current_port_box_index = 0;
		// _trySerialConnect();
		// QMessageBox::information(
		// 		this, tr("Connection failed"),
		// 		tr("Autopilot not found. Please try to connect manually"));
	}
	qDebug() << "RECONNECT TIMEOUT. PORT INDEX: " << _current_port_box_index
					 << "PORT_COUNT: " << _ports_box->count();
	_trySerialConnect();
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

void MainWindow::showWriteError(const QString &message) {
	QMessageBox::warning(this, tr("Warning"), message);
}

void MainWindow::fillPortsInfo() {
	if (_serial_port_state == SerialPortState::Connected) {
		return;
	}
	const auto infos = QSerialPortInfo::availablePorts();
	const auto port_list_updated =
			!std::equal(_port_list.cbegin(), _port_list.cend(), infos.cbegin(),
									[](const QSerialPortInfo &a, const QSerialPortInfo &b) {
										return a.portName() == b.portName();
									});
	if (port_list_updated) {
		qDebug() << "PORT LIST UPDATED";
		_current_port_box_index = 0;
	}

	_ports_box->clear();
	const auto blankString = tr(::blankString);
	static const auto port_regex = QRegularExpression("((ttyACM)|(COM))\\d+");
	for (const auto &info : infos) {
		if (!port_regex.match(info.portName()).hasMatch()) {
			continue;
		}
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
}

void MainWindow::_trySerialConnect() {
	if (_serial->isOpen()) {
		qDebug() << "Serial already open. Cannot connect";
		return;
	}
	_ap_status_label->setText(tr("Autopilot searching..."));
	fillPortsInfo();
	// static const auto port_regex = QRegularExpression("((ttyACM)|(COM))\\d+");
	// setPortSettings(_current_port_box_index);
	// if (port_regex.match(_port_settings.name).hasMatch()) {
	// 	qDebug() << "Try connect to " << _port_settings.name;
	// 	_ports_box->setCurrentIndex(_current_port_box_index);
	// 	openSerialPort();
	// 	_serial_reconnect_timer->start(kSerialReconnectTimeout);
	// }
	if (_ports_box->count() > 0) {
		setPortSettings(_current_port_box_index);
		qDebug() << "Try connect to " << _port_settings.name;
		qDebug() << "PORT INDEX: " << _current_port_box_index;
		_ports_box->setCurrentIndex(_current_port_box_index);
		openSerialPort();
		_serial_reconnect_timer->start(kSerialReconnectTimeout);
		return;
	}
	_current_port_box_index = 0;
	_serial_reconnect_delay_timer->start(kSerialReconnectDelayTimeout);
}

void MainWindow::_disconnect() {
	_closeSerialPort();
	// if (_ports_box->count() - 1 >= _current_port_box_index) {
	// 		_current_port_box_index = 0;
	// 	}
	_serial_port_state = SerialPortState::Disconnected;
	// _current_port_box_index = 0;
	// fillPortsInfo();
	_autopilot->setState(AutopilotState::None);

	_ports_box->setEnabled(true);
	_action_refresh->setEnabled(true);
	_action_connect->setEnabled(true);
	_action_disconnect->setEnabled(false);
	_action_reboot_ap->setEnabled(false);

	_serial_status_label->setText(tr("Disconnected"));
	_ap_status_label->setText(tr("Autopilot disconnected"));
	_ap_os_label->clear();
	_ap_name_label->clear();

	_heartbeat_timer->stop();
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
		qDebug() << "Serial connected";
		_console->setEnabled(true);
		_ports_box->setEnabled(false);
		_action_refresh->setEnabled(false);
		_action_connect->setEnabled(false);
		_action_disconnect->setEnabled(true);
		_action_reboot_ap->setEnabled(true);

		_serial_status_label->setText(
				tr("Connected to %1").arg(_serial->portName()));
		_heartbeat_timer->start(kHeartbeatTimeout);
		_serial_port_state = SerialPortState::Connected;
	} else {
		// QMessageBox::critical(this, tr("Error"), _serial->errorString());
		_serial_status_label->setText(tr("Open error"));
	}
}

void MainWindow::_closeSerialPort() {
	if (_serial->isOpen()) {
		_serial->close();
		qDebug() << "Serial disconnected";
	}
}

void MainWindow::_handleAutopilotConnection() {
	_autopilot->setState(AutopilotState::Alive);
}

void MainWindow::_handleAutopilotStateUpdate(const AutopilotState &new_state) {
	switch (new_state) {
	case AutopilotState::None: {
		_ap_status_label->setText(tr("Autopilot disconnected"));
		_action_open_settings->setEnabled(false);
		_action_open_ap_params->setEnabled(false);
		_action_open_mavftp_page->setEnabled(false);
	} break;
	case AutopilotState::Alive: {
		_serial_reconnect_timer->stop();
		_ap_status_label->setText(tr("Autopilot connected"));
		_action_open_settings->setEnabled(true);
		_action_open_ap_params->setEnabled(true);
		_action_open_mavftp_page->setEnabled(true);
		_mavlink_manager->sendCmdLong(MAV_CMD_DO_SEND_BANNER, 0);
	} break;
	case AutopilotState::Flashing: {

	} break;
	}
}

void MainWindow::_handleParamsResetRequest() {
	_rebootAp();
	_serial_reconnect_delay_timer->start(kSerialReconnectDelayTimeout);
}

void MainWindow::_handleFullSetupCompletion(const FullSetupResult &result) {
	_serial_reconnect_delay_timer->start(kSerialReconnectDelayTimeout);
	switch (result) {
	case FullSetupResult::Success: {
		QMessageBox::information(nullptr, tr("Setup completed"),
														 tr("Setup done successfully"));
	} break;
	case FullSetupResult::FcFirmwareNotFound: {
		QMessageBox::warning(nullptr, tr("Setup failed"),
												 tr("Autopilot firmware file not found"));
	} break;
	case FullSetupResult::MultipleFcFirmwareFilesFound: {
		QMessageBox::warning(nullptr, tr("Setup failed"),
												 tr("Found multiple autopilot firmware files"));
	} break;
	case FullSetupResult::ParamFileNotFound: {
		QMessageBox::warning(nullptr, tr("Setup failed"),
												 tr("Parameters file not found"));
	} break;
	case FullSetupResult::MultipleParamFilesFound: {
		QMessageBox::warning(nullptr, tr("Setup failed"),
												 tr("Found multiple parameters files"));
	} break;
	case FullSetupResult::LuaFilesNotFound: {
		QMessageBox::warning(nullptr, tr("Setup failed"),
												 tr("Lua files not found"));
	} break;
	case FullSetupResult::FirmwareUploadError: {
		QMessageBox::warning(nullptr, tr("Setup failed"),
												 tr("Firmware upload error"));
	} break;
	case FullSetupResult::ParametersUploadError: {
		QMessageBox::warning(nullptr, tr("Setup failed"),
												 tr("Parameters upload error"));
	} break;
	case FullSetupResult::LuaFilesUploadError: {
		QMessageBox::warning(nullptr, tr("Setup failed"),
												 tr("Lua files upload error"));
	} break;
	case FullSetupResult::FileReadError: {
		QMessageBox::warning(nullptr, tr("Setup failed"),
												 tr("Tar file read error"));
	} break;
	}
}

void MainWindow::_rebootAp() {
	qDebug() << "REBOOT AUTOPILOT";
	const auto command = MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN;
	const uint8_t confirmation = 0;
	const float param1 = 1;
	_mavlink_manager->sendCmdLong(command, confirmation, param1);
	// _serial->flush();
	_serial->waitForBytesWritten(500);
	_autopilot->setState(AutopilotState::None);
	_heartbeat_timer->stop();
	_disconnect();
}

void MainWindow::_handleFirmwareUploadStart(/*DroneType drone_type*/) {
	qDebug() << std::format("Upload firmware");
	_serial_reconnect_timer->stop();
	_disconnect();
	_autopilot->setState(AutopilotState::Flashing);
	_heartbeat_timer->stop();
}

void MainWindow::_handleFirmwareUploadCompletion(
		/*FirmwareUploadResult result*/) {
	_serial_reconnect_delay_timer->start(kSerialReconnectDelayTimeout);

	// auto error_title = QString();
	// auto error_msg = QString();
	// switch (result) {
	// // Firmware file errors
	// case FirmwareUploadResult::FirmwareImageNotFound: {
	// 	error_title = tr("Firmware file error");
	// 	error_msg = tr("Firmware image not found");
	// } break;
	// case FirmwareUploadResult::FirmwareSizeNotFound: {
	// 	error_title = tr("Firmware file error");
	// 	error_msg = tr("Firmware size not found");
	// } break;
	// case FirmwareUploadResult::BoardIdNotFound: {
	// 	error_title = tr("Firmware file error");
	// 	error_msg = tr("Board id not found");
	// } break;
	// case FirmwareUploadResult::DecodeFail: {
	// 	error_title = tr("Firmware file error");
	// 	error_msg = tr("Image decoding error");
	// } break;
	// // Bootloader errors
	// case FirmwareUploadResult::BootloaderNotFound: {
	// 	error_title = tr("Bootloader error");
	// 	error_msg = tr("Bootloader not found");
	// } break;
	// case FirmwareUploadResult::IncompatibleBoardType: {
	// 	error_title = tr("Bootloader error");
	// 	error_msg = tr("Incompatible board");
	// } break;
	// case FirmwareUploadResult::UnsupportedBoard: {
	// 	error_title = tr("Bootloader error");
	// 	error_msg = tr("Unsupported board");
	// } break;
	// case FirmwareUploadResult::UnsupportedBootloader: {
	// 	error_title = tr("Bootloader error");
	// 	error_msg = tr("Unsupported bootloader");
	// } break;
	// // Erase errors
	// case FirmwareUploadResult::EraseFail: {
	// 	error_title = tr("Erase error");
	// 	error_msg = tr("Board erase failed");
	// } break;
	// // Flashing errors
	// case FirmwareUploadResult::ProgramFail: {
	// 	error_title = tr("Program error");
	// 	error_msg = tr("Firmware program failed");
	// } break;
	// case FirmwareUploadResult::TooLargeFirmware: {
	// 	error_title = tr("Program error");
	// 	error_msg = tr("Too large firmware size");
	// } break;
	// // Verification errors
	// case FirmwareUploadResult::VerificationFail: {
	// 	error_title = tr("Verification error");
	// 	error_msg = tr("Firmware verification failed");
	// } break;
	// // Serial errors
	// case FirmwareUploadResult::SerialPortError: {
	// 	error_title = tr("Serial port error");
	// 	error_msg = tr("Serial port error");
	// } break;
	// // Successful upload
	// case FirmwareUploadResult::Ok: {
	// 	return;
	// }
	// }
	// if (!error_title.isEmpty() || !error_msg.isEmpty()) {
	// 	QMessageBox::critical(this, tr("Firmware upload error"), error_msg);
	// }
	// _autopilot->setState(AutopilotState::None);
	// _heartbeat_timer->start(kHeartbeatTimeout);
}
