#include "autopilotsettingspage.h"

static const int MIN_WIDGET_WIDTH = 150;
static const int MAX_WIDGET_WIDTH = 450;

AutopilotSettingsPage::AutopilotSettingsPage(QWidget *parent)
		: QWidget{parent}, _layout(new QVBoxLayout(this)),
			_magnetometer_info_widget(new MagnetometerInfoWidget()),
			_accelerometer_info_widget(new AccelerometerInfoWidget()),
			_gyroscope_info_widget(new GyroscopeInfoWidget()),
			_mcu_info_widget(new McuInfoWidget()) {
	_layout->addWidget(_magnetometer_info_widget);
	_layout->addWidget(_accelerometer_info_widget);
	_layout->addWidget(_gyroscope_info_widget);
	_layout->addWidget(_mcu_info_widget);

	_magnetometer_info_widget->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_magnetometer_info_widget->setMinimumWidth(MIN_WIDGET_WIDTH);
	_magnetometer_info_widget->setMaximumWidth(MAX_WIDGET_WIDTH);

	_accelerometer_info_widget->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_accelerometer_info_widget->setMinimumWidth(MIN_WIDGET_WIDTH);
	_accelerometer_info_widget->setMaximumWidth(MAX_WIDGET_WIDTH);

	_gyroscope_info_widget->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_gyroscope_info_widget->setMinimumWidth(MIN_WIDGET_WIDTH);
	_gyroscope_info_widget->setMaximumWidth(MAX_WIDGET_WIDTH);

	_mcu_info_widget->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_mcu_info_widget->setMinimumWidth(MIN_WIDGET_WIDTH);
	_mcu_info_widget->setMaximumWidth(MAX_WIDGET_WIDTH);

	connect(_accelerometer_info_widget,
					&AccelerometerInfoWidget::startCalibration, this,
					&AutopilotSettingsPage::_handleStartCalibration);

	connect(_magnetometer_info_widget, &MagnetometerInfoWidget::startCalibration,
					this, &AutopilotSettingsPage::_handleStartMagCalibration);
	connect(_magnetometer_info_widget, &MagnetometerInfoWidget::cancelCalibration,
					this, &AutopilotSettingsPage::_handleCancelMagCalibration);
	connect(this, &AutopilotSettingsPage::magCalProgressUpdated,
					_magnetometer_info_widget,
					&MagnetometerInfoWidget::handleMagCalProgressUpdate);
	connect(this, &AutopilotSettingsPage::magCalReportUpdated,
					_magnetometer_info_widget,
					&MagnetometerInfoWidget::handleMagCalReportUpdate);
}

void AutopilotSettingsPage::handleIMUUpdate(mavlink_raw_imu_t raw_imu) {
	_magnetometer_info_widget->handleIMUUpdate(raw_imu.xmag, raw_imu.ymag,
																						 raw_imu.zmag);
	_accelerometer_info_widget->handleIMUUpdate(raw_imu.xacc, raw_imu.yacc,
																							raw_imu.zacc);
	_gyroscope_info_widget->handleIMUUpdate(raw_imu.xgyro, raw_imu.ygyro,
																					raw_imu.zgyro);
}

void AutopilotSettingsPage::handlePowerStatusUpdate(
		mavlink_power_status_t power_status) {
	_mcu_info_widget->handlePowerStatusUpdate(power_status.Vcc);
}

void AutopilotSettingsPage::handleMcuStatusUpdate(
		mavlink_mcu_status_t mcu_status) {
	_mcu_info_widget->handleMcuStatusUpdate(mcu_status);
}

void AutopilotSettingsPage::handleMagCalProgressUpdate(
		mavlink_mag_cal_progress_t mag_cal_progress) {
	emit magCalProgressUpdated(mag_cal_progress);
}

void AutopilotSettingsPage::handleMagCalReportUpdate(
		mavlink_mag_cal_report_t mag_cal_report) {
	emit magCalReportUpdated(mag_cal_report);
}

void AutopilotSettingsPage::_handleStartCalibration() {
	emit startCalibration();
}

void AutopilotSettingsPage::_handleStartMagCalibration() {
	emit startMagCalibration();
}

void AutopilotSettingsPage::_handleCancelMagCalibration() {
	emit cancelMagCalibration();
}
