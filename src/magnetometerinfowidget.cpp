#include "magnetometerinfowidget.h"

MagnetometerInfoWidget::MagnetometerInfoWidget(QWidget *parent)
		: QWidget{parent}, _layout(new QVBoxLayout(this)),
			_title_label(new QLabel()), _status_label(new QLabel()),
			_x_label(new QLabel("x: 0")), _y_label(new QLabel("y: 0")),
			_z_label(new QLabel("z: 0")),
			_start_calibration_button(new QPushButton()),
			_cancel_calibration_button(new QPushButton()),
			_cal_progress_container(new QWidget()), _cal_attempt_label(new QLabel()),
			_cal_step_label(new QLabel()), _mag_cal_progress_bar(new QProgressBar()),
			_cal_result_label(new QLabel()) {
	auto title_layout = new QHBoxLayout();
	auto values_layout = new QHBoxLayout();
	_layout->addLayout(title_layout);
	_layout->addLayout(values_layout);
	_layout->addWidget(_cal_progress_container);
	_layout->addWidget(_cal_result_label);

	// Title section
	title_layout->addWidget(_title_label);
	title_layout->addWidget(_status_label);

	_title_label->setText(tr("Magnetometer"));
	_status_label->setText(tr("Stats: not found"));

	// Values section
	values_layout->addWidget(_x_label);
	values_layout->addWidget(_y_label);
	values_layout->addWidget(_z_label);
	values_layout->addWidget(_start_calibration_button);
	values_layout->addWidget(_cancel_calibration_button);

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
}

void MagnetometerInfoWidget::handleIMUUpdate(uint16_t x, uint16_t y,
																						 uint16_t z) {
	_x_label->setText(QString("x: %1").arg(x));
	_y_label->setText(QString("y: %1").arg(y));
	_z_label->setText(QString("z: %1").arg(z));
}

void MagnetometerInfoWidget::handleMagStatusUpdate(SensorStatus status) {
	switch (status) {
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
