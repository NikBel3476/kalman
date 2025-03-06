#include "parametersmanager.hpp"

/// params counter is zero based
static constexpr auto PARAM_UPLOAD_ACK_INDEX = 65535;
static constexpr auto kApParamsUpdateDelay = std::chrono::seconds{10};
static constexpr auto kParamsDownloadTimeout = std::chrono::seconds{30};
// zero based
static constexpr auto PARAMS_DOWNLOAD_ATTEMPTS_MAX = 1;

ParametersManager::ParametersManager(QObject *parent,
																		 MavlinkManager *mavlink_manager,
																		 Autopilot *autopilot)
		: QObject{parent},
			_update_params_on_ap_connect_timer(new QTimer(this)),
			_params_download_timer(new QTimer(this)),
			_mavlink_manager(mavlink_manager),
			_autopilot(autopilot) {
	_update_params_on_ap_connect_timer->setSingleShot(true);
	_params_download_timer->setSingleShot(true);

	// connections
	connect(_update_params_on_ap_connect_timer, &QTimer::timeout, this,
					&ParametersManager::updateApParameters);
	connect(_params_download_timer, &QTimer::timeout, this,
					&ParametersManager::updateApParameters);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&ParametersManager::_handleMavlinkMessageReceive);
}

void ParametersManager::updateApParameters() {
	if (_download_params_attempt_counter < PARAMS_DOWNLOAD_ATTEMPTS_MAX) {
		_params_download_timer->start(kParamsDownloadTimeout);
		_download_params_attempt_counter++;
	} else {
		qDebug() << "SECOND PARAMS REQUEST";
	}
	_autopilot->setParamsState(AutopilotParamsState::Receiving);
	_ap_params.clear();
	_mavlink_manager->sendParamRequestList();
}

void ParametersManager::handleAutopilotConnection() {
	qDebug() << "AUTOPILOT CONNECTED. STATE: "
					 << static_cast<int>(_autopilot->getState());
	if (!_not_saved_params.empty() && _autopilot->getParamsUploadState() ==
																				AutopilotParamsUploadState::Uploading) {
		// wait for autopilot parameters update
		_update_params_on_ap_connect_timer->start(kApParamsUpdateDelay);
		qDebug() << "NOT_SAVED_PARAMETERS IS NOT EMPTY";
	}
}

void ParametersManager::handleResetParamsRequest() {
	mavlink_param_value_t format_version_param{.param_value = 0.0,
																						 .param_count = _params_total_count,
																						 .param_index = 0,
																						 .param_id = {0},
																						 .param_type = MAV_PARAM_TYPE_INT8};
	memcpy(format_version_param.param_id, "FORMAT_VERSION", 14);

	_mavlink_manager->sendParamSet(format_version_param);
	std::this_thread::sleep_for(std::chrono::milliseconds{1000});

	_reset_state();
	emit paramsResetRequest();
}

void ParametersManager::handleCompareParamsRequest(
		const QByteArray &file_content) {
	_reset_state();

	// parse ap parameters
	const auto file_str = QString(file_content);
	static const auto endline_regex = QRegularExpression("[\r\n]");
	static const auto comment_regex = QRegularExpression("^[^#].*");
	const auto params_str =
			file_str.split(endline_regex, Qt::SkipEmptyParts).filter(comment_regex);

	std::unordered_map<std::string, float> params_from_file;
	for (const auto &param_str : params_str) {
		const auto param_value_str = param_str.split(',');
		auto key = QString(16, '\0');
		key.replace(0, param_value_str[0].length(), param_value_str[0]);
		params_from_file[key.toStdString()] = param_value_str[1].toFloat();
	}
	_not_saved_params = params_from_file;

	// remove all equal parameters
	for (const auto &[ap_param_key, ap_param] : _ap_params) {
		if (_not_saved_params.contains(ap_param_key) &&
				ap_param.param_value == _not_saved_params[ap_param_key]) {
			_not_saved_params.erase(ap_param_key);
		}
	}

	qDebug() << "PARAMS COMPARING COMPLETE. NOT_MATCHING:"
					 << _not_saved_params.size();
	if (_not_saved_params.empty()) {
		emit paramsCompareCompleted(ParametersCompareResult::Matching,
																_not_saved_params);
	} else {
		emit paramsCompareCompleted(ParametersCompareResult::NotMatching,
																_not_saved_params);
	}
}

void ParametersManager::handleUploadParamsRequest() {
	_uploadParameters();
}

void ParametersManager::_handleMavlinkMessageReceive(
		const mavlink_message_t &msg) {
	switch (msg.msgid) {
	case MAVLINK_MSG_ID_PARAM_VALUE: {
		mavlink_param_value_t param_value;
		mavlink_msg_param_value_decode(&msg, &param_value);
		_handleApParamReceive(param_value);
	} break;
	default:
		break;
	}
}

