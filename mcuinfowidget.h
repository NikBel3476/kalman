#ifndef MCUINFOWIDGET_H
#define MCUINFOWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

#include "mavlink/ardupilotmega/mavlink.h"

class McuInfoWidget : public QWidget {
  Q_OBJECT
public:
  explicit McuInfoWidget(QWidget *parent = nullptr);

signals:

public slots:
  void handleMcuStatusUpdate(mavlink_mcu_status_t mcu_status);

private:
  QVBoxLayout *m_layout = nullptr;
  QLabel *m_temperature_label = nullptr;
  QLabel *m_voltage_label = nullptr;
};

#endif // MCUINFOWIDGET_H
