#include "autopilotsettingspage.hpp"

AutopilotSettingsPage::AutopilotSettingsPage(QWidget *parent,
																						 MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QGridLayout(this)),
			_altitude_label(new QLabel(tr("Altitude: %1 m").arg(0))),
			_scaled_pressure_label(new QLabel(tr("Absolute pressure: %1").arg(0))),
			_magnetometer_info_widget(
					new MagnetometerInfoWidget(this, mavlink_manager)),
			_accelerometer_info_widget(
					new AccelerometerInfoWidget(this, mavlink_manager)),
			_gyroscope_info_widget(new GyroscopeInfoWidget(this, mavlink_manager)),
			_mcu_info_widget(new McuInfoWidget(this, mavlink_manager)),
			_avionics_widget(new AvionicsWidget(this, mavlink_manager)),
			_mavlink_manager{mavlink_manager},
			_ekf{new Kalman::ExtendedKalmanFilter<State<float>>()} {
	_layout->addWidget(_magnetometer_info_widget, 0, 0, 2, 0, Qt::AlignLeft);
	_layout->addWidget(_altitude_label, 0, 1);
	_layout->addWidget(_scaled_pressure_label, 1, 1, Qt::AlignLeft);
	_layout->addWidget(_accelerometer_info_widget, 2, 0, Qt::AlignLeft);
	_layout->addWidget(_gyroscope_info_widget, 2, 1, Qt::AlignLeft);
	_layout->addWidget(_mcu_info_widget, 3, 0, 1, -1, Qt::AlignLeft);
	_layout->addWidget(_avionics_widget, 4, 0, -1, -1, Qt::AlignCenter);
	_layout->setSpacing(5);
	_layout->setContentsMargins(5, 0, 0, 0);
	_layout->setAlignment(Qt::AlignTop);

	_altitude_state.setZero();
	_ekf->init(_altitude_state);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&AutopilotSettingsPage::_handleMavlinkMessageReceive);
}

void AutopilotSettingsPage::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_VFR_HUD: {
		mavlink_vfr_hud_t vfr_hud;
		mavlink_msg_vfr_hud_decode(&mavlink_message, &vfr_hud);
		_handleVfrHudUpdate(vfr_hud);
	} break;
	case MAVLINK_MSG_ID_SCALED_PRESSURE: {
		mavlink_scaled_pressure_t scaled_pressure;
		mavlink_msg_scaled_pressure_decode(&mavlink_message, &scaled_pressure);
		_handleScaledPressureUpdate(scaled_pressure);
	} break;
	}
}

void AutopilotSettingsPage::_handleVfrHudUpdate(
		const mavlink_vfr_hud_t &vfr_hud) {
	// _altitude_control.altitude() = vfr_hud.alt;
	// _altitude_state = _altitude_system_model.f(_altitude_state,
	// _altitude_control); const auto altitude_ekf =
	// _ekf->predict(_altitude_system_model, _altitude_control); qDebug() << "ALT:
	// " << vfr_hud.alt << "\nEKF" << altitude_ekf.altitude();
	// _altitude_label->setText(tr("Altitude: %1
	// m").arg(altitude_ekf.altitude()));
	_altitude_last_values.push_back(vfr_hud.alt);
	static constexpr auto altitude_values_max_size = 8;
	if (_altitude_last_values.size() >= altitude_values_max_size) {
		const auto altitude_sum = std::reduce(_altitude_last_values.cbegin(),
																					_altitude_last_values.cend());
		_altitude_label->setText(
				tr("Altitude: %1 m")
						.arg(altitude_sum / static_cast<float>(altitude_values_max_size)));
		_altitude_last_values.clear();
	}
}

void AutopilotSettingsPage::_handleScaledPressureUpdate(
		const mavlink_scaled_pressure_t &scaled_pressure) {
	_scaled_pressure_label->setText(
			tr("Absolute pressure: %1").arg(scaled_pressure.press_abs));
}
