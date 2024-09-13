#include "accelerometerinfowidget.h"

static const int MIN_LABEL_WIDTH = 50;
static const int MAX_LABEL_WIDTH = 100;

AccelerometerInfoWidget::AccelerometerInfoWidget(QWidget *parent)
    : QWidget{parent}, m_layout(new QVBoxLayout(this)),
      m_x_label(new QLabel("x: 0")), m_y_label(new QLabel("y: 0")),
      m_z_label(new QLabel("z: 0")), m_calibration_button(new QPushButton()),
      _calibration_cancel_button(new QPushButton()) {
  auto values_layout = new QHBoxLayout();
  m_layout->addWidget(new QLabel(tr("Accelerometr")));
  m_layout->addLayout(values_layout);

  values_layout->addWidget(m_x_label);
  values_layout->addWidget(m_y_label);
  values_layout->addWidget(m_z_label);
  values_layout->addWidget(m_calibration_button);
  values_layout->addWidget(_calibration_cancel_button);

  m_calibration_button->setText(tr("Calibration"));

  _calibration_cancel_button->setText(tr("Cancel calibration"));

  connect(m_calibration_button, &QPushButton::pressed, this,
          &AccelerometerInfoWidget::handleCalibrationButtonPress);
  connect(_calibration_cancel_button, &QPushButton::pressed, this,
          &AccelerometerInfoWidget::handleCalibrationCancelButtonPress);
}

void AccelerometerInfoWidget::handleIMUUpdate(uint16_t x, uint16_t y,
                                              uint16_t z) {
  m_x_label->setText(QString("x: %1").arg(x));
  m_y_label->setText(QString("y: %1").arg(y));
  m_z_label->setText(QString("z: %1").arg(z));
}

void AccelerometerInfoWidget::handleCalibrationButtonPress() {
  emit startCalibration();
}

void AccelerometerInfoWidget::handleCalibrationCancelButtonPress() {
  emit cancelCalibration();
}
