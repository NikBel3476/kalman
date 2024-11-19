#ifndef SENSOR_H
#define SENSOR_H

enum class SensorStatus {
	NotFound,
	Enabled,
	Disabled,
	Error
};

enum class CalibrationState {
	None,
	InProgress
};

#endif // SENSOR_H
