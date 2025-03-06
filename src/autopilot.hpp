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
	Sending
};

enum class AutopilotParamsUploadState {
	None,
	Uploading
};

class Autopilot : public QObject {
	Q_OBJECT

public:
	explicit Autopilot(QObject *parent = nullptr);
	void setState(const AutopilotState &);
	void setParamsState(const AutopilotParamsState &);
	void setParamsSendState(const AutopilotParamsUploadState &);
	const AutopilotState &getState() const;
	const AutopilotParamsState &getParamsState() const;
	const AutopilotParamsUploadState &getParamsUploadState() const;

signals:
	void stateUpdated(const AutopilotState &new_state);
	void paramsStateUpdated(const AutopilotParamsState &new_params_state);
	void
	paramsSendStateUpdated(const AutopilotParamsUploadState &new_params_send_state);

private:
	AutopilotState _state = AutopilotState::None;
	AutopilotParamsState _params_state = AutopilotParamsState::None;
	AutopilotParamsUploadState _params_send_state = AutopilotParamsUploadState::None;
};

#endif // AUTOPILOT_H
