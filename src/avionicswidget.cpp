#include "avionicswidget.h"

static constexpr auto kRedrawTimeout = std::chrono::milliseconds{50}; // 20 FPS

AvionicsWidget::AvionicsWidget(QWidget *parent, MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QGridLayout(this)),
			_eadi(new qfi_EADI()),
			_eadi_redraw_timer(new QTimer()),
			_mavlink_manager{mavlink_manager} {
	_layout->addWidget(_eadi);

	_eadi->setMaximumSize(500, 500);
	_eadi->setMinimumSize(500, 500);
	_eadi->setGeometry(QRect(0, 0, 500, 500));
	_eadi->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_eadi->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_eadi->setInteractive(false);
	_eadi->setEnabled(false);

	connect(_eadi_redraw_timer, &QTimer::timeout, this, &AvionicsWidget::update);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&AvionicsWidget::_handleMavlinkMessageReceive);

	_eadi_redraw_timer->start(kRedrawTimeout);
}

void AvionicsWidget::update() {
	_eadi->redraw();
}

void AvionicsWidget::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_ATTITUDE: {
		mavlink_attitude_t attitude;
		mavlink_msg_attitude_decode(&mavlink_message, &attitude);
		_handleAttitudeUpdate(attitude);
	} break;
	case MAVLINK_MSG_ID_GLOBAL_POSITION_INT: {
		mavlink_global_position_int_t global_position;
		mavlink_msg_global_position_int_decode(&mavlink_message, &global_position);
		_handleGlobalPositionIntUpdate(global_position);
	} break;
	case MAVLINK_MSG_ID_VFR_HUD: {
		mavlink_vfr_hud_t vfr_hud;
		mavlink_msg_vfr_hud_decode(&mavlink_message, &vfr_hud);
		_handleVfrHudUpdate(vfr_hud);
	} break;
	}
}

void AvionicsWidget::_handleAttitudeUpdate(const mavlink_attitude_t &attitude) {
	const auto roll_in_deg = attitude.roll * 180.0 / M_PI;
	const auto pitch_in_deg = attitude.pitch * 180.0 / M_PI;
	_eadi->setFD(roll_in_deg, pitch_in_deg);
	_eadi->setRoll(roll_in_deg);
	_eadi->setPitch(pitch_in_deg);
}

void AvionicsWidget::_handleGlobalPositionIntUpdate(
		const mavlink_global_position_int_t global_position) {
	_eadi->setHeading(global_position.hdg / 100.0);
}

void AvionicsWidget::_handleVfrHudUpdate(const mavlink_vfr_hud_t vfr_hud) {
	_eadi->setAirspeed(vfr_hud.airspeed);
}
