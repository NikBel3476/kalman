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
	LevelDone,
	LeftSide,
	LeftSideDone,
	RightSide,
	RightSideDone,
	NoseUp,
	NoseUpDone,
	NoseDown,
	NoseDownDone,
	Back,
	BackDone
};

#endif // SENSOR_H
