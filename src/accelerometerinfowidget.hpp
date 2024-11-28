#ifndef ACCELEROMETERINFOWIDGET_H
#define ACCELEROMETERINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "mavlinkmanager.hpp"
#include "sensor.hpp"
#include <ardupilotmega/mavlink.h>

enum class CalibrationResult {
	Success,
	Failed
};

class AccelerometerInfoWidget : public QWidget {
	Q_OBJECT
public:
	explicit AccelerometerInfoWidget(QWidget *parent,
																	 MavlinkManager *mavlink_manager);

signals:

private slots:
	void _handleMavlinkMessageReceive(const mavlink_message_t &mavlink_message);
	void _handleAccelCalBtnPress();
	void _handleLvlCalBtnPress();

private:
	void _handleIMUUpdate(const mavlink_scaled_imu_t &scaled_imu);
	void _handleIMU2Update(const mavlink_scaled_imu2_t &scaled_imu);
	void _handleSysStatusUpdate(const mavlink_sys_status_t &sys_status);
	void _parseCommand(const mavlink_command_long_t &cmd);
	void _handleAccelCalComplete(CalibrationResult result);
	void _handleCalibrationDialogButton();
	void _handleCommandAck(const mavlink_command_ack_t &cmd);
	void _handleLvlCalComplete(const CalibrationResult &cal_result);

	QVBoxLayout *_layout = nullptr;
	QLabel *_title_label = nullptr;
	QLabel *_status_label = nullptr;
	QLabel *_accel_strength_label = nullptr;
	QLabel *_x_imu_label = nullptr;
	QLabel *_y_imu_label = nullptr;
	QLabel *_z_imu_label = nullptr;
	QLabel *_x_imu2_label = nullptr;
	QLabel *_y_imu2_label = nullptr;
	QLabel *_z_imu2_label = nullptr;
	QPushButton *_accel_cal_btn = nullptr;
	QPushButton *_lvl_cal_btn = nullptr;
	QLabel *_cal_result_label = nullptr;
	QMessageBox *_msg_cal_box = nullptr;
	QPushButton *_msg_cal_box_button = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
	SensorStatus _acc_status = SensorStatus::NotFound;
	CalibrationState _cal_state = CalibrationState::None;
	CalibrationAccelState _cal_acc_state = CalibrationAccelState::None;
	CalibrationState _cal_lvl_state = CalibrationState::None;
};

#endif // ACCELEROMETERINFOWIDGET_H
