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

	// accelerometer ifno widget connections
	connect(_accelerometer_info_widget, &AccelerometerInfoWidget::startAccelCal,
					this, &AutopilotSettingsPage::_handleStartAccelCalibration);
	connect(this, &AutopilotSettingsPage::accelerometerCalibrationCompleted,
					_accelerometer_info_widget,
					&AccelerometerInfoWidget::handleAccelCalComplete);
	connect(_accelerometer_info_widget, &AccelerometerInfoWidget::startLevelCal,
					this, &AutopilotSettingsPage::_handleStartLevelCalibration);
	connect(this, &AutopilotSettingsPage::levelCalibrationCompleted,
					_accelerometer_info_widget,
					&AccelerometerInfoWidget::handleLvlCalComplete);

	// gyroscope info widget connections
	connect(_gyroscope_info_widget, &GyroscopeInfoWidget::startCalibration, this,
					&AutopilotSettingsPage::_handleStartGyroCalibration);
	connect(this, &AutopilotSettingsPage::gyroCalibrationCompleted,
					_gyroscope_info_widget,
					&GyroscopeInfoWidget::handleGyroCalibrationComplete);
}

void AutopilotSettingsPage::handleCompleteAccelerometerCalibration(
		CalibrationResult result) {
	emit accelerometerCalibrationCompleted(result);
}

void AutopilotSettingsPage::handleGyroCalibrationComplete() {
	emit gyroCalibrationCompleted();
}

void AutopilotSettingsPage::_handleStartAccelCalibration() {
	emit startAccelCalibration();
}

void AutopilotSettingsPage::_handleStartLevelCalibration() {
	emit startLevelCalibration();
}

void AutopilotSettingsPage::handleCompleteLevelCalibration() {
	emit levelCalibrationCompleted();
}

void AutopilotSettingsPage::_handleStartGyroCalibration() {
	emit startGyroCalibration();
}
