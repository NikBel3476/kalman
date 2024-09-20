#include "gyroscopeinfowidget.h"

GyroscopeInfoWidget::GyroscopeInfoWidget(QWidget *parent)
		: QWidget{parent}, _layout(new QVBoxLayout(this)),
			_title_label(new QLabel()), _status_label(new QLabel()),
			_x_label(new QLabel("x: 0")), _y_label(new QLabel("y: 0")),
			_z_label(new QLabel("z: 0")), _cal_start_btn(new QPushButton()),
			_cal_result_label(new QLabel()) {
	const auto title_layout = new QHBoxLayout();
	auto values_layout = new QHBoxLayout();
	_layout->addLayout(title_layout);
	_layout->addLayout(values_layout);
	_layout->addWidget(_cal_result_label);

	title_layout->addWidget(_title_label);
	title_layout->addWidget(_status_label);

	_title_label->setText(tr("Gyroscope"));
	_status_label->setText(tr("Status: not found"));

	values_layout->addWidget(_x_label);
	values_layout->addWidget(_y_label);
	values_layout->addWidget(_z_label);
	values_layout->addWidget(_cal_start_btn);

	_cal_start_btn->setText(tr("Calibration"));

	connect(_cal_start_btn, &QPushButton::pressed, this,
					&GyroscopeInfoWidget::_handleCalStartBtnPress);
}

void GyroscopeInfoWidget::handleIMUUpdate(uint16_t x, uint16_t y, uint16_t z) {
	_x_label->setText(QString("x: %1").arg(x));
	_y_label->setText(QString("y: %1").arg(y));
	_z_label->setText(QString("z: %1").arg(z));
}

void GyroscopeInfoWidget::handleGyroStatusUpdate(SensorStatus status) {
	switch (status) {
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

void GyroscopeInfoWidget::handleGyroCalibrationComplete() {
	_cal_result_label->setText(tr("Success"));
	_cal_result_label->setVisible(true);
	_cal_start_btn->setEnabled(true);
}

void GyroscopeInfoWidget::_handleCalStartBtnPress() {
	_cal_start_btn->setEnabled(false);
	_cal_result_label->setVisible(false);
	emit startCalibration();
}
