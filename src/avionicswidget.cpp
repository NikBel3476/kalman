#include "avionicswidget.h"

static constexpr auto kRedrawTimeout = std::chrono::milliseconds{50}; // 20 FPS

AvionicsWidget::AvionicsWidget(QWidget *parent)
		: QWidget{parent}, _layout(new QGridLayout(this)), _eadi(new qfi_EADI()),
			_eadi_redraw_timer(new QTimer()) {
	_layout->addWidget(_eadi);

	_eadi->setMaximumSize(300, 300);
	_eadi->setMinimumSize(300, 300);
	_eadi->setGeometry(QRect(0, 0, 300, 300));
	_eadi->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_eadi->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	_eadi->setInteractive(false);
	_eadi->setEnabled(false);

	connect(_eadi_redraw_timer, &QTimer::timeout, this, &AvionicsWidget::update);

	_eadi_redraw_timer->start(kRedrawTimeout);
}

void AvionicsWidget::update() { _eadi->redraw(); }

void AvionicsWidget::handleImu2Update(mavlink_scaled_imu2_t imu) {}

void AvionicsWidget::handleAttitudeUpdate(mavlink_attitude_t attitude) {
	const auto roll_in_deg = attitude.roll * 180.0 / M_PI;
	const auto pitch_in_deg = attitude.pitch * 180.0 / M_PI;
	_eadi->setFD(roll_in_deg, pitch_in_deg);
	_eadi->setRoll(roll_in_deg);
	_eadi->setPitch(pitch_in_deg);
}

void AvionicsWidget::handleGlobalPositionIntUpdate(
		mavlink_global_position_int_t global_position) {
	_eadi->setHeading(global_position.hdg / 100.0);
}

void AvionicsWidget::handleVfrHudUpdate(mavlink_vfr_hud_t vfr_hud) {
	_eadi->setAirspeed(vfr_hud.airspeed);
}
