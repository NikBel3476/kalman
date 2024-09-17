#ifndef MAGNETOMETERINFOWIDGET_H
#define MAGNETOMETERINFOWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "mavlink/ardupilotmega/mavlink.h"

class MagnetometerInfoWidget : public QWidget {
	Q_OBJECT
public:
	explicit MagnetometerInfoWidget(QWidget *parent = nullptr);

signals:
	void startCalibration();
	void cancelCalibration();

public slots:
	void handleIMUUpdate(uint16_t x, uint16_t y, uint16_t z);
	void handleMagCalProgressUpdate(mavlink_mag_cal_progress_t mag_cal_progress);
	void handleMagCalReportUpdate(mavlink_mag_cal_report_t mag_cal_report);

private slots:
	void _handleCalCancelButtonPress();
	void _handleCalStartButtonPress();

private:
	QVBoxLayout *_layout = nullptr;
	QLabel *_x_label = nullptr;
	QLabel *_y_label = nullptr;
	QLabel *_z_label = nullptr;
	QPushButton *_start_calibration_button = nullptr;
	QPushButton *_cancel_calibration_button = nullptr;
	QWidget *_cal_progress_container = nullptr;
	QLabel *_cal_attempt_label = nullptr;
	QLabel *_cal_step_label = nullptr;
	QProgressBar *_mag_cal_progress_bar = nullptr;
	QLabel *_cal_result_label = nullptr;

	uint8_t _cal_attempt = 0;
	uint8_t _cal_step = 0;
};

#endif // MAGNETOMETERINFOWIDGET_H
