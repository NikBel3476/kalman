#ifndef PARAMETERSPAGE_H
#define PARAMETERSPAGE_H

#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <set>

#include "mavlink/ardupilotmega/mavlink.h"

class ApParametersPage : public QWidget {
	Q_OBJECT
public:
	explicit ApParametersPage(QWidget *parent = nullptr);

signals:
	void apAllParamsReceived();

public slots:
	void handleApParamReceive(mavlink_param_value_t);

private:
	std::set<mavlink_param_value_t> _ap_params;
	QVBoxLayout *_layout = nullptr;
	QTableWidget *_ap_params_table = nullptr;

	uint16_t _params_total_count = 0;
	uint32_t _params_received = 0;
};

#endif // PARAMETERSPAGE_H
