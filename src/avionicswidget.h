#ifndef AVIONICSWIDGET_H
#define AVIONICSWIDGET_H

#include <QGridLayout>
#include <QTimer>
#include <QWidget>
#include <cmath>

#include "mavlinkmanager.h"
#include <ardupilotmega/mavlink.h>
#include <qfi_EADI.h>

class AvionicsWidget : public QWidget {
	Q_OBJECT
public:
	explicit AvionicsWidget(QWidget *parent, MavlinkManager *mavlink_manager);

signals:

public slots:
	void update();

private slots:
	void _handleMavlinkMessageReceive(const mavlink_message_t &mavlink_message);

private:
	void _handleAttitudeUpdate(const mavlink_attitude_t &attitude);
	void _handleGlobalPositionIntUpdate(
			const mavlink_global_position_int_t global_position);
	void _handleVfrHudUpdate(const mavlink_vfr_hud_t vfr_hud);

	QGridLayout *_layout = nullptr;
	qfi_EADI *_eadi = nullptr;
	QTimer *_eadi_redraw_timer = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
};

#endif // AVIONICSWIDGET_H
