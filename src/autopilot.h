#ifndef AUTOPILOT_H
#define AUTOPILOT_H

#include <QObject>

enum class AutopilotState {
	None,
	Alive,
	Flashing
};
enum class AutopilotParamsState {
	None,
	Receiving,
	Received
};
enum class AutopilotParamsSendState {
	None,
	Sending
};

class Autopilot : public QObject {
	Q_OBJECT
public:
	explicit Autopilot(QObject *parent = nullptr);

	AutopilotState state = AutopilotState::None;
	AutopilotParamsState params_state = AutopilotParamsState::None;
	AutopilotParamsSendState params_send_state = AutopilotParamsSendState::None;
};

#endif // AUTOPILOT_H
