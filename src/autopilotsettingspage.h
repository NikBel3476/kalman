#ifndef AUTOPILOTSETTINGSPAGE_H
#define AUTOPILOTSETTINGSPAGE_H

#include <QVBoxLayout>
#include <QWidget>

#include <ardupilotmega/mavlink.h>

#include "accelerometerinfowidget.h"
#include "avionicswidget.h"
#include "gyroscopeinfowidget.h"
#include "magnetometerinfowidget.h"
#include "mavlinkmanager.h"
#include "mcuinfowidget.h"
#include "sensor.h"

class AutopilotSettingsPage : public QWidget {
	Q_OBJECT
public:
	explicit AutopilotSettingsPage(QWidget *parent,
																 MavlinkManager *mavlink_manager);

signals:
	void imu2Updated(mavlink_scaled_imu2_t);
	void attitudeUpdated(mavlink_attitude_t);
	void globalPositionIntUpdated(mavlink_global_position_int_t);
	void vfrHudUpdated(mavlink_vfr_hud_t);
	void startAccelCalibration();
	void accelerometerCalibrationCompleted(CalibrationResult);
	void startLevelCalibration();
	void levelCalibrationCompleted();

public slots:
	void handleCompleteAccelerometerCalibration(CalibrationResult);
	void handleCompleteLevelCalibration();

private slots:
	void _handleStartAccelCalibration();
	void _handleStartLevelCalibration();

private:
	QVBoxLayout *_layout = nullptr;
	MagnetometerInfoWidget *_magnetometer_info_widget = nullptr;
	AccelerometerInfoWidget *_accelerometer_info_widget = nullptr;
	GyroscopeInfoWidget *_gyroscope_info_widget = nullptr;
	McuInfoWidget *_mcu_info_widget = nullptr;
	AvionicsWidget *_avionics_widget = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
};

#endif // AUTOPILOTSETTINGSPAGE_H
