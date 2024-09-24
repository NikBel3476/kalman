#ifndef AVIONICSWIDGET_H
#define AVIONICSWIDGET_H

#include <QGridLayout>
#include <QTimer>
#include <QWidget>
#include <cmath>

#include "mavlink/ardupilotmega/mavlink.h"
#include "qflightinstruments/src/qfi/qfi_EADI.h"

class AvionicsWidget : public QWidget {
	Q_OBJECT
public:
	explicit AvionicsWidget(QWidget *parent = nullptr);

signals:

public slots:
	void handleImu2Update(mavlink_scaled_imu2_t);
	void handleAttitudeUpdate(mavlink_attitude_t);
	void handleGlobalPositionIntUpdate(mavlink_global_position_int_t);
	void update();

private:
	QGridLayout *_layout = nullptr;
	qfi_EADI *_eadi = nullptr;
};

#endif // AVIONICSWIDGET_H
