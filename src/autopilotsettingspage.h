#ifndef AUTOPILOTSETTINGSPAGE_H
#define AUTOPILOTSETTINGSPAGE_H

#include <QVBoxLayout>
#include <QWidget>

#include "mavlink/ardupilotmega/mavlink.h"

#include "accelerometerinfowidget.h"
#include "avionicswidget.h"
#include "gyroscopeinfowidget.h"
#include "magnetometerinfowidget.h"
#include "mcuinfowidget.h"
#include "sensor.h"

class AutopilotSettingsPage : public QWidget {
	Q_OBJECT
public:
	explicit AutopilotSettingsPage(QWidget *parent = nullptr);

signals:
	void imu2Updated(mavlink_scaled_imu2_t);
	void attitudeUpdated(mavlink_attitude_t);
	void globalPositionIntUpdated(mavlink_global_position_int_t);
	void vfrHudUpdated(mavlink_vfr_hud_t);
	void gyroStatusUpdated(SensorStatus);
	void accelStatusUpdated(SensorStatus);
	void magStatusUpdated(SensorStatus);
	void startAccelCalibration();
	void accelerometerCalibrationCompleted();
	void startLevelCalibration();
	void levelCalibrationCompleted();
	void startMagCalibration();
	void cancelMagCalibration();
	void magCalProgressUpdated(mavlink_mag_cal_progress_t);
	void magCalReportUpdated(mavlink_mag_cal_report_t);
	void startGyroCalibration();
	void gyroCalibrationCompleted();

public slots:
	void handleIMUUpdate(mavlink_raw_imu_t);
	void handleImu2Update(mavlink_scaled_imu2_t);
	void handleAttitudeUpdate(mavlink_attitude_t);
	void handleGlobalPositionIntUpdate(mavlink_global_position_int_t);
	void handleVfrHudUpdate(mavlink_vfr_hud_t);
	void handlePowerStatusUpdate(mavlink_power_status_t);
	void handleMcuStatusUpdate(mavlink_mcu_status_t);
	void handleGyroStatusUpdate(SensorStatus);
	void handleAccelStatusUpdate(SensorStatus);
	void handleMagStatusUpdate(SensorStatus);
	void handleCompleteAccelerometerCalibration();
	void handleCompleteLevelCalibration();
	void handleMagCalProgressUpdate(mavlink_mag_cal_progress_t);
	void handleMagCalReportUpdate(mavlink_mag_cal_report_t);
	void handleGyroCalibrationComplete();

private slots:
	void _handleStartAccelCalibration();
	void _handleStartLevelCalibration();
	void _handleStartMagCalibration();
	void _handleCancelMagCalibration();
	void _handleStartGyroCalibration();

private:
	QVBoxLayout *_layout = nullptr;
	MagnetometerInfoWidget *_magnetometer_info_widget = nullptr;
	AccelerometerInfoWidget *_accelerometer_info_widget = nullptr;
	GyroscopeInfoWidget *_gyroscope_info_widget = nullptr;
	McuInfoWidget *_mcu_info_widget = nullptr;
	AvionicsWidget *_avionics_widget = nullptr;
};

#endif // AUTOPILOTSETTINGSPAGE_H
