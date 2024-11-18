#ifndef MCUINFOWIDGET_H
#define MCUINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "mavlinkmanager.h"
#include <ardupilotmega/mavlink.h>

class McuInfoWidget : public QWidget {
	Q_OBJECT
public:
	explicit McuInfoWidget(QWidget *parent, MavlinkManager *mavlink_manager);

signals:

private slots:
	void _handleMavlinkMessageReceive(const mavlink_message_t &);

private:
	void _handlePowerStatusUpdate(const mavlink_power_status_t &);
	void _handleMcuStatusUpdate(const mavlink_mcu_status_t &);

	QVBoxLayout *m_layout = nullptr;
	QLabel *m_temperature_label = nullptr;
	QLabel *m_voltage_label = nullptr;
	QLabel *m_rail_voltage_label = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
};

#endif // MCUINFOWIDGET_H
