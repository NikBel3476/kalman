#ifndef GYROSCOPEINFOWIDGET_H
#define GYROSCOPEINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "mavlinkmanager.hpp"
#include "sensor.hpp"

class GyroscopeInfoWidget : public QWidget {
	Q_OBJECT
public:
	explicit GyroscopeInfoWidget(QWidget *parent,
															 MavlinkManager *mavlink_manager);

signals:

private slots:
	void _handleMavlinkMessageReceive(const mavlink_message_t &);
	void _handleCalStartBtnPress();

private:
	void _handleIMUUpdate(const mavlink_scaled_imu_t &);
	void _handleIMU2Update(const mavlink_scaled_imu2_t &);
	void _handleSysStatusUpdate(const mavlink_sys_status_t &sys_status);
	void _handleCommandAck(const mavlink_command_ack_t &cmd);

	void _handleGyroCalibrationComplete();

	QVBoxLayout *_layout = nullptr;
	QLabel *_title_label = nullptr;
	QLabel *_status_label = nullptr;
	QLabel *_x_imu_label = nullptr;
	QLabel *_y_imu_label = nullptr;
	QLabel *_z_imu_label = nullptr;
	QLabel *_x_imu2_label = nullptr;
	QLabel *_y_imu2_label = nullptr;
	QLabel *_z_imu2_label = nullptr;
	QPushButton *_cal_start_btn = nullptr;
	QLabel *_cal_result_label = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
	SensorStatus _gyro_status = SensorStatus::NotFound;
	CalibrationState _cal_gyro_state = CalibrationState::None;
};

#endif // GYROSCOPEINFOWIDGET_H
