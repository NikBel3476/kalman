#ifndef ACCELEROMETERINFOWIDGET_H
#define ACCELEROMETERINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "sensor.h"
#include <ardupilotmega/mavlink.h>

enum class CalibrationResult { Success, Failed };

class AccelerometerInfoWidget : public QWidget {
	Q_OBJECT
public:
	explicit AccelerometerInfoWidget(QWidget *parent = nullptr);

signals:
	void startAccelCal();
	void startLevelCal();

public slots:
	void handleIMUUpdate(uint16_t, uint16_t, uint16_t);
	void handleAccelStatusUpdate(SensorStatus);
	void handleAccelCalComplete(CalibrationResult);
	void handleLvlCalComplete();

private slots:
	void _handleAccelCalBtnPress();
	void _handleLvlCalBtnPress();

private:
	QVBoxLayout *_layout = nullptr;
	QLabel *_title_label = nullptr;
	QLabel *_status_label = nullptr;
	QLabel *_x_label = nullptr;
	QLabel *_y_label = nullptr;
	QLabel *_z_label = nullptr;
	QPushButton *_accel_cal_btn = nullptr;
	QPushButton *_lvl_cal_btn = nullptr;
	QLabel *_cal_result_label = nullptr;
};

#endif // ACCELEROMETERINFOWIDGET_H
