#include "mcuinfowidget.hpp"

McuInfoWidget::McuInfoWidget(QWidget *parent, MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_temperature_label(new QLabel(
					tr("Temperature: %1.%2 %3").arg(0).arg(0).arg(QChar(0260) + 'C'))),
			_voltage_label(new QLabel(tr("Voltage: %1 mV").arg(0))),
			_rail_voltage_label(new QLabel(tr("Rail voltage: %1 mV").arg(0))),
			_mavlink_manager{mavlink_manager} {
	auto values_layout = new QHBoxLayout();
	_layout->addWidget(new QLabel(tr("MCU information")));
	_layout->addLayout(values_layout);
	_layout->setSpacing(0);
	_layout->setContentsMargins(0, 0, 0, 0);

	values_layout->addWidget(_temperature_label);
	values_layout->addWidget(_voltage_label);
	values_layout->addWidget(_rail_voltage_label);

	static constexpr auto label_min_width = 150;
	_temperature_label->setMinimumWidth(label_min_width);
	_voltage_label->setMinimumWidth(label_min_width);
	_rail_voltage_label->setMinimumWidth(label_min_width);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&McuInfoWidget::_handleMavlinkMessageReceive);
}

void McuInfoWidget::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_POWER_STATUS: {
		mavlink_power_status_t power_status;
		mavlink_msg_power_status_decode(&mavlink_message, &power_status);
		_handlePowerStatusUpdate(power_status);
	} break;
	case MAVLINK_MSG_ID_MCU_STATUS: {
		mavlink_mcu_status_t mcu_status;
		mavlink_msg_mcu_status_decode(&mavlink_message, &mcu_status);
		_handleMcuStatusUpdate(mcu_status);
	} break;
	}
}

void McuInfoWidget::_handlePowerStatusUpdate(
		const mavlink_power_status_t &power_status) {
	_rail_voltage_label->setText(tr("Rail voltage: %1 mV").arg(power_status.Vcc));
}

void McuInfoWidget::_handleMcuStatusUpdate(
		const mavlink_mcu_status_t &mcu_status) {
	_temperature_label->setText(tr("Temperature: %1.%2 %3")
																	.arg(mcu_status.MCU_temperature / 100)
																	.arg(mcu_status.MCU_temperature % 100)
																	.arg(QChar(0260) + 'C'));
	_voltage_label->setText(tr("Voltage: %1 mV").arg(mcu_status.MCU_voltage));
	if (mcu_status.MCU_voltage < mcu_status.MCU_voltage_min ||
			mcu_status.MCU_voltage > mcu_status.MCU_voltage_max) {
		_voltage_label->setStyleSheet("border: 3px solid red");
	} else {
		_voltage_label->setStyleSheet("");
	}
}
