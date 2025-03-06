#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QString>
#include <QTimer>
#include <QToolBar>
#include <QtQuick/QQuickView>
#include <algorithm>
#include <chrono>
#include <format>
#include <string>

#include <ardupilotmega/mavlink.h>

#include "apparameterspage.hpp"
#include "authenticationpage.hpp"
#include "autopilot.hpp"
#include "autopilotsettingspage.hpp"
#include "firmwareuploader.hpp"
#include "firmwareuploadpage.hpp"
#include "mavftppage.hpp"
#include "fullsetuppage.hpp"
#include "mavlinkmanager.hpp"
#include "sensor.hpp"
#include "parametersmanager.hpp"
#include "mavftpmanager.hpp"

QT_BEGIN_NAMESPACE

class QLabel;
class QTimer;

QT_END_NAMESPACE

class Console;

enum class SerialPortState {
	Connected,
	Disconnected
};
enum class CalibrationLevelState {
	None,
	InProgress
};

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

	struct Settings {
		QString name;
		qint32 baudRate;
		QSerialPort::DataBits dataBits;
		QSerialPort::Parity parity;
		QSerialPort::StopBits stopBits;
		QSerialPort::FlowControl flowControl;
	};

signals:
	void autopilotConnected();

private slots:
	void fillPortsInfo();
	void openSerialPort();
	void _closeSerialPort();
	void _handleAutopilotConnection();
	void _handleAutopilotStateUpdate(const AutopilotState &new_state);
	void _handleParamsResetRequest();
	void _handleFullSetupCompletion(const FullSetupResult &);

	void _handleSerialWriteError(const QString &error_msg);
	void handleError(QSerialPort::SerialPortError);
	void handleBytesWritten(qint64);
	void handleWriteTimeout();
	void handleHeartbeatTimeout();
	void handleSerialReconnectTimeout();

	void _login(const QString &login, const QString &password);
	void _logout();
	void _openConsole();
	void _openApParamsPage();
	void _openMavftpPage();
	void _openSettingsPage();
	void _openFirmwareUploadPage();
	void _openFullSetupPage();
	void _rebootAp();
	void _handleFirmwareUploadStart(/*DroneType*/);
	void _handleFirmwareUploadCompletion(/*FirmwareUploadResult*/);
	void _handleMavlinkMessageReceive(const mavlink_message_t &);
	void _handleApParametersWrite();
	void _handleDisconnectActionTrigger();
	void _handleRebootActionTrigger();
	void _handleAboutActionTrigger();

private:
	void initSerialPortEventsConnections();
	void showWriteError(const QString &);
	void setPortSettings(int);
	void _findBootloader();
	void initPortsBoxEventsConnections();
	void _trySerialConnect();
	void _disconnect();

	QToolBar *_toolbar = nullptr;
	QComboBox *_ports_box = nullptr;
	QAction *_action_refresh = nullptr;
	QAction *_action_connect = nullptr;
	QAction *_action_disconnect = nullptr;
	QAction *_action_clear = nullptr;
	QAction *_action_open_settings = nullptr;
	QAction *_action_open_ap_params = nullptr;
	QAction *_action_open_mavftp_page = nullptr;
	QAction *_action_open_console = nullptr;
	QAction *_action_open_firmware_page = nullptr;
	QAction *_action_open_full_setup_page = nullptr;
	QAction *_action_reboot_ap = nullptr;
	QAction *_action_about = nullptr;
	QAction *_action_logout = nullptr;
	QStackedWidget *_central_widget = nullptr;
	QStatusBar *_statusbar = nullptr;
	QLabel *_serial_status_label = nullptr;
	QLabel *_ap_status_label = nullptr;
	QLabel *_ap_os_label = nullptr;
	QLabel *_ap_name_label = nullptr;

	Autopilot *_autopilot = nullptr;
	QSerialPort *_serial = nullptr;
	MavlinkManager *_mavlink_manager = nullptr;
	ParametersManager *_parameters_manager = nullptr;
	MavFtpManager *_mav_ftp_manager = nullptr;

	Console *_console = nullptr;
	AuthenticationPage *_authentication_page = nullptr;
	FirmwareUploadPage *_firmware_upload_page = nullptr;
	AutopilotSettingsPage *_autopilot_settings_page = nullptr;
	ApParametersPage *_ap_params_page = nullptr;
	MavftpPage *_mavftp_page = nullptr;
	FullSetupPage *_full_setup_page = nullptr;
	// QQuickView *_qml_view = nullptr;
	// QWidget *_qml_container = nullptr;

	qint64 _bytesToWrite = 0;
	QTimer *_serial_write_timer = nullptr;
	int _current_port_box_index = 0;
	Settings _port_settings;
	QTimer *_heartbeat_timer = nullptr;
	QTimer *_send_param_timer = nullptr;
	QTimer *_serial_reconnect_timer = nullptr;
	QTimer *_serial_reconnect_delay_timer = nullptr;
	std::vector<mavlink_param_value_t> _params_to_upload;
	std::vector<mavlink_param_value_t> _not_written_params;
	QList<QSerialPortInfo> _port_list = {};

	CalibrationState _cal_state = CalibrationState::None;
	CalibrationLevelState _cal_lvl_state = CalibrationLevelState::None;
	CalibrationAccelState _cal_accel_state = CalibrationAccelState::None;
	SerialPortState _serial_port_state = SerialPortState::Disconnected;
};

#endif // MAINWINDOW_H
