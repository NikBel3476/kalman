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
			_mavlink_manager{mavlink_manager} {
	// _layout->setAlignment(Qt::AlignTop);
	_layout->addWidget(_altitude_label, 0, 1);
	_layout->addWidget(_scaled_pressure_label, 1, 1, Qt::AlignLeft);
	_layout->addWidget(_magnetometer_info_widget, 0, 0, 2, 0, Qt::AlignLeft);
	_layout->addWidget(_accelerometer_info_widget, 2, 0, Qt::AlignLeft);
	_layout->addWidget(_gyroscope_info_widget, 2, 1, Qt::AlignLeft);
	_layout->addWidget(_mcu_info_widget, 3, 0, 1, -1, Qt::AlignLeft);
	_layout->addWidget(_avionics_widget, 4, 0, -1, -1, Qt::AlignCenter);
	_layout->setSpacing(5);
	_layout->setContentsMargins(5, 0, 0, 0);

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
	_altitude_label->setText(tr("Altitude: %1 m").arg(vfr_hud.alt));
}

void AutopilotSettingsPage::_handleScaledPressureUpdate(
		const mavlink_scaled_pressure_t &scaled_pressure) {
	_scaled_pressure_label->setText(
			tr("Absolute pressure: %1").arg(scaled_pressure.press_abs));
}
