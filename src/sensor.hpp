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

enum class CalibrationAccelState {
	None,
	Level,
	LeftSide,
	RightSide,
	NoseUp,
	NoseDown,
	Back
};

#endif // SENSOR_H
