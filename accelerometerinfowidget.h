#ifndef ACCELEROMETERINFOWIDGET_H
#define ACCELEROMETERINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "mavlink/ardupilotmega/mavlink.h"

class AccelerometerInfoWidget : public QWidget {
  Q_OBJECT
public:
  explicit AccelerometerInfoWidget(QWidget *parent = nullptr);

signals:
  void startCalibration();
  void cancelCalibration();

public slots:
  void handleIMUUpdate(uint16_t x, uint16_t y, uint16_t z);

private slots:
  void handleCalibrationButtonPress();
  void handleCalibrationCancelButtonPress();

private:
  QVBoxLayout *m_layout = nullptr;
  QLabel *m_x_label = nullptr;
  QLabel *m_y_label = nullptr;
  QLabel *m_z_label = nullptr;
  QPushButton *m_calibration_button = nullptr;
  QPushButton *_calibration_cancel_button = nullptr;
};

#endif // ACCELEROMETERINFOWIDGET_H