void ParametersManager::_handleApParamReceive(
		mavlink_param_value_t param_value) {
	switch (_autopilot->getParamsUploadState()) {
	case AutopilotParamsUploadState::None: {
		if (param_value.param_count != _params_total_count) {
			_params_total_count = param_value.param_count;
			qDebug() << "PARAM TOTAL COUNT CHANGED";
			emit paramsTotalCountUpdated(_params_total_count);
		}

		if (_autopilot->getParamsState() == AutopilotParamsState::Receiving) {
			_ap_params[std::string(param_value.param_id, 16)] = param_value;
			emit paramsCountUpdated(_ap_params.size());

			if (_ap_params.size() == _params_total_count) {
				qDebug() << "ALL PARAMETERS RECEIVED";
				qDebug() << "NOT SAVED PARAMS SIZE: " << _not_saved_params.size();
				_autopilot->setParamsState(AutopilotParamsState::None);
				emit allParamsReceived(_ap_params);
				_params_download_timer->stop();
				_download_params_attempt_counter = 0;
			}
		}
	} break;
	case AutopilotParamsUploadState::Uploading: {
		switch (_autopilot->getParamsState()) {
		case AutopilotParamsState::None: {
			// qDebug() << "PARAMS STATE NONE";
		} break;
		case AutopilotParamsState::Receiving: {
			// qDebug() << "PARAMS STATE RECEIVING";
			if (param_value.param_count != _params_total_count) {
				_params_total_count = param_value.param_count;
				qDebug() << "PARAM TOTAL COUNT CHANGED";
				emit paramsTotalCountUpdated(_params_total_count);
			}
			_ap_params[std::string(param_value.param_id, 16)] = param_value;
			emit paramsCountUpdated(_ap_params.size());

			// all parameters received
			if (_ap_params.size() == _params_total_count) {
				// check saved parameters from last write operation
				for (const auto &[ap_param_key, ap_param] : _ap_params) {
					if (_not_saved_params.contains(ap_param_key) &&
							_not_saved_params[ap_param_key] == ap_param.param_value) {
						_not_saved_params.erase(ap_param_key);
						qDebug() << "PARAM SAVED: " << ap_param_key;
					}
				}

				emit allParamsReceived(_ap_params);
				_params_download_timer->stop();
				_download_params_attempt_counter = 0;
				_autopilot->setParamsState(AutopilotParamsState::None);
				qDebug() << "ALL PARAMETERS RECEIVED";
				qDebug() << "NOT SAVED PARAMS SIZE: " << _not_saved_params.size();

				if (!_not_saved_params.empty()) {
					if (_params_upload_attempt_counter >= PARAMS_UPLOAD_ATTEMPT_MAX) {
						qDebug() << "PARAMS UPLOAD MAX ATTEMPT NUMBER REACHED";
						emit paramsUploaded(ParametersUploadResult::NotAllSaved,
																_not_saved_params);
						_reset_state();
						return;
					}
					_uploadParameters();
				} else {
					emit paramsUploaded(ParametersUploadResult::Ok, _not_saved_params);
					_reset_state();
				}
			}
		} break;
		case AutopilotParamsState::Sending: {
			// qDebug() << "PARAMS STATE SENDING";
			if (param_value.param_index == PARAM_UPLOAD_ACK_INDEX) {
				_handleParamUploadAck(param_value);
			}
		} break;
		}
	} break;
	}
}

void ParametersManager::_handleParamUploadAck(mavlink_param_value_t &param) {
	(void)(param);
	_uploadApParam();
}

void ParametersManager::_reset_state() {
	_not_saved_params.clear();
	_params_upload_attempt_counter = 0;
	_autopilot->setParamsSendState(AutopilotParamsUploadState::None);
	_autopilot->setParamsState(AutopilotParamsState::None);
}

void ParametersManager::_uploadParameters() {
	// _send_param_timer->start(kSendParamTimeout);
	qDebug() << "PARAMS UPLOAD ATTEMPT: " << _params_upload_attempt_counter;
	emit paramsUploadAttemptUpdated(_params_upload_attempt_counter);
	_params_upload_attempt_counter++;
	_autopilot->setParamsSendState(AutopilotParamsUploadState::Uploading);
	_autopilot->setParamsState(AutopilotParamsState::Sending);
	_params_to_upload_buffer.clear();
	for (const auto &[not_saved_param_key, not_saved_param_value] :
			 _not_saved_params) {
		if (_ap_params.contains(not_saved_param_key)) {
			auto new_ap_param = _ap_params[not_saved_param_key];
			new_ap_param.param_value = not_saved_param_value;
			_params_to_upload_buffer[not_saved_param_key] = new_ap_param;
		}
	}
	_uploadApParam();
}

void ParametersManager::_uploadApParam() {
	if (_params_to_upload_buffer.empty()) {
		_autopilot->setParamsState(AutopilotParamsState::None);
		emit parametersWritten();
		return;
	}
	auto [param_to_upload_key, param] = *_params_to_upload_buffer.begin();
	_mavlink_manager->sendParamSet(param);
	_params_to_upload_buffer.erase(param_to_upload_key);
}
