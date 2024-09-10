#ifndef ACCELEROMETRINFOWIDGET_H
#define ACCELEROMETRINFOWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class AccelerometrInfoWidget : public QWidget {
  Q_OBJECT
public:
  explicit AccelerometrInfoWidget(QWidget *parent = nullptr);

signals:

public slots:
  void handleIMUUpdate(uint16_t x, uint16_t y, uint16_t z);

private:
  QVBoxLayout *m_layout = nullptr;
  QLabel *m_x_label = nullptr;
  QLabel *m_y_label = nullptr;
  QLabel *m_z_label = nullptr;
};

#endif // ACCELEROMETRINFOWIDGET_H
