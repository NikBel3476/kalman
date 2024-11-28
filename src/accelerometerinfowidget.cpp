#include "accelerometerinfowidget.hpp"

AccelerometerInfoWidget::AccelerometerInfoWidget(
		QWidget *parent, MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_title_label(new QLabel(tr("Accelerometer"))),
			_status_label(new QLabel(tr("Status: not found"))),
			_accel_strength_label(new QLabel(tr("Strength: %1").arg(0))),
			_x_imu_label(new QLabel("imu_x: 0")),
			_y_imu_label(new QLabel("imu_y: 0")),
			_z_imu_label(new QLabel("imu_z: 0")),
			_x_imu2_label(new QLabel("imu2_x: 0")),
			_y_imu2_label(new QLabel("imu2_y: 0")),
			_z_imu2_label(new QLabel("imu2_z: 0")),
			_accel_cal_btn(new QPushButton()),
			_lvl_cal_btn(new QPushButton()),
			_cal_result_label(new QLabel()),
			_msg_cal_box(new QMessageBox(this)),
			_msg_cal_box_button(_msg_cal_box->addButton(QMessageBox::Ok)),
			_mavlink_manager{mavlink_manager} {
	const auto title_layout = new QHBoxLayout();
	const auto imu_values_layout = new QHBoxLayout();
	const auto imu2_values_layout = new QHBoxLayout();
	const auto buttons_layout = new QHBoxLayout();
	_layout->addLayout(title_layout);
	_layout->addWidget(_accel_strength_label);
	_layout->addLayout(imu_values_layout);
	_layout->addLayout(imu2_values_layout);
	_layout->addLayout(buttons_layout);
	_layout->addWidget(_cal_result_label);
	_layout->setSpacing(5);
	_layout->setContentsMargins(0, 0, 0, 0);

	// Title section
	title_layout->addWidget(_title_label);
	title_layout->addWidget(_status_label);

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
	buttons_layout->addWidget(_accel_cal_btn);
	buttons_layout->addWidget(_lvl_cal_btn);
	buttons_layout->addStretch();

	_cal_result_label->setVisible(false);

	_accel_cal_btn->setText(tr("Calibration"));
	_lvl_cal_btn->setText(tr("Level calibration"));

	_msg_cal_box->setWindowTitle(tr("Calibration"));

	// connections
	connect(_accel_cal_btn, &QPushButton::pressed, this,
					&AccelerometerInfoWidget::_handleAccelCalBtnPress);
	connect(_lvl_cal_btn, &QPushButton::pressed, this,
					&AccelerometerInfoWidget::_handleLvlCalBtnPress);
	connect(_msg_cal_box_button, &QPushButton::clicked, this,
					&AccelerometerInfoWidget::_handleCalibrationDialogButton);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&AccelerometerInfoWidget::_handleMavlinkMessageReceive);
}

void AccelerometerInfoWidget::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_SCALED_IMU: {
		mavlink_scaled_imu_t scaled_imu;
		mavlink_msg_scaled_imu_decode(&mavlink_message, &scaled_imu);
		_handleIMUUpdate(scaled_imu);
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
	case MAVLINK_MSG_ID_COMMAND_LONG: {
		mavlink_command_long_t cmd;
		mavlink_msg_command_long_decode(&mavlink_message, &cmd);
		_parseCommand(cmd);
	} break;
	case MAVLINK_MSG_ID_COMMAND_ACK: {
		mavlink_command_ack_t cmd_ack;
		mavlink_msg_command_ack_decode(&mavlink_message, &cmd_ack);
		_handleCommandAck(cmd_ack);
	} break;
	}
}

