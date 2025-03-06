#ifndef PARAMETERSMANAGER_HPP
#define PARAMETERSMANAGER_HPP

#include <ardupilotmega/mavlink.h>
#include <thread>
#include <stack>

#include <QObject>
#include <QRegularExpression>

#include "autopilot.hpp"
#include "mavlinkmanager.hpp"

static constexpr uint8_t PARAMS_UPLOAD_ATTEMPT_MAX = 6;

enum class ParametersCompareResult {
	Matching,
	NotMatching
};

enum class ParametersUploadResult {
	Ok,
	NotAllSaved
};

class ParametersManager : public QObject {
	Q_OBJECT
public:
	explicit ParametersManager(QObject *parent, MavlinkManager* mavlink_manager, Autopilot *autopilot);

public slots:
	void updateApParameters();
	void handleAutopilotConnection();
	void handleResetParamsRequest();
	void handleCompareParamsRequest(const QByteArray &file_content);
	void handleUploadParamsRequest();

signals:
	void parametersWritten();
	void paramsResetRequest();
	/// Note: FC sometimes send parameters for no reason, so this signal may be invoked multiple times. Pleas check this
	void allParamsReceived(const std::unordered_map<std::string, mavlink_param_value_t> &ap_params);
	void paramsCompareCompleted(const ParametersCompareResult &, const std::unordered_map<std::string, float> not_matched_params);
	void paramsUploaded(const ParametersUploadResult &, const std::unordered_map<std::string, float> &not_saved_params);
	void paramsTotalCountUpdated(size_t parameters_total_count);
	void paramsCountUpdated(size_t parameters_count);
	void paramsUploadAttemptUpdated(uint8_t attempt);

private:
	void _handleMavlinkMessageReceive(const mavlink_message_t &msg);
	void _handleApParamReceive(mavlink_param_value_t);
	void _handleParamUploadAck(mavlink_param_value_t &);
	void _reset_state();
	void _uploadParameters();
	void _uploadApParam();

	QTimer *_update_params_on_ap_connect_timer = nullptr;
	QTimer *_params_download_timer = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
	Autopilot *_autopilot = nullptr;
	std::unordered_map<std::string, mavlink_param_value_t> _ap_params;
	std::unordered_map<std::string, float> _not_saved_params;
	uint16_t _params_total_count = 0;
	uint8_t _params_upload_attempt_counter = 0;
	uint8_t _download_params_attempt_counter = 0;
	std::unordered_map<std::string, mavlink_param_value_t> _params_to_upload_buffer;
};

#endif // PARAMETERSMANAGER_HPP
