#include "mcuinfowidget.h"

McuInfoWidget::McuInfoWidget(QWidget *parent)
		: QWidget{parent}, m_layout(new QVBoxLayout(this)),
			m_temperature_label(new QLabel()), m_voltage_label(new QLabel()),
			m_rail_voltage_label(new QLabel()) {
	auto values_layout = new QHBoxLayout();
	m_layout->addWidget(new QLabel(tr("MCU information")));
	m_layout->addLayout(values_layout);

	values_layout->addWidget(m_temperature_label);
	values_layout->addWidget(m_voltage_label);
	values_layout->addWidget(m_rail_voltage_label);
}

void McuInfoWidget::handleMcuStatusUpdate(mavlink_mcu_status_t mcu_status) {
	m_temperature_label->setText(QString(tr("Temperature: %1.%2 %3"))
																	 .arg(mcu_status.MCU_temperature / 100)
																	 .arg(mcu_status.MCU_temperature % 100)
																	 .arg(QChar(0260) + 'C'));
	m_voltage_label->setText(
			QString(tr("Voltage: %1 mV")).arg(mcu_status.MCU_voltage));
	if (mcu_status.MCU_voltage < mcu_status.MCU_voltage_min ||
			mcu_status.MCU_voltage > mcu_status.MCU_voltage_max) {
		m_voltage_label->setStyleSheet("border: 3px solid red");
	} else {
		m_voltage_label->setStyleSheet("");
	}
}

void McuInfoWidget::handlePowerStatusUpdate(uint16_t rail_voltage) {
	m_rail_voltage_label->setText(
			QString(tr("Rail voltage: %1 mV")).arg(rail_voltage));
}
