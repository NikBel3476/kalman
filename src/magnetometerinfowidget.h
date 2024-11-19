#ifndef MAGNETOMETERINFOWIDGET_H
#define MAGNETOMETERINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "mavlinkmanager.h"
#include "sensor.h"
#include <ardupilotmega/mavlink.h>

class MagnetometerInfoWidget : public QWidget {
	Q_OBJECT
public:
	explicit MagnetometerInfoWidget(QWidget *parent,
																	MavlinkManager *mavlink_manager);

signals:

private slots:
	void _handleMavlinkMessageReceive(const mavlink_message_t &);
	void _handleCalCancelButtonPress();
	void _handleCalStartButtonPress();

private:
	void _handleIMU2Update(const mavlink_scaled_imu2_t &);
	void _handleSysStatusUpdate(const mavlink_sys_status_t &sys_status);
	void _handleMagCalProgressUpdate(
			const mavlink_mag_cal_progress_t &mag_cal_progress);
	void
	_handleMagCalReportUpdate(const mavlink_mag_cal_report_t &mag_cal_report);

	QVBoxLayout *_layout = nullptr;
	QLabel *_title_label = nullptr;
	QLabel *_status_label = nullptr;
	QLabel *_x_label = nullptr;
	QLabel *_y_label = nullptr;
	QLabel *_z_label = nullptr;
	QPushButton *_start_calibration_button = nullptr;
	QPushButton *_cancel_calibration_button = nullptr;
	QWidget *_cal_progress_container = nullptr;
	QLabel *_cal_attempt_label = nullptr;
	QLabel *_cal_step_label = nullptr;
	QProgressBar *_mag_cal_progress_bar = nullptr;
	QLabel *_cal_result_label = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
	SensorStatus _mag_status = SensorStatus::NotFound;
	CalibrationState _cal_mag_state = CalibrationState::None;
	uint8_t _cal_attempt = 0;
	uint8_t _cal_step = 0;
};

#endif // MAGNETOMETERINFOWIDGET_H