void AccelerometerInfoWidget::_handleAccelCalBtnPress() {
	_accel_cal_btn->setEnabled(false);
	_lvl_cal_btn->setEnabled(false);
	_cal_result_label->setVisible(false);

	const auto command = MAV_CMD_PREFLIGHT_CALIBRATION;
	const uint8_t confirmation = 0;
	const float param1 = 0;
	const float param2 = 0;
	const float param3 = 0;
	const float param4 = 0;
	const float param5 = 1; // accelerometer calibration
	_mavlink_manager->sendCmdLong(command, confirmation, param1, param2, param3,
																param4, param5);
	_cal_state = CalibrationState::InProgress;
	qDebug("Accel calibration started\n");
}

void AccelerometerInfoWidget::_handleLvlCalBtnPress() {
	_accel_cal_btn->setEnabled(false);
	_lvl_cal_btn->setEnabled(false);
	_cal_result_label->setVisible(false);

	const auto command = MAV_CMD_PREFLIGHT_CALIBRATION;
	const uint8_t confirmation = 0;
	const float param1 = 0;
	const float param2 = 0;
	const float param3 = 0;
	const float param4 = 0;
	const float param5 = 2; // level calibration
	_mavlink_manager->sendCmdLong(command, confirmation, param1, param2, param3,
																param4, param5);
	_cal_lvl_state = CalibrationState::InProgress;
}

void AccelerometerInfoWidget::_handleIMUUpdate(
		const mavlink_scaled_imu_t &scaled_imu) {
	_x_imu_label->setText(QString("imu_x: %1").arg(scaled_imu.xacc));
	_y_imu_label->setText(QString("imu_y: %1").arg(scaled_imu.yacc));
	_z_imu_label->setText(QString("imu_z: %1").arg(scaled_imu.zacc));
}

