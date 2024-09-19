#ifndef GYROSCOPEINFOWIDGET_H
#define GYROSCOPEINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

class GyroscopeInfoWidget : public QWidget {
	Q_OBJECT
public:
	explicit GyroscopeInfoWidget(QWidget *parent = nullptr);

signals:

public slots:
	void handleIMUUpdate(uint16_t x, uint16_t y, uint16_t z);

private:
	QVBoxLayout *m_layout = nullptr;
	QLabel *m_x_label = nullptr;
	QLabel *m_y_label = nullptr;
	QLabel *m_z_label = nullptr;
};

#endif // GYROSCOPEINFOWIDGET_H
