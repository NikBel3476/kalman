#ifndef AUTOPILOTSETTINGSPAGE_H
#define AUTOPILOTSETTINGSPAGE_H

#include <QGridLayout>
#include <QWidget>

#include <ardupilotmega/mavlink.h>
// #include <kalman/ExtendedKalmanFilter.hpp>
#include <numeric>

#include "accelerometerinfowidget.hpp"
#include "avionicswidget.hpp"
#include "gyroscopeinfowidget.hpp"
#include "magnetometerinfowidget.hpp"
#include "mavlinkmanager.hpp"
#include "mcuinfowidget.hpp"
#include "sensor.hpp"
#include "simplekalmanfilter.hpp"

// template <typename T> class State : public Kalman::Vector<T, 1> {
// public:
// 	KALMAN_VECTOR(State, T, 1)

// 	//! altitude
// 	static constexpr float _altitude = 0;

// 	T altitude() const {
// 		return (*this)[_altitude];
// 	}
// 	T &altitude() {
// 		return (*this)[_altitude];
// 	}
// };

// template <typename T> class Control : public Kalman::Vector<T, 1> {
// public:
// 	KALMAN_VECTOR(Control, T, 1)

// 	//! Altitude
// 	static constexpr float _altitude = 0;

// 	T altitude() const {
// 		return (*this)[_altitude];
// 	}
// 	T &altitude() {
// 		return (*this)[_altitude];
// 	}
// };

// template <typename T,
// 					template <class> class CovarianceBase = Kalman::StandardBase>
// class SystemModel : public Kalman::LinearizedSystemModel<State<T>,
// Control<T>, CovarianceBase> { public:
// 	//! State type shortcut definition
// 	typedef State<T> S;

// 	//! Control type shortcut definition
// 	typedef Control<T> C;

// 	S f(const S &x, const C &u) const {
// 		//! Predicted state vector after transition
// 		S x_;

// 		x_.x() += u.altitude();

// 		// Return transitioned state vector
// 		return x_;
// 	}

// protected:
// 	void updateJacobians(const S &x, const C &u) {}
// };

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
	// Kalman::ExtendedKalmanFilter<State<float>> *_ekf = nullptr;
	// SystemModel<float> _altitude_system_model;
	// State<float> _altitude_state;
	// Control<float> _altitude_control;
	SimpleKalmanFilter *_altitude_kalman_filter = nullptr;
	std::vector<float> _altitude_last_values;
};

#endif // AUTOPILOTSETTINGSPAGE_H
