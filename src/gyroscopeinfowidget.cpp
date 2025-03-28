#include "gyroscopeinfowidget.hpp"

GyroscopeInfoWidget::GyroscopeInfoWidget(QWidget *parent,
																				 MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_title_label(new QLabel()),
			_status_label(new QLabel()),
			_x_imu_label(new QLabel("imu_x: 0")),
			_y_imu_label(new QLabel("imu_y: 0")),
			_z_imu_label(new QLabel("imu_z: 0")),
			_x_imu2_label(new QLabel("imu2_x: 0")),
			_y_imu2_label(new QLabel("imu2_y: 0")),
			_z_imu2_label(new QLabel("imu2_z: 0")),
			_cal_start_btn(new QPushButton()),
			_cal_result_label(new QLabel()),
			_mavlink_manager{mavlink_manager} {
	const auto title_layout = new QHBoxLayout();
	const auto imu_values_layout = new QHBoxLayout();
	const auto imu2_values_layout = new QHBoxLayout();
	const auto buttons_layout = new QHBoxLayout();
	_layout->addLayout(title_layout);
	_layout->addLayout(imu_values_layout);
	_layout->addLayout(imu2_values_layout);
	_layout->addLayout(buttons_layout);
	_layout->addWidget(_cal_result_label);
	_layout->setSpacing(5);
	_layout->setContentsMargins(0, 0, 0, 0);

	// Title section
	title_layout->addWidget(_title_label);
	title_layout->addWidget(_status_label);

	_title_label->setText(tr("Gyroscope"));
	_status_label->setText(tr("Status: not found"));

	// Values section
	imu_values_layout->addWidget(_x_imu_label);
	imu_values_layout->addWidget(_y_imu_label);
	imu_values_layout->addWidget(_z_imu_label);

	imu2_values_layout->addWidget(_x_imu2_label);
	imu2_values_layout->addWidget(_y_imu2_label);
	imu2_values_layout->addWidget(_z_imu2_label);

	_x_imu_label->setMinimumWidth(80);
	_y_imu_label->setMinimumWidth(80);
	_z_imu_label->setMinimumWidth(80);
	_x_imu2_label->setMinimumWidth(80);
	_y_imu2_label->setMinimumWidth(80);
	_z_imu2_label->setMinimumWidth(80);

	// Buttons section
	buttons_layout->addWidget(_cal_start_btn);
	buttons_layout->addStretch();

	_cal_start_btn->setText(tr("Calibration"));

	_cal_result_label->setVisible(false);

	connect(_cal_start_btn, &QPushButton::pressed, this,
					&GyroscopeInfoWidget::_handleCalStartBtnPress);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&GyroscopeInfoWidget::_handleMavlinkMessageReceive);
}

void GyroscopeInfoWidget::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_RAW_IMU: {
		mavlink_raw_imu_t raw_imu;
		mavlink_msg_raw_imu_decode(&mavlink_message, &raw_imu);
		_handleIMUUpdate(raw_imu);
	} break;
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
	case MAVLINK_MSG_ID_COMMAND_ACK: {
		mavlink_command_ack_t cmd_ack;
		mavlink_msg_command_ack_decode(&mavlink_message, &cmd_ack);
		_handleCommandAck(cmd_ack);
	} break;
	}
}

void GyroscopeInfoWidget::_handleIMUUpdate(const mavlink_raw_imu_t &raw_imu) {
	_x_imu_label->setText(QString("imu_x: %1").arg(raw_imu.xgyro));
	_y_imu_label->setText(QString("imu_y: %1").arg(raw_imu.ygyro));
	_z_imu_label->setText(QString("imu_z: %1").arg(raw_imu.zgyro));
}

void GyroscopeInfoWidget::_handleIMU2Update(
		const mavlink_scaled_imu2_t &scaled_imu) {
	_x_imu2_label->setText(QString("imu2_x: %1").arg(scaled_imu.xgyro));
	_y_imu2_label->setText(QString("imu2_y: %1").arg(scaled_imu.ygyro));
	_z_imu2_label->setText(QString("imu2_z: %1").arg(scaled_imu.zgyro));
}

void GyroscopeInfoWidget::_handleCalStartBtnPress() {
	_cal_start_btn->setEnabled(false);
	_cal_result_label->setVisible(false);

	const auto command = MAV_CMD_PREFLIGHT_CALIBRATION;
	const uint8_t confirmation = 0;
	const float param1 = 1; // gyroscope calibration
	_mavlink_manager->sendCmdLong(command, confirmation, param1);
	_cal_gyro_state = CalibrationState::InProgress;
	qDebug("Gyro calibration started\n");
}

void GyroscopeInfoWidget::_handleSysStatusUpdate(
		const mavlink_sys_status_t &sys_status) {
	auto current_gyro_status = SensorStatus::NotFound;
	if (sys_status.onboard_control_sensors_present &
			MAV_SYS_STATUS_SENSOR_3D_GYRO) {
		if (sys_status.onboard_control_sensors_enabled &
				MAV_SYS_STATUS_SENSOR_3D_GYRO) {
			if (sys_status.onboard_control_sensors_health &
					MAV_SYS_STATUS_SENSOR_3D_GYRO) {
				current_gyro_status = SensorStatus::Enabled;
			} else {
				current_gyro_status = SensorStatus::Error;
			}
		} else {
			current_gyro_status = SensorStatus::Disabled;
		}
	}
	if (current_gyro_status != _gyro_status) {
		_gyro_status = current_gyro_status;
		switch (_gyro_status) {
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

void GyroscopeInfoWidget::_handleCommandAck(const mavlink_command_ack_t &cmd) {
	switch (cmd.command) {
	case MAV_CMD_PREFLIGHT_CALIBRATION: {
		switch (cmd.result) {
		case MAV_RESULT_ACCEPTED: {
			if (_cal_gyro_state == CalibrationState::InProgress) {
				_cal_gyro_state = CalibrationState::None;
				emit _handleGyroCalibrationComplete();
			}
		} break;
		default:
			break;
		}
	} break;
	}
}

void GyroscopeInfoWidget::_handleGyroCalibrationComplete() {
	_cal_result_label->setText(tr("Success"));
	_cal_result_label->setVisible(true);
	_cal_start_btn->setEnabled(true);
}
