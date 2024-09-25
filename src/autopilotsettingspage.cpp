#include "autopilotsettingspage.h"

static const int MIN_WIDGET_WIDTH = 150;
static const int MAX_WIDGET_WIDTH = 500;

AutopilotSettingsPage::AutopilotSettingsPage(QWidget *parent)
		: QWidget{parent}, _layout(new QVBoxLayout(this)),
			_magnetometer_info_widget(new MagnetometerInfoWidget()),
			_accelerometer_info_widget(new AccelerometerInfoWidget()),
			_gyroscope_info_widget(new GyroscopeInfoWidget()),
			_mcu_info_widget(new McuInfoWidget()),
			_avionics_widget(new AvionicsWidget()) {
	_layout->setAlignment(Qt::AlignTop);
	_layout->addWidget(_magnetometer_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_accelerometer_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_gyroscope_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_mcu_info_widget, 0, Qt::AlignLeft);
	_layout->addWidget(_avionics_widget, 0, Qt::AlignCenter);
	_layout->addStretch();

	connect(this, &AutopilotSettingsPage::accelStatusUpdated,
					_accelerometer_info_widget,
					&AccelerometerInfoWidget::handleAccelStatusUpdate);
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

	connect(this, &AutopilotSettingsPage::magStatusUpdated,
					_magnetometer_info_widget,
					&MagnetometerInfoWidget::handleMagStatusUpdate);
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

	connect(this, &AutopilotSettingsPage::gyroStatusUpdated,
					_gyroscope_info_widget, &GyroscopeInfoWidget::handleGyroStatusUpdate);
	connect(_gyroscope_info_widget, &GyroscopeInfoWidget::startCalibration, this,
					&AutopilotSettingsPage::_handleStartGyroCalibration);
	connect(this, &AutopilotSettingsPage::gyroCalibrationCompleted,
					_gyroscope_info_widget,
					&GyroscopeInfoWidget::handleGyroCalibrationComplete);

	connect(this, &AutopilotSettingsPage::imu2Updated, _avionics_widget,
					&AvionicsWidget::handleImu2Update);
	connect(this, &AutopilotSettingsPage::attitudeUpdated, _avionics_widget,
					&AvionicsWidget::handleAttitudeUpdate);
	connect(this, &AutopilotSettingsPage::globalPositionIntUpdated,
					_avionics_widget, &AvionicsWidget::handleGlobalPositionIntUpdate);
	connect(this, &AutopilotSettingsPage::vfrHudUpdated, _avionics_widget,
					&AvionicsWidget::handleVfrHudUpdate);
}

void AutopilotSettingsPage::handleIMUUpdate(mavlink_raw_imu_t raw_imu) {
	_magnetometer_info_widget->handleIMUUpdate(raw_imu.xmag, raw_imu.ymag,
																						 raw_imu.zmag);
	_accelerometer_info_widget->handleIMUUpdate(raw_imu.xacc, raw_imu.yacc,
																							raw_imu.zacc);
	_gyroscope_info_widget->handleIMUUpdate(raw_imu.xgyro, raw_imu.ygyro,
																					raw_imu.zgyro);
}

void AutopilotSettingsPage::handleImu2Update(mavlink_scaled_imu2_t imu) {
	emit imu2Updated(imu);
}

void AutopilotSettingsPage::handlePowerStatusUpdate(
		mavlink_power_status_t power_status) {
	_mcu_info_widget->handlePowerStatusUpdate(power_status.Vcc);
}

void AutopilotSettingsPage::handleMcuStatusUpdate(
		mavlink_mcu_status_t mcu_status) {
	_mcu_info_widget->handleMcuStatusUpdate(mcu_status);
}

void AutopilotSettingsPage::handleAttitudeUpdate(mavlink_attitude_t attitude) {
	emit attitudeUpdated(attitude);
}

void AutopilotSettingsPage::handleGlobalPositionIntUpdate(
		mavlink_global_position_int_t global_position) {
	emit globalPositionIntUpdated(global_position);
}

void AutopilotSettingsPage::handleVfrHudUpdate(mavlink_vfr_hud_t vfr_hud) {
	emit vfrHudUpdated(vfr_hud);
}

void AutopilotSettingsPage::handleGyroStatusUpdate(SensorStatus status) {
	emit gyroStatusUpdated(status);
}

void AutopilotSettingsPage::handleAccelStatusUpdate(SensorStatus status) {
	emit accelStatusUpdated(status);
}

void AutopilotSettingsPage::handleMagStatusUpdate(SensorStatus status) {
	emit magStatusUpdated(status);
}

void AutopilotSettingsPage::handleCompleteAccelerometerCalibration() {
	emit accelerometerCalibrationCompleted();
}

void AutopilotSettingsPage::handleMagCalProgressUpdate(
		mavlink_mag_cal_progress_t mag_cal_progress) {
	emit magCalProgressUpdated(mag_cal_progress);
}

void AutopilotSettingsPage::handleMagCalReportUpdate(
		mavlink_mag_cal_report_t mag_cal_report) {
	emit magCalReportUpdated(mag_cal_report);
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

void AutopilotSettingsPage::_handleStartMagCalibration() {
	emit startMagCalibration();
}

void AutopilotSettingsPage::_handleCancelMagCalibration() {
	emit cancelMagCalibration();
}

void AutopilotSettingsPage::_handleStartGyroCalibration() {
	emit startGyroCalibration();
}
