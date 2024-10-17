#ifndef MCUINFOWIDGET_H
#define MCUINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <ardupilotmega/mavlink.h>

class McuInfoWidget : public QWidget {
	Q_OBJECT
public:
	explicit McuInfoWidget(QWidget *parent = nullptr);

signals:

public slots:
	void handleMcuStatusUpdate(mavlink_mcu_status_t mcu_status);
	void handlePowerStatusUpdate(uint16_t rail_voltage);

private:
	QVBoxLayout *m_layout = nullptr;
	QLabel *m_temperature_label = nullptr;
	QLabel *m_voltage_label = nullptr;
	QLabel *m_rail_voltage_label = nullptr;
};

#endif // MCUINFOWIDGET_H
