#include "apparameterspage.h"

ApParametersPage::ApParametersPage(QWidget *parent)
		: QWidget{parent}, _layout(new QVBoxLayout(this)),
			_ap_params_table(new QTableWidget()) {
	_layout->addWidget(_ap_params_table);

	_ap_params_table->setColumnCount(2);
	_ap_params_table->setHorizontalHeaderLabels(
			QStringList{tr("Name"), tr("Value")});
	_ap_params_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void ApParametersPage::handleApParamReceive(mavlink_param_value_t param_value) {
	if (param_value.param_index == 0) {
		_ap_params_table->setRowCount(param_value.param_count);
		_params_total_count = param_value.param_count;
	}
	auto table_item_name = new QTableWidgetItem(QString(param_value.param_id),
																							QTableWidgetItem::Type);
	auto table_item_value = new QTableWidgetItem(
			QString::number(param_value.param_value), QTableWidgetItem::Type);
	_ap_params_table->setItem(param_value.param_index, 0, table_item_name);
	_ap_params_table->setItem(param_value.param_index, 1, table_item_value);
	_params_received++;
	qDebug() << "COUNT: " << _params_received
					 << "TOTAL COUNT: " << _params_total_count << '\n';
	if (_params_received == _params_total_count) {
		emit apAllParamsReceived();
	}
}
