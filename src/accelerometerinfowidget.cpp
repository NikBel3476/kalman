#include "accelerometerinfowidget.h"

static const int MIN_LABEL_WIDTH = 50;
static const int MAX_LABEL_WIDTH = 100;

AccelerometerInfoWidget::AccelerometerInfoWidget(QWidget *parent)
		: QWidget{parent}, _layout(new QVBoxLayout(this)),
			_x_label(new QLabel("x: 0")), _y_label(new QLabel("y: 0")),
			_z_label(new QLabel("z: 0")), _accel_cal_btn(new QPushButton()),
			_lvl_cal_btn(new QPushButton()), _cal_result_label(new QLabel()) {
	auto values_layout = new QHBoxLayout();
	_layout->addWidget(new QLabel(tr("Accelerometer")));
	_layout->addLayout(values_layout);
	_layout->addWidget(_cal_result_label);

	values_layout->addWidget(_x_label);
	values_layout->addWidget(_y_label);
	values_layout->addWidget(_z_label);
	values_layout->addWidget(_accel_cal_btn);
	values_layout->addWidget(_lvl_cal_btn);

	_cal_result_label->setVisible(false);

	_accel_cal_btn->setText(tr("Calibration"));
	_lvl_cal_btn->setText(tr("Level calibration"));

	connect(_accel_cal_btn, &QPushButton::pressed, this,
					&AccelerometerInfoWidget::_handleAccelCalBtnPress);
	connect(_lvl_cal_btn, &QPushButton::pressed, this,
					&AccelerometerInfoWidget::_handleLvlCalBtnPress);
}

void AccelerometerInfoWidget::handleIMUUpdate(uint16_t x, uint16_t y,
																							uint16_t z) {
	_x_label->setText(QString("x: %1").arg(x));
	_y_label->setText(QString("y: %1").arg(y));
	_z_label->setText(QString("z: %1").arg(z));
}

void AccelerometerInfoWidget::handleAccelCalComplete() {
	_accel_cal_btn->setEnabled(true);
	_lvl_cal_btn->setEnabled(false);
	_cal_result_label->setText(tr("Success"));
	_cal_result_label->setVisible(true);
}

void AccelerometerInfoWidget::handleLvlCalComplete() {
	_accel_cal_btn->setEnabled(true);
	_lvl_cal_btn->setEnabled(true);
	_cal_result_label->setText(tr("Success"));
	_cal_result_label->setVisible(true);
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
