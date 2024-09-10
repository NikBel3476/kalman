#ifndef MAGNETOMETRINFOWIDGET_H
#define MAGNETOMETRINFOWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class MagnetometrInfoWidget : public QWidget {
  Q_OBJECT
public:
  explicit MagnetometrInfoWidget(QWidget *parent = nullptr);

signals:

public slots:
  void handleIMUUpdate(uint16_t x, uint16_t y, uint16_t z);

private:
  QVBoxLayout *m_layout = nullptr;;
  QLabel *m_x_label = nullptr;
  QLabel *m_y_label = nullptr;
  QLabel *m_z_label = nullptr;
};

#endif // MAGNETOMETRINFOWIDGET_H
