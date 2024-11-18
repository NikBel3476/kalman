#include "accelerometerinfowidget.h"

AccelerometerInfoWidget::AccelerometerInfoWidget(
		QWidget *parent, MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_title_label(new QLabel()),
			_status_label(new QLabel()),
			_x_label(new QLabel("x: 0")),
			_y_label(new QLabel("y: 0")),
			_z_label(new QLabel("z: 0")),
			_accel_cal_btn(new QPushButton()),
			_lvl_cal_btn(new QPushButton()),
			_cal_result_label(new QLabel()),
			_mavlink_manager{mavlink_manager} {
	const auto values_layout = new QHBoxLayout();
	const auto title_layout = new QHBoxLayout();
	const auto buttons_layout = new QHBoxLayout();
	_layout->addLayout(title_layout);
	_layout->addLayout(values_layout);
	_layout->addLayout(buttons_layout);
	_layout->addWidget(_cal_result_label);

	// Title section
	title_layout->addWidget(_title_label);
	title_layout->addWidget(_status_label);

	_title_label->setText(tr("Accelerometer"));
	_status_label->setText(tr("Status: not found"));

	// Values section
	values_layout->addWidget(_x_label);
	values_layout->addWidget(_y_label);
	values_layout->addWidget(_z_label);

	// Buttons section
	buttons_layout->addWidget(_accel_cal_btn);
	buttons_layout->addWidget(_lvl_cal_btn);
	buttons_layout->addStretch();

	_cal_result_label->setVisible(false);

	_accel_cal_btn->setText(tr("Calibration"));
	_lvl_cal_btn->setText(tr("Level calibration"));

	connect(_accel_cal_btn, &QPushButton::pressed, this,
					&AccelerometerInfoWidget::_handleAccelCalBtnPress);
	connect(_lvl_cal_btn, &QPushButton::pressed, this,
					&AccelerometerInfoWidget::_handleLvlCalBtnPress);
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&AccelerometerInfoWidget::_handleMavlinkMessageReceive);
}

void AccelerometerInfoWidget::handleAccelCalComplete(CalibrationResult result) {
	_accel_cal_btn->setEnabled(true);
	_lvl_cal_btn->setEnabled(true);
	switch (result) {
	case CalibrationResult::Success: {
		_cal_result_label->setText(tr("Success"));
	} break;
	case CalibrationResult::Failed: {
		_cal_result_label->setText(tr("Failed"));
	}
	}
	_cal_result_label->setVisible(true);
}

void AccelerometerInfoWidget::handleLvlCalComplete() {
	_accel_cal_btn->setEnabled(true);
	_lvl_cal_btn->setEnabled(true);
	_cal_result_label->setText(tr("Success"));
	_cal_result_label->setVisible(true);
}

void AccelerometerInfoWidget::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_SCALED_IMU2: {
		mavlink_scaled_imu2_t scaled_imu;
		mavlink_msg_scaled_imu2_decode(&mavlink_message, &scaled_imu);
		_handleIMU2Update(scaled_imu);
	} break;
	case MAVLINK_MSG_ID_SYS_STATUS: {
		mavlink_sys_status_t sys_status;
		mavlink_msg_sys_status_decode(&mavlink_message, &sys_status);
		_handleSysStatusUpdate(sys_status);
	} break;
	}
}

void AccelerometerInfoWidget::_handleAccelCalBtnPress() {
	_accel_cal_btn->setEnabled(false);
	_lvl_cal_btn->setEnabled(false);
	_cal_result_label->setVisible(false);
	emit startAccelCal();
}

void AccelerometerInfoWidget::_handleLvlCalBtnPress() {
	_accel_cal_btn->setEnabled(false);
	_lvl_cal_btn->setEnabled(false);
	_cal_result_label->setVisible(false);
	emit startLevelCal();
}

void AccelerometerInfoWidget::_handleIMU2Update(
		const mavlink_scaled_imu2_t &scaled_imu) {
	_x_label->setText(QString("x: %1").arg(scaled_imu.xacc));
	_y_label->setText(QString("y: %1").arg(scaled_imu.yacc));
	_z_label->setText(QString("z: %1").arg(scaled_imu.zacc));
}

void AccelerometerInfoWidget::_handleSysStatusUpdate(
		const mavlink_sys_status_t &sys_status) {
	auto current_accel_status = SensorStatus::NotFound;
	if (sys_status.onboard_control_sensors_present &
			MAV_SYS_STATUS_SENSOR_3D_ACCEL) {
		if (sys_status.onboard_control_sensors_enabled &
				MAV_SYS_STATUS_SENSOR_3D_ACCEL) {
			if (sys_status.onboard_control_sensors_health &
					MAV_SYS_STATUS_SENSOR_3D_ACCEL) {
				current_accel_status = SensorStatus::Enabled;
			} else {
				current_accel_status = SensorStatus::Error;
			}
		} else {
			current_accel_status = SensorStatus::Disabled;
		}
	}
	if (current_accel_status != _acc_status) {
		_acc_status = current_accel_status;
		switch (_acc_status) {
		case SensorStatus::NotFound: {
			_status_label->setText(tr("Status: not found"));
		} break;
		case SensorStatus::Disabled: {
			_status_label->setText(tr("Status: disabled"));
		} break;
		case SensorStatus::Enabled: {
			_status_label->setText(tr("Status: enabled"));
		} break;
		case SensorStatus::Error: {
			_status_label->setText(tr("Status: error"));
		} break;
		}
	}
}
