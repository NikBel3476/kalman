#include "magnetometerinfowidget.h"

MagnetometerInfoWidget::MagnetometerInfoWidget(QWidget *parent)
    : QWidget{parent}, m_layout(new QVBoxLayout(this)),
      m_x_label(new QLabel("x: 0")), m_y_label(new QLabel("y: 0")),
      m_z_label(new QLabel("z: 0")) {
  auto values_layout = new QHBoxLayout();
  m_layout->addWidget(new QLabel(tr("Magnetometr")));
  m_layout->addLayout(values_layout);

  values_layout->addWidget(m_x_label);
  values_layout->addWidget(m_y_label);
  values_layout->addWidget(m_z_label);
}

void MagnetometerInfoWidget::handleIMUUpdate(uint16_t x, uint16_t y,
                                             uint16_t z) {
  m_x_label->setText(QString("x: %1").arg(x));
  m_y_label->setText(QString("y: %1").arg(y));
  m_z_label->setText(QString("z: %1").arg(z));
}
