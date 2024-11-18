#ifndef GYROSCOPEINFOWIDGET_H
#define GYROSCOPEINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "mavlinkmanager.h"
#include "sensor.h"

class GyroscopeInfoWidget : public QWidget {
	Q_OBJECT
public:
	explicit GyroscopeInfoWidget(QWidget *parent,
															 MavlinkManager *mavlink_manager);

signals:
	void startCalibration();

public slots:
	void handleGyroCalibrationComplete();

private slots:
	void _handleMavlinkMessageReceive(const mavlink_message_t &);
	void _handleCalStartBtnPress();

private:
	void _handleIMU2Update(const mavlink_scaled_imu2_t &);
	void _handleSysStatusUpdate(const mavlink_sys_status_t &sys_status);

	QVBoxLayout *_layout = nullptr;
	QLabel *_title_label = nullptr;
	QLabel *_status_label = nullptr;
	QLabel *_x_label = nullptr;
	QLabel *_y_label = nullptr;
	QLabel *_z_label = nullptr;
	QPushButton *_cal_start_btn = nullptr;
	QLabel *_cal_result_label = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
	SensorStatus _gyro_status = SensorStatus::NotFound;
};

#endif // GYROSCOPEINFOWIDGET_H
