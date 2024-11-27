#include "autopilotsettingspage.hpp"

AutopilotSettingsPage::AutopilotSettingsPage(QWidget *parent,
																						 MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_altitude_label(new QLabel()),
			_magnetometer_info_widget(
					new MagnetometerInfoWidget(this, mavlink_manager)),
			_accelerometer_info_widget(
					new AccelerometerInfoWidget(this, mavlink_manager)),
			_gyroscope_info_widget(new GyroscopeInfoWidget(this, mavlink_manager)),
			_mcu_info_widget(new McuInfoWidget(this, mavlink_manager)),
			_avionics_widget(new AvionicsWidget(this, mavlink_manager)),
			_mavlink_manager{mavlink_manager} {
	_layout->setAlignment(Qt::AlignTop);
	_layout->addWidget(_altitude_label);
	_layout->addWidget(_magnetometer_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_accelerometer_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_gyroscope_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_mcu_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_avionics_widget, 0, Qt::AlignCenter);
	_layout->addStretch();
	_layout->setSpacing(5);
	_layout->setContentsMargins(5, 0, 0, 0);

	_altitude_label->setText(tr("Altitude: %1").arg(0));

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
	}
}

void AutopilotSettingsPage::_handleVfrHudUpdate(
		const mavlink_vfr_hud_t &vfr_hud) {
	_altitude_label->setText(tr("Altitude: %1").arg(vfr_hud.alt));
}