void AccelerometerInfoWidget::_handleIMU2Update(
		const mavlink_scaled_imu2_t &scaled_imu) {
	_x_imu2_label->setText(QString("imu2_x: %1").arg(scaled_imu.xacc));
	_y_imu2_label->setText(QString("imu2_y: %1").arg(scaled_imu.yacc));
	_z_imu2_label->setText(QString("imu2_z: %1").arg(scaled_imu.zacc));

	const auto accel_strength =
			std::sqrt(std::pow(scaled_imu.xacc, 2) + std::pow(scaled_imu.yacc, 2) +
								std::pow(scaled_imu.zacc, 2)) /
			1000.0;
	_accel_strength_label->setText(tr("Strength: %1").arg(accel_strength));
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

void AccelerometerInfoWidget::_parseCommand(const mavlink_command_long_t &cmd) {
	switch (cmd.command) {
	case MAV_CMD_ACCELCAL_VEHICLE_POS: {
		if (_cal_state == CalibrationState::InProgress) {
			switch (static_cast<ACCELCAL_VEHICLE_POS>(static_cast<int>(cmd.param1))) {
			case ACCELCAL_VEHICLE_POS_LEVEL: {
				_cal_acc_state = CalibrationAccelState::Level;
				_msg_cal_box->setText(
						tr("Place vehicle in level position and then press OK"));
			} break;
			case ACCELCAL_VEHICLE_POS_LEFT: {
				_cal_acc_state = CalibrationAccelState::LeftSide;
				_msg_cal_box->setText(
						tr("Place vehicle on the left side and then press OK"));
			} break;
			case ACCELCAL_VEHICLE_POS_RIGHT: {
				_cal_acc_state = CalibrationAccelState::RightSide;
				_msg_cal_box->setText(
						tr("Place vehicle on the right side and then press OK"));
			} break;
			case ACCELCAL_VEHICLE_POS_NOSEUP: {
				_cal_acc_state = CalibrationAccelState::NoseUp;
				_msg_cal_box->setText(
						tr("Place vehicle in noseup position and then press OK"));
			} break;
			case ACCELCAL_VEHICLE_POS_NOSEDOWN: {
				_cal_acc_state = CalibrationAccelState::NoseDown;
				_msg_cal_box->setText(
						tr("Place vehicle in nosedown position and then press OK"));
			} break;
			case ACCELCAL_VEHICLE_POS_BACK: {
				_cal_acc_state = CalibrationAccelState::Back;
				_msg_cal_box->setText(
						tr("Place vehicle on the back side and then press OK"));
			} break;
			case ACCELCAL_VEHICLE_POS_SUCCESS: {
				_cal_state = CalibrationState::None;
				_cal_acc_state = CalibrationAccelState::None;
				_msg_cal_box->setText(tr("Calibration completed"));
				_handleAccelCalComplete(CalibrationResult::Success);
			} break;
			case ACCELCAL_VEHICLE_POS_FAILED: {
				_cal_state = CalibrationState::None;
				_cal_acc_state = CalibrationAccelState::None;
				_msg_cal_box->setText(tr("Calibration failed"));
				_handleAccelCalComplete(CalibrationResult::Failed);
			} break;
			case ACCELCAL_VEHICLE_POS_ENUM_END:
				break;
			}
			_msg_cal_box->show();
		}
	} break;
	default:
		break;
	}
}

void AccelerometerInfoWidget::_handleAccelCalComplete(
		CalibrationResult result) {
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

void AccelerometerInfoWidget::_handleCalibrationDialogButton() {
	if (_cal_state == CalibrationState::None) {
		return;
	}
	float vehicle_position_param;
	switch (_cal_acc_state) {
	case CalibrationAccelState::Level: {
		_cal_acc_state = CalibrationAccelState::LeftSide;
		vehicle_position_param = ACCELCAL_VEHICLE_POS_LEVEL;
	} break;
	case CalibrationAccelState::LeftSide: {
		vehicle_position_param = ACCELCAL_VEHICLE_POS_LEFT;
	} break;
	case CalibrationAccelState::RightSide: {
		vehicle_position_param = ACCELCAL_VEHICLE_POS_RIGHT;
	} break;
	case CalibrationAccelState::NoseDown: {
		vehicle_position_param = ACCELCAL_VEHICLE_POS_NOSEDOWN;
	} break;
	case CalibrationAccelState::NoseUp: {
		vehicle_position_param = ACCELCAL_VEHICLE_POS_NOSEUP;
	} break;
	case CalibrationAccelState::Back: {
		vehicle_position_param = ACCELCAL_VEHICLE_POS_BACK;
	} break;
	case CalibrationAccelState::None:
	default:
		_msg_cal_box->close();
		return;
	}

	const auto command = MAV_CMD_ACCELCAL_VEHICLE_POS;
	const auto confirmation = 0;
	_mavlink_manager->sendCmdLong(command, confirmation, vehicle_position_param);
}

void AccelerometerInfoWidget::_handleCommandAck(
		const mavlink_command_ack_t &cmd) {
	switch (cmd.command) {
	case MAV_CMD_PREFLIGHT_CALIBRATION: {
		switch (cmd.result) {
		case MAV_RESULT_ACCEPTED: {
			if (_cal_lvl_state == CalibrationState::InProgress) {
				_cal_lvl_state = CalibrationState::None;
				_handleLvlCalComplete(CalibrationResult::Success);
			}
		} break;
		case MAV_RESULT_FAILED: {
			if (_cal_lvl_state == CalibrationState::InProgress) {
				_cal_lvl_state = CalibrationState::None;
				_handleLvlCalComplete(CalibrationResult::Failed);
			}
		}
		default:
			break;
		}
	} break;
	default:
		break;
	}
}

void AccelerometerInfoWidget::_handleLvlCalComplete(
		const CalibrationResult &cal_result) {
	_accel_cal_btn->setEnabled(true);
	_lvl_cal_btn->setEnabled(true);
	switch (cal_result) {
	case CalibrationResult::Success: {
		_cal_result_label->setText(tr("Success"));
	} break;
	case CalibrationResult::Failed: {
		_cal_result_label->setText(tr("Success"));
	} break;
	}
	_cal_result_label->setVisible(true);
}
