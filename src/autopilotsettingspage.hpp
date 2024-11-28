#ifndef AUTOPILOTSETTINGSPAGE_H
#define AUTOPILOTSETTINGSPAGE_H

#include <QGridLayout>
#include <QWidget>

#include <ardupilotmega/mavlink.h>

#include "accelerometerinfowidget.hpp"
#include "avionicswidget.hpp"
#include "gyroscopeinfowidget.hpp"
#include "magnetometerinfowidget.hpp"
#include "mavlinkmanager.hpp"
#include "mcuinfowidget.hpp"
#include "sensor.hpp"

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

private slots:
	void _handleMavlinkMessageReceive(const mavlink_message_t &mavlink_message);
	void _handleVfrHudUpdate(const mavlink_vfr_hud_t &);
	void
	_handleScaledPressureUpdate(const mavlink_scaled_pressure_t &scaled_pressure);

private:
	QGridLayout *_layout = nullptr;
	QLabel *_altitude_label = nullptr;
	QLabel *_scaled_pressure_label = nullptr;
	MagnetometerInfoWidget *_magnetometer_info_widget = nullptr;
	AccelerometerInfoWidget *_accelerometer_info_widget = nullptr;
	GyroscopeInfoWidget *_gyroscope_info_widget = nullptr;
	McuInfoWidget *_mcu_info_widget = nullptr;
	AvionicsWidget *_avionics_widget = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
};

#endif // AUTOPILOTSETTINGSPAGE_H
