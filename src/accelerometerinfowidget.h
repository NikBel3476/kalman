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
	void startAccelCal();
	void startLevelCal();

public slots:
	void handleIMUUpdate(uint16_t x, uint16_t y, uint16_t z);
	void handleAccelCalComplete();
	void handleLvlCalComplete();

private slots:
	void _handleAccelCalBtnPress();
	void _handleLvlCalBtnPress();

private:
	QVBoxLayout *_layout = nullptr;
	QLabel *_x_label = nullptr;
	QLabel *_y_label = nullptr;
	QLabel *_z_label = nullptr;
	QPushButton *_accel_cal_btn = nullptr;
	QPushButton *_lvl_cal_btn = nullptr;
	QLabel *_cal_result_label = nullptr;
};

#endif // ACCELEROMETERINFOWIDGET_H
