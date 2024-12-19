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
	void setState(const AutopilotState &);
	void setParamsState(const AutopilotParamsState &);
	void setParamsSendState(const AutopilotParamsSendState &);
	const AutopilotState &getState() const;
	const AutopilotParamsState &getParamsState() const;
	const AutopilotParamsSendState &getParamsSendState() const;

signals:
	void stateUpdated(const AutopilotState &new_state);
	void paramsStateUpdated(const AutopilotParamsState &new_params_state);
	void
	paramsSendStateUpdated(const AutopilotParamsSendState &new_params_send_state);

private:
	AutopilotState _state = AutopilotState::None;
	AutopilotParamsState _params_state = AutopilotParamsState::None;
	AutopilotParamsSendState _params_send_state = AutopilotParamsSendState::None;
};

#endif // AUTOPILOT_H
