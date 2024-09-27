#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QComboBox>
#include <QMainWindow>
#include <QMessageBox>
#include <QSerialPort>
#include <QStackedWidget>
#include <QtQuick/QQuickView>
#include <format>

#include "mavlink/ardupilotmega/mavlink.h"

#include "apparameterspage.h"
#include "authenticationform.h"
#include "autopilotsettingspage.h"
#include "firmwareuploadpage.h"
#include "sensor.h"

QT_BEGIN_NAMESPACE

class QLabel;
class QTimer;

QT_END_NAMESPACE

class Console;

enum class SerialPortState { Connected, Disconnected };
enum class AutopilotState { None, Alive };
enum class AutopilotParamsState { None, Received };
enum class AutopilotParamsSendState { None, Sending };
enum class CalibrationState { None, InProgress };
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
enum class CalibrationLevelState { None, InProgress };
enum class CalibrationMagState { None, InProgress };

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
	void accelerometerCalibrationCompleted();
	void levelCalibrationCompleted();
	void magCalProgressUpdated(mavlink_mag_cal_progress_t);
	void magCalReportUpdated(mavlink_mag_cal_report_t);
	void gyroStatusUpdated(SensorStatus);
	void accelStatusUpdated(SensorStatus);
	void magStatusUpdated(SensorStatus);
	void gyroCalibrationCompleted();
	void apParamValueReceived(mavlink_param_value_t);

private slots:
	void fillPortsInfo();
	void openSerialPort();
	void closeSerialPort();
	void writeData(const QByteArray &);
	void readData();

	void handleError(QSerialPort::SerialPortError);
	void handleBytesWritten(qint64);
	void handleWriteTimeout();
	void handleHeartbeatTimeout();
	void handleCalibrationDialogButton();

	void handleLogin(const QString &, const QString &);
	void handleFirmwareUpload();
	void _openApParamsPage();
	void _openSettingsPage();
	void _getParameterList();
	void _handleStartAccelCalibration();
	void _handleStartLevelCalibration();
	void _handleStartMagCalibration();
	void _handleCancelMagCalibration();
	void _handleStartGyroCalibration();
	void _handleApAllParamsReceive();
	void _handleUploadApParamsRequest(std::vector<mavlink_param_value_t>);

private:
	void initActionsConnections();
	void initSerialPortEventsConnections();

	void showStatusMessage(const QString &);
	void showWriteError(const QString &);
	void setPortSettings(int);
	void initPortsBoxEventsConnections();
	void parseCommand(const mavlink_command_long_t &);
	void reset();
	void _handleCommandAck(mavlink_command_ack_t &);
	void _updateSensorsStatus(mavlink_sys_status_t);
	void _handleApParamReceive(mavlink_param_value_t);
	void _uploadApParam();

	QToolBar *_toolbar = nullptr;
	QComboBox *_ports_box = nullptr;
	QAction *_action_refresh = nullptr;
	QAction *_action_connect = nullptr;
	QAction *_action_disconnect = nullptr;
	QAction *_action_clear = nullptr;
	QAction *_action_open_settings = nullptr;
	QAction *_action_open_ap_params = nullptr;
	QStackedWidget *_central_widget = nullptr;
	QStatusBar *_statusbar = nullptr;
	QLabel *_serial_status_label = nullptr;
	QLabel *_ap_status_label = nullptr;
	Console *_console = nullptr;
	AuthenticationForm *_authentication_form = nullptr;
	QMessageBox *_msg_cal_box = nullptr;
	QPushButton *_msg_cal_box_button = nullptr;

	FirmwareUploadPage *_firmware_upload_page = nullptr;
	AutopilotSettingsPage *_autopilot_settings_page = nullptr;
	ApParametersPage *_ap_params_page = nullptr;
	// QQuickView *_qml_view = nullptr;
	// QWidget *_qml_container = nullptr;

	qint64 _bytesToWrite = 0;
	QTimer *_timer = nullptr;
	QSerialPort *_serial = nullptr;
	mavlink_message_t _mavlink_message;
	mavlink_status_t _mavlink_status;
	Settings _port_settings;
	QTimer *_heartbeat_timer = nullptr;
	QTimer *_send_param_timer = nullptr;
	std::vector<mavlink_param_value_t> _params_to_upload;

	CalibrationState _cal_state = CalibrationState::None;
	CalibrationLevelState _cal_lvl_state = CalibrationLevelState::None;
	CalibrationAccelState _cal_accel_state = CalibrationAccelState::None;
	CalibrationMagState _cal_mag_state = CalibrationMagState::None;
	CalibrationState _cal_gyro_state = CalibrationState::None;
	SerialPortState _serial_port_state = SerialPortState::Disconnected;
	SensorStatus _gyro_status = SensorStatus::NotFound;
	SensorStatus _accel_status = SensorStatus::NotFound;
	SensorStatus _mag_status = SensorStatus::NotFound;
	AutopilotState _ap_state = AutopilotState::None;
	AutopilotParamsState _ap_params_state = AutopilotParamsState::None;
	AutopilotParamsSendState _ap_params_send_state =
			AutopilotParamsSendState::None;
};

#endif // MAINWINDOW_H
