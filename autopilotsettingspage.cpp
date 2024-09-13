#include "autopilotsettingspage.h"

static const int MIN_WIDGET_WIDTH = 150;
static const int MAX_WIDGET_WIDTH = 400;

AutopilotSettingsPage::AutopilotSettingsPage(QWidget *parent)
    : QWidget{parent}, m_layout(new QVBoxLayout(this)),
      m_magnetometr_info_widget(new MagnetometerInfoWidget()),
      m_accelerometr_info_widget(new AccelerometerInfoWidget()),
      m_gyroscope_info_widget(new GyroscopeInfoWidget()),
      m_mcu_info_widget(new McuInfoWidget()) {
  m_layout->addWidget(m_magnetometr_info_widget);
  m_layout->addWidget(m_accelerometr_info_widget);
  m_layout->addWidget(m_gyroscope_info_widget);
  m_layout->addWidget(m_mcu_info_widget);

  m_magnetometr_info_widget->setSizePolicy(
      QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  m_magnetometr_info_widget->setMinimumWidth(MIN_WIDGET_WIDTH);
  m_magnetometr_info_widget->setMaximumWidth(MAX_WIDGET_WIDTH);

  m_accelerometr_info_widget->setSizePolicy(
      QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  m_accelerometr_info_widget->setMinimumWidth(MIN_WIDGET_WIDTH);
  m_accelerometr_info_widget->setMaximumWidth(MAX_WIDGET_WIDTH);

  m_gyroscope_info_widget->setSizePolicy(
      QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  m_gyroscope_info_widget->setMinimumWidth(MIN_WIDGET_WIDTH);
  m_gyroscope_info_widget->setMaximumWidth(MAX_WIDGET_WIDTH);

  m_mcu_info_widget->setSizePolicy(
      QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
  m_mcu_info_widget->setMinimumWidth(MIN_WIDGET_WIDTH);
  m_mcu_info_widget->setMaximumWidth(MAX_WIDGET_WIDTH);

  connect(m_accelerometr_info_widget,
          &AccelerometerInfoWidget::startCalibration, this,
          &AutopilotSettingsPage::handleStartCalibration);
  connect(m_accelerometr_info_widget,
          &AccelerometerInfoWidget::cancelCalibration, this,
          &AutopilotSettingsPage::handleCancelCalibration);
}

void AutopilotSettingsPage::handleIMUUpdate(mavlink_raw_imu_t raw_imu) {
  m_magnetometr_info_widget->handleIMUUpdate(raw_imu.xmag, raw_imu.ymag,
                                             raw_imu.zmag);
  m_accelerometr_info_widget->handleIMUUpdate(raw_imu.xacc, raw_imu.yacc,
                                              raw_imu.zacc);
  m_gyroscope_info_widget->handleIMUUpdate(raw_imu.xgyro, raw_imu.ygyro,
                                           raw_imu.zgyro);
}

void AutopilotSettingsPage::handlePowerStatusUpdate(
    mavlink_power_status_t power_status) {
  m_mcu_info_widget->handlePowerStatusUpdate(power_status.Vcc);
}

void AutopilotSettingsPage::handleMcuStatusUpdate(
    mavlink_mcu_status_t mcu_status) {
  m_mcu_info_widget->handleMcuStatusUpdate(mcu_status);
}

void AutopilotSettingsPage::handleStartCalibration() {
  emit startCalibration();
}

void AutopilotSettingsPage::handleCancelCalibration() {
  emit cancelCalibration();
}
