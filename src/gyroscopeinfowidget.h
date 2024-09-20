#ifndef GYROSCOPEINFOWIDGET_H
#define GYROSCOPEINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "sensor.h"

class GyroscopeInfoWidget : public QWidget {
	Q_OBJECT
public:
	explicit GyroscopeInfoWidget(QWidget *parent = nullptr);

signals:
	void startCalibration();

public slots:
	void handleIMUUpdate(uint16_t, uint16_t, uint16_t);
	void handleGyroStatusUpdate(SensorStatus);
	void handleGyroCalibrationComplete();

private slots:
	void _handleCalStartBtnPress();

private:
	QVBoxLayout *_layout = nullptr;
	QLabel *_title_label = nullptr;
	QLabel *_status_label = nullptr;
	QLabel *_x_label = nullptr;
	QLabel *_y_label = nullptr;
	QLabel *_z_label = nullptr;
	QPushButton *_cal_start_btn = nullptr;
	QLabel *_cal_result_label = nullptr;
};

#endif // GYROSCOPEINFOWIDGET_H
