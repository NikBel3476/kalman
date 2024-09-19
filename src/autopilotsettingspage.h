#ifndef AUTOPILOTSETTINGSPAGE_H
#define AUTOPILOTSETTINGSPAGE_H

#include <QVBoxLayout>
#include <QWidget>

#include "mavlink/ardupilotmega/mavlink.h"

#include "accelerometerinfowidget.h"
#include "gyroscopeinfowidget.h"
#include "magnetometerinfowidget.h"
#include "mcuinfowidget.h"

class AutopilotSettingsPage : public QWidget {
	Q_OBJECT
public:
	explicit AutopilotSettingsPage(QWidget *parent = nullptr);

signals:
	void startCalibration();
	void accelerometerCalibrationCompleted();
	void startLevelCalibration();
	void levelCalibrationCompleted();
	void startMagCalibration();
	void cancelMagCalibration();
	void magCalProgressUpdated(mavlink_mag_cal_progress_t mag_cal_progress);
	void magCalReportUpdated(mavlink_mag_cal_report_t mag_cal_report);

public slots:
	void handleIMUUpdate(mavlink_raw_imu_t raw_imu);
	void handlePowerStatusUpdate(mavlink_power_status_t power_status);
	void handleMcuStatusUpdate(mavlink_mcu_status_t mcu_status);
	void handleCompleteAccelerometerCalibration();
	void handleCompleteLevelCalibration();
	void handleMagCalProgressUpdate(mavlink_mag_cal_progress_t mag_cal_progress);
	void handleMagCalReportUpdate(mavlink_mag_cal_report_t mag_cal_report);

private slots:
	void _handleStartCalibration();
	void _handleStartLevelCalibration();
	void _handleStartMagCalibration();
	void _handleCancelMagCalibration();

private:
	QVBoxLayout *_layout = nullptr;
	MagnetometerInfoWidget *_magnetometer_info_widget = nullptr;
	AccelerometerInfoWidget *_accelerometer_info_widget = nullptr;
	GyroscopeInfoWidget *_gyroscope_info_widget = nullptr;
	McuInfoWidget *_mcu_info_widget = nullptr;
};

#endif // AUTOPILOTSETTINGSPAGE_H
