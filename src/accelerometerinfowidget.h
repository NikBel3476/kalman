#ifndef ACCELEROMETERINFOWIDGET_H
#define ACCELEROMETERINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "mavlinkmanager.h"
#include "sensor.h"
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
	void startAccelCal();
	void startLevelCal();

public slots:
	void handleAccelCalComplete(CalibrationResult);
	void handleLvlCalComplete();

private slots:
	void _handleMavlinkMessageReceive(const mavlink_message_t &mavlink_message);
	void _handleAccelCalBtnPress();
	void _handleLvlCalBtnPress();

private:
	void _handleIMU2Update(const mavlink_scaled_imu2_t &scaled_imu);
	void _handleSysStatusUpdate(const mavlink_sys_status_t &sys_status);

	QVBoxLayout *_layout = nullptr;
	QLabel *_title_label = nullptr;
	QLabel *_status_label = nullptr;
	QLabel *_x_label = nullptr;
	QLabel *_y_label = nullptr;
	QLabel *_z_label = nullptr;
	QPushButton *_accel_cal_btn = nullptr;
	QPushButton *_lvl_cal_btn = nullptr;
	QLabel *_cal_result_label = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
	SensorStatus _acc_status = SensorStatus::NotFound;
};

#endif // ACCELEROMETERINFOWIDGET_H
