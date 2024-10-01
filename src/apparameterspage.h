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

#include "mavlink/ardupilotmega/mavlink.h"

class ApParametersPage : public QWidget {
	Q_OBJECT
public:
	explicit ApParametersPage(QWidget *parent = nullptr);

signals:
	void requestDownloadParams();
	void apAllParamsReceived();
	void requestUploadApParams(std::vector<mavlink_param_value_t>);

public slots:
	void handleApParamReceive(mavlink_param_value_t);
	void
	handleApParamsUploadCompletion(const std::vector<mavlink_param_value_t> &);

private slots:
	void _handleUpdateParamsButtonClick();
	void _handleCompareParamsButtonClick();
	void _handleUploadParamsButtonClick();

private:
	void _parseApParameters(const QByteArray &);

	QVBoxLayout *_layout = nullptr;
	QPushButton *_update_params_btn = nullptr;
	QPushButton *_compare_params_btn = nullptr;
	QPushButton *_upload_params_btn = nullptr;
	QTableWidget *_ap_params_table = nullptr;
	QProgressBar *_upload_params_progress_bar = nullptr;

	std::map<std::string, mavlink_param_value_t> _ap_params;
	std::vector<mavlink_param_value_t> _params_to_change;
	uint16_t _params_total_count = 0;
	uint32_t _params_received = 0;
};

#endif // PARAMETERSPAGE_H
