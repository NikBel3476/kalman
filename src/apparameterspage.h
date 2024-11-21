#ifndef PARAMETERSPAGE_H
#define PARAMETERSPAGE_H

#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QString>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include <set>
#include <thread>

#include <ardupilotmega/mavlink.h>

#include "mavlinkmanager.h"

class ApParametersPage : public QWidget {
	Q_OBJECT
public:
	explicit ApParametersPage(QWidget *parent, MavlinkManager *mavlink_manager,
														Autopilot *autopilot);
	void clearParamsToUpload();
	void clearNotSavedParams();
	void updateApParameters();

signals:
	void requestDownloadParams();
	// void apAllParamsReceived();
	// void requestUploadApParams(std::vector<mavlink_param_value_t>);
	void parametersWritten();

public slots:
	void handleAutopilotConnection();

private slots:
	void _handleUpdateParamsButtonClick();
	void _handleCompareParamsButtonClick();
	void _handleUploadParamsButtonClick();
	void _handleMavlinkMessageReceive(const mavlink_message_t &msg);
	void _handleApParamsUploadCompletion();
	void _handleParameterSendTimeout();

private:
	void _parseApParameters(const QByteArray &);
	void _uploadApParam();
	void _handleApParamReceive(mavlink_param_value_t);
	void _handleParamUploadAck(mavlink_param_value_t &);
	void _uploadParameters();
	void _showUploadResult();
	void _reset();

	QVBoxLayout *_layout = nullptr;
	QWidget *_upload_params_progress_wrapper = nullptr;
	QPushButton *_update_params_btn = nullptr;
	QPushButton *_compare_params_btn = nullptr;
	QPushButton *_upload_params_btn = nullptr;
	QLabel *_file_name_label = nullptr;
	QTableWidget *_ap_params_table = nullptr;
	QProgressBar *_download_params_progress_bar = nullptr;
	QProgressBar *_upload_params_progress_bar = nullptr;
	QTimer *_send_param_timer = nullptr;
	QTimer *_update_params_on_ap_connect_timer = nullptr;

	std::unordered_map<std::string, mavlink_param_value_t> _ap_params;
	std::vector<mavlink_param_value_t> _params_to_upload;
	std::vector<mavlink_param_value_t> _not_written_params;
	std::unordered_map<std::string, float> _not_saved_params;
	std::unordered_map<std::string, mavlink_param_value_t> _cannot_save_params;
	uint16_t _params_total_count = 0;
	MavlinkManager *_mavlink_manager = nullptr;
	Autopilot *_autopilot = nullptr;
	bool _params_have_been_saved = false;
	bool _params_total_count_have_been_changed = false;
	uint8_t _params_upload_attempt_counter = 0;
};

#endif // PARAMETERSPAGE_H
