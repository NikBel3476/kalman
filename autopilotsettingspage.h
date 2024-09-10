#ifndef AUTOPILOTSETTINGSPAGE_H
#define AUTOPILOTSETTINGSPAGE_H

#include <QWidget>
#include <QVBoxLayout>

#include <mavlink/ardupilotmega/mavlink.h>

#include "magnetometrinfowidget.h"
#include "accelerometrinfowidget.h"
#include "gyroscopeinfowidget.h"
#include "mcuinfowidget.h"

class AutopilotSettingsPage : public QWidget {
  Q_OBJECT
public:
  explicit AutopilotSettingsPage(QWidget *parent = nullptr);

signals:

public slots:
  void handleIMUUpdate(mavlink_raw_imu_t raw_imu);
  void handlePowerStatusUpdate(mavlink_power_status_t power_status);
  void handleMcuStatusUpdate(mavlink_mcu_status_t mcu_status);

private:
  QVBoxLayout *m_layout = nullptr;
  MagnetometrInfoWidget *m_magnetometr_info_widget = nullptr;
  AccelerometrInfoWidget *m_accelerometr_info_widget = nullptr;
  GyroscopeInfoWidget *m_gyroscope_info_widget = nullptr;
  McuInfoWidget *m_mcu_info_widget = nullptr;
};

#endif // AUTOPILOTSETTINGSPAGE_H
