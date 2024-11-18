#include "magnetometerinfowidget.h"

MagnetometerInfoWidget::MagnetometerInfoWidget(QWidget *parent,
																							 MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_title_label(new QLabel()),
			_status_label(new QLabel()),
			_x_label(new QLabel("x: 0")),
			_y_label(new QLabel("y: 0")),
			_z_label(new QLabel("z: 0")),
			_start_calibration_button(new QPushButton()),
			_cancel_calibration_button(new QPushButton()),
			_cal_progress_container(new QWidget()),
			_cal_attempt_label(new QLabel()),
			_cal_step_label(new QLabel()),
			_mag_cal_progress_bar(new QProgressBar()),
			_cal_result_label(new QLabel()),
			_mavlink_manager{mavlink_manager} {
	const auto title_layout = new QHBoxLayout();
	const auto values_layout = new QHBoxLayout();
	const auto buttons_layout = new QHBoxLayout();
	_layout->addLayout(title_layout);
	_layout->addLayout(values_layout);
	_layout->addLayout(buttons_layout);
	_layout->addWidget(_cal_progress_container);
	_layout->addWidget(_cal_result_label);

	// Title section
	title_layout->addWidget(_title_label);
	title_layout->addWidget(_status_label);
	title_layout->addStretch();

	_title_label->setText(tr("Magnetometer"));
	_status_label->setText(tr("Stats: not found"));

	// Values section
	values_layout->addWidget(_x_label);
	values_layout->addWidget(_y_label);
	values_layout->addWidget(_z_label);

	// Buttons section
	buttons_layout->addWidget(_start_calibration_button);
	buttons_layout->addWidget(_cancel_calibration_button);
	buttons_layout->addStretch();

	_start_calibration_button->setText(tr("Calibration"));
	_cancel_calibration_button->setText(tr("Cancel"));
	_cancel_calibration_button->setEnabled(false);

	// Progress bar section
	const auto cal_progress_container_layout = new QVBoxLayout();
	_cal_progress_container->setLayout(cal_progress_container_layout);
	_cal_progress_container->setVisible(false);

	const auto progress_info_layout = new QHBoxLayout();
	cal_progress_container_layout->addLayout(progress_info_layout);
	cal_progress_container_layout->addWidget(_mag_cal_progress_bar);

	progress_info_layout->addWidget(_cal_attempt_label);
	progress_info_layout->addWidget(_cal_step_label);

	_cal_attempt_label->setText(tr("Attempt: %1").arg(_cal_attempt));
	_cal_step_label->setText(tr("Step: %1").arg(_cal_step));

	_mag_cal_progress_bar->setMinimum(0);
	_mag_cal_progress_bar->setMaximum(100);

	_cal_result_label->setVisible(false);

	connect(_start_calibration_button, &QPushButton::pressed, this,
					&MagnetometerInfoWidget::_handleCalStartButtonPress);
	connect(_cancel_calibration_button, &QPushButton::pressed, this,
					&MagnetometerInfoWidget::_handleCalCancelButtonPress);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&MagnetometerInfoWidget::_handleMavlinkMessageReceive);
}

void MagnetometerInfoWidget::handleMagCalProgressUpdate(
		mavlink_mag_cal_progress_t mag_cal_progress) {
	_mag_cal_progress_bar->setValue(mag_cal_progress.completion_pct);
	_cal_attempt_label->setText(tr("Attempt: %1").arg(mag_cal_progress.attempt));
	switch (mag_cal_progress.cal_status) {
	case MAG_CAL_RUNNING_STEP_ONE: {
		_cal_step_label->setText(tr("Step: %1").arg(1));
	} break;
	case MAG_CAL_RUNNING_STEP_TWO: {
		_cal_step_label->setText(tr("Step: %1").arg(2));
	} break;
	default:
		break;
	}
}

void MagnetometerInfoWidget::handleMagCalReportUpdate(
		mavlink_mag_cal_report_t mag_cal_report) {
	_cal_progress_container->setVisible(false);
	_cal_result_label->setText(mag_cal_report.cal_status == MAG_CAL_SUCCESS
																 ? tr("Success")
																 : tr("Failed"));
	_cal_result_label->setVisible(true);
	_start_calibration_button->setEnabled(true);
	_cancel_calibration_button->setEnabled(false);
}

void MagnetometerInfoWidget::_handleMavlinkMessageReceive(
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

void MagnetometerInfoWidget::_handleCalStartButtonPress() {
	emit startCalibration();
	_cal_progress_container->setVisible(true);
	_start_calibration_button->setEnabled(false);
	_cancel_calibration_button->setEnabled(true);
	_cal_result_label->setVisible(false);
}

void MagnetometerInfoWidget::_handleCalCancelButtonPress() {
	emit cancelCalibration();
	_cal_progress_container->setVisible(false);
	_start_calibration_button->setEnabled(true);
	_cancel_calibration_button->setEnabled(false);
	_cal_result_label->setVisible(false);
}

void MagnetometerInfoWidget::_handleIMU2Update(
		const mavlink_scaled_imu2_t &scaled_imu) {
	_x_label->setText(QString("x: %1").arg(scaled_imu.xmag));
	_y_label->setText(QString("y: %1").arg(scaled_imu.ymag));
	_z_label->setText(QString("z: %1").arg(scaled_imu.zmag));
}

void MagnetometerInfoWidget::_handleSysStatusUpdate(
		const mavlink_sys_status_t &sys_status) {
	auto current_mag_status = SensorStatus::NotFound;
	if (sys_status.onboard_control_sensors_present &
			MAV_SYS_STATUS_SENSOR_3D_MAG) {
		if (sys_status.onboard_control_sensors_enabled &
				MAV_SYS_STATUS_SENSOR_3D_MAG) {
			if (sys_status.onboard_control_sensors_health &
					MAV_SYS_STATUS_SENSOR_3D_MAG) {
				_mag_status = SensorStatus::Enabled;
			} else {
				current_mag_status = SensorStatus::Error;
			}
		} else {
			current_mag_status = SensorStatus::Disabled;
		}
	}
	if (current_mag_status != _mag_status) {
		_mag_status = current_mag_status;
		switch (_mag_status) {
		case SensorStatus::NotFound: {
			_status_label->setText(tr("Status: Not found"));
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
