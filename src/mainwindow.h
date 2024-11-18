#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QSerialPort>
#include <QShortcut>
#include <QStackedWidget>
#include <QtQuick/QQuickView>
#include <format>

#include <ardupilotmega/mavlink.h>

#include "apparameterspage.h"
#include "authenticationpage.h"
#include "autopilot.h"
#include "autopilotsettingspage.h"
#include "firmwareuploader.h"
#include "firmwareuploadpage.h"
#include "mavlinkmanager.h"
#include "sensor.h"

QT_BEGIN_NAMESPACE

class QLabel;
class QTimer;

QT_END_NAMESPACE

class Console;

enum class SerialPortState {
	Connected,
	Disconnected
};
enum class CalibrationState {
	None,
	InProgress
};
enum class CalibrationAccelState {
	None,
	Level,
	LevelDone,
	LeftSide,
	LeftSideDone,
	RightSide,
	RightSideDone,
	NoseUp,
	NoseUpDone,
	NoseDown,
	NoseDownDone,
	Back,
	BackDone
};
enum class CalibrationLevelState {
	None,
	InProgress
};
enum class CalibrationMagState {
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
	void serialConnected();
	void serialDisconnected();
	void IMUUpdated(mavlink_raw_imu_t);
	void imu2Updated(mavlink_scaled_imu2_t);
	void attitudeUpdated(mavlink_attitude_t);
	void vfrHudUpdated(mavlink_vfr_hud_t);
	void globalPositionIntUpdated(mavlink_global_position_int_t);
	void powerStatusUpdated(mavlink_power_status_t);
	void mcuStatusUpdated(mavlink_mcu_status_t);
	void accelerometerCalibrationCompleted(CalibrationResult);
	void levelCalibrationCompleted();
	void magCalProgressUpdated(mavlink_mag_cal_progress_t);
	void magCalReportUpdated(mavlink_mag_cal_report_t);
	void gyroCalibrationCompleted();
	// void apParamValueReceived(mavlink_param_value_t);
	void apParamsUploaded(const std::vector<mavlink_param_value_t> &);
	void apStateUpdated(AutopilotState);
	void autopilotConnected();

private slots:
	void fillPortsInfo();
	void openSerialPort();
	void closeSerialPort();

	void _handleSerialWriteError(const QString &error_msg);
	void handleError(QSerialPort::SerialPortError);
	void handleBytesWritten(qint64);
	void handleWriteTimeout();
	void handleHeartbeatTimeout();
	void handleSerialReconnectTimeout();
	void handleCalibrationDialogButton();

	void _login(const QString &login, const QString &password);
	void _logout();
	void handleFirmwareUpload(DroneType);
	void _openConsole();
	void _openApParamsPage();
	void _openSettingsPage();
	void _rebootAp();
	void _handleStartAccelCalibration();
	void _handleStartLevelCalibration();
	void _handleStartMagCalibration();
	void _handleCancelMagCalibration();
	void _handleStartGyroCalibration();
	// void _handleApAllParamsReceive();
	// void _handleUploadApParamsRequest(std::vector<mavlink_param_value_t>);
	void _handleFirmwareUploadCompletion(FirmwareUploadResult);
	void _handleMavlinkMessageReceive(const mavlink_message_t &);
	void _handleApParametersWrite();
	void _handleRebootActionTrigger();

private:
	void initActionsConnections();
	void initSerialPortEventsConnections();

	void showStatusMessage(const QString &);
	void showWriteError(const QString &);
	void setPortSettings(int);
	void _findBootloader();
	void initPortsBoxEventsConnections();
	void parseCommand(const mavlink_command_long_t &);
	void reset();
	void _handleCommandAck(mavlink_command_ack_t &);
	void _setApState(AutopilotState);
	void _trySerialConnect();

	QToolBar *_toolbar = nullptr;
	QComboBox *_ports_box = nullptr;
	QAction *_action_refresh = nullptr;
	QAction *_action_connect = nullptr;
	QAction *_action_disconnect = nullptr;
	QAction *_action_clear = nullptr;
	QAction *_action_open_settings = nullptr;
	QAction *_action_open_ap_params = nullptr;
	QAction *_action_open_console = nullptr;
	QAction *_action_reboot_ap = nullptr;
	QAction *_action_logout = nullptr;
	QStackedWidget *_central_widget = nullptr;
	QStatusBar *_statusbar = nullptr;
	QLabel *_serial_status_label = nullptr;
	QLabel *_ap_status_label = nullptr;
	Console *_console = nullptr;
	AuthenticationPage *_authentication_page = nullptr;
	QMessageBox *_msg_cal_box = nullptr;
	QPushButton *_msg_cal_box_button = nullptr;

	Autopilot *_autopilot = nullptr;
	QSerialPort *_serial = nullptr;
	MavlinkManager *_mavlink_manager = nullptr;
	FirmwareUploader *_firmware_uploader = nullptr;

	FirmwareUploadPage *_firmware_upload_page = nullptr;
	AutopilotSettingsPage *_autopilot_settings_page = nullptr;
	ApParametersPage *_ap_params_page = nullptr;
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

	CalibrationState _cal_state = CalibrationState::None;
	CalibrationLevelState _cal_lvl_state = CalibrationLevelState::None;
	CalibrationAccelState _cal_accel_state = CalibrationAccelState::None;
	CalibrationMagState _cal_mag_state = CalibrationMagState::None;
	CalibrationState _cal_gyro_state = CalibrationState::None;
	SerialPortState _serial_port_state = SerialPortState::Disconnected;
};

#endif // MAINWINDOW_H
