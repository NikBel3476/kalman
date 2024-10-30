#ifndef PARAMETERSPAGE_H
#define PARAMETERSPAGE_H

#include <QFileDialog>
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

#include <ardupilotmega/mavlink.h>

#include "mavlinkmanager.h"

class ApParametersPage : public QWidget {
	Q_OBJECT
public:
	explicit ApParametersPage(QWidget *parent, MavlinkManager *mavlink_manager,
														Autopilot *autopilot);
	void clearParamsToUpload();

signals:
	void requestDownloadParams();
	// void apAllParamsReceived();
	// void requestUploadApParams(std::vector<mavlink_param_value_t>);
	void parametersWritten();

public slots:
	void
	_handleApParamsUploadCompletion(const std::vector<mavlink_param_value_t> &);

private slots:
	void _handleUpdateParamsButtonClick();
	void _handleCompareParamsButtonClick();
	void _handleUploadParamsButtonClick();
	void _handleMavlinkMessageReceive(const mavlink_message_t &msg);

private:
	void _parseApParameters(const QByteArray &);
	void _uploadApParam();
	void _handleApParamReceive(mavlink_param_value_t);
	void _handleApAllParamsReceive();

	QVBoxLayout *_layout = nullptr;
	QPushButton *_update_params_btn = nullptr;
	QPushButton *_compare_params_btn = nullptr;
	QPushButton *_upload_params_btn = nullptr;
	QTableWidget *_ap_params_table = nullptr;
	QProgressBar *_upload_params_progress_bar = nullptr;
	QTimer *_send_param_timer = nullptr;

	std::map<std::string, mavlink_param_value_t> _ap_params;
	std::vector<mavlink_param_value_t> _params_to_change;
	std::vector<mavlink_param_value_t> _not_written_params;
	uint16_t _params_total_count = 0;
	uint32_t _params_received = 0;
	MavlinkManager *_mavlink_manager = nullptr;
	Autopilot *_autopilot = nullptr;
};

#endif // PARAMETERSPAGE_H
