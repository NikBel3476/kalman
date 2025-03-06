#include "autopilot.hpp"

Autopilot::Autopilot(QObject *parent)
		: QObject{parent} {}

void Autopilot::setState(const AutopilotState &new_state) {
	_state = new_state;
	emit stateUpdated(_state);
};

void Autopilot::setParamsState(const AutopilotParamsState &new_params_state) {
	_params_state = new_params_state;
	emit paramsStateUpdated(_params_state);
};

void Autopilot::setParamsSendState(
		const AutopilotParamsUploadState &new_params_send_state) {
	_params_send_state = new_params_send_state;
	emit paramsSendStateUpdated(_params_send_state);
};

const AutopilotState &Autopilot::getState() const {
	return _state;
};

const AutopilotParamsState &Autopilot::getParamsState() const {
	return _params_state;
};

const AutopilotParamsUploadState &Autopilot::getParamsUploadState() const {
	return _params_send_state;
};
