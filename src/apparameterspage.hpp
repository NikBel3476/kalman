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

#include "parametersmanager.hpp"

class ApParametersPage : public QWidget {
	Q_OBJECT
public:
	explicit ApParametersPage(QWidget *parent,
														Autopilot *autopilot, ParametersManager *parameters_manager);

signals:
	void parametersWritten();
	void paramsResetRequest();
	void allParamsReceived();

public slots:
	void updateApParameters();

private slots:
	void _handleUpdateParamsButtonClick();
	void _handleCompareParamsButtonClick();
	void _handleUploadParamsButtonClick();
	void _handleResetParamsButtonClick();
	void _handleApParamsReceived(const std::unordered_map<std::string, mavlink_param_value_t> &ap_params);
	void _handleApParametersTotalCountUpdate(size_t parameters_total_count);
	void _handleApParametersCountUpdate(size_t parameters_count);
	void _handleParamsCompareCompletion(const ParametersCompareResult &, const std::unordered_map<std::string, float> not_matched_params);
	void _handleParamsUpload(const ParametersUploadResult &, std::unordered_map<std::string, float> not_saved_parames);
	void _handleParamsUploadAttemptUpdate(uint8_t attempt);

private:
	enum class State {
		None,
		ParametersComparing,
		ParametersUploading
	};

	void _fill_fc_params_columns();
	void _reset_state();

	QVBoxLayout *_layout = nullptr;
	QWidget *_upload_params_progress_wrapper = nullptr;
	QPushButton *_update_params_btn = nullptr;
	QPushButton *_compare_params_btn = nullptr;
	QPushButton *_upload_params_btn = nullptr;
	QPushButton *_reset_params_btn = nullptr;
	QLabel *_file_name_label = nullptr;
	QTableWidget *_ap_params_table = nullptr;
	QProgressBar *_download_params_progress_bar = nullptr;
	QProgressBar *_upload_params_progress_bar = nullptr;
	QTimer *_update_params_on_ap_connect_timer = nullptr;

	ParametersManager *_parameters_manager = nullptr;
	std::unordered_map<std::string, mavlink_param_value_t> _fc_params;
	Autopilot *_autopilot = nullptr;
	State _state = State::None;
};

#endif // PARAMETERSPAGE_H
