#ifndef AUTOPILOT_H
#define AUTOPILOT_H

#include <QObject>

enum class AutopilotState {
	None,
	Alive,
	Flashing
};

class Autopilot : public QObject {
	Q_OBJECT
public:
	explicit Autopilot(QObject *parent = nullptr);

	AutopilotState state = AutopilotState::None;
};

#endif // AUTOPILOT_H
