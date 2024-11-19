#include "autopilotsettingspage.h"

AutopilotSettingsPage::AutopilotSettingsPage(QWidget *parent,
																						 MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_magnetometer_info_widget(
					new MagnetometerInfoWidget(this, mavlink_manager)),
			_accelerometer_info_widget(
					new AccelerometerInfoWidget(this, mavlink_manager)),
			_gyroscope_info_widget(new GyroscopeInfoWidget(this, mavlink_manager)),
			_mcu_info_widget(new McuInfoWidget(this, mavlink_manager)),
			_avionics_widget(new AvionicsWidget(this, mavlink_manager)),
			_mavlink_manager{mavlink_manager} {
	_layout->setAlignment(Qt::AlignTop);
	_layout->addWidget(_magnetometer_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_accelerometer_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_gyroscope_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_mcu_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_avionics_widget, 0, Qt::AlignCenter);
	_layout->addStretch();
}
