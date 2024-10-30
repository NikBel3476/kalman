#include "apparameterspage.h"

static constexpr auto kSendParamTimeout = std::chrono::seconds{1};

ApParametersPage::ApParametersPage(QWidget *parent,
																	 MavlinkManager *mavlink_manager,
																	 Autopilot *autopilot)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_update_params_btn(new QPushButton()),
			_compare_params_btn(new QPushButton()),
			_upload_params_btn(new QPushButton()),
			_ap_params_table(new QTableWidget()),
			_upload_params_progress_bar(new QProgressBar()),
			_send_param_timer{new QTimer(this)},
			_mavlink_manager{mavlink_manager},
			_autopilot{autopilot} {
	const auto buttons_layout = new QHBoxLayout();
	_layout->addLayout(buttons_layout);
	_layout->addWidget(_ap_params_table);
	_layout->addWidget(_upload_params_progress_bar);

	buttons_layout->addWidget(_update_params_btn);
	buttons_layout->addWidget(_compare_params_btn);
	buttons_layout->addWidget(_upload_params_btn);

	_update_params_btn->setText(tr("Update"));
	_update_params_btn->setEnabled(false);
	_compare_params_btn->setText(tr("Compare parameters"));
	_compare_params_btn->setEnabled(false);
	_upload_params_btn->setText(tr("Upload parameters"));
	_upload_params_btn->setEnabled(false);

	_ap_params_table->setColumnCount(2);
	_ap_params_table->setHorizontalHeaderLabels(
			QStringList{tr("Name"), tr("Value")});
	_ap_params_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

	_upload_params_progress_bar->setVisible(false);

	// connections
	connect(_update_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleUpdateParamsButtonClick);
	connect(_compare_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleCompareParamsButtonClick);
	connect(_upload_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleUploadParamsButtonClick);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&ApParametersPage::_handleMavlinkMessageReceive);
	connect(this, &ApParametersPage::requestDownloadParams, _mavlink_manager,
					&MavlinkManager::sendParamRequestList);
}

void ApParametersPage::clearParamsToUpload() {
	const auto column_count = _ap_params_table->columnCount();
	if (column_count == 3) {
		_ap_params_table->removeColumn(column_count - 1);
	}
	_params_to_change.clear();
}

void ApParametersPage::_handleApParamsUploadCompletion(
		const std::vector<mavlink_param_value_t> &not_written_params) {
	const auto column_index = 2;
	for (const auto &param_to_change : _params_to_change) {
		_ap_params_table->item(param_to_change.param_index, column_index)
				->setBackground(QBrush(Qt::transparent));
	}
	_params_to_change.clear();
	for (const auto param : not_written_params) {
		_ap_params_table->item(param.param_index, column_index)
				->setBackground(QBrush(Qt::red));
	}
	if (!not_written_params.empty()) {
		QMessageBox::warning(
				this, tr("Warning"),
				tr("Cannot write all new parameters. Autopilot will be rebooted"));
	} else {
		QMessageBox::information(
				this, tr("Information"),
				tr("Parameters written succesfully. Autopilot will be rebooted"));
	}
	emit parametersWritten();
	// _upload_params_btn->setEnabled(true);
}

void ApParametersPage::_handleUpdateParamsButtonClick() {
	_update_params_btn->setEnabled(false);
	_upload_params_btn->setEnabled(false);
	_compare_params_btn->setEnabled(false);
	// emit requestDownloadParams();
	_mavlink_manager->sendParamRequestList();
}

void ApParametersPage::_handleCompareParamsButtonClick() {
	auto fileContentReady = [this](const QString &file_name,
																 const QByteArray &file_content) {
		if (!file_name.isEmpty()) {
			_update_params_btn->setEnabled(false);
			_compare_params_btn->setEnabled(false);
			_upload_params_btn->setEnabled(false);
			_parseApParameters(file_content);
		}
	};
	QFileDialog::getOpenFileContent("*.param", fileContentReady);
}

void ApParametersPage::_handleUploadParamsButtonClick() {
	_upload_params_btn->setEnabled(false);
	// emit requestUploadApParams(_params_to_change);
	// _params_to_change.clear();

	_send_param_timer->start(kSendParamTimeout);
	_autopilot->params_send_state = AutopilotParamsSendState::Sending;
	_uploadApParam();
}

void ApParametersPage::_handleMavlinkMessageReceive(
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

void ApParametersPage::_parseApParameters(const QByteArray &file_content) {
	const auto file_str = QString(file_content);
	static const auto line_regex = QRegularExpression("[\r\n]");
	static const auto comment_regex = QRegularExpression("^[^#]*");
	const auto params_str =
			file_str.split(line_regex, Qt::SkipEmptyParts).filter(comment_regex);
	// QStringList filtered_params_str;
	// std::copy_if(
	// 		std::execution::par,
	// 		params_str.begin(),
	// 		params_str.end(),
	// 		std::back_inserter(filtered_params_str),
	// 		[](const auto &p) {
	// 			return !p.startsWith('#');
	// 		}
	// 		);
	std::unordered_map<QString, float> new_params;
	for (const auto &param_str : params_str) {
		const auto param_value_str = param_str.split(',');
		new_params[param_value_str[0]] = param_value_str[1].toFloat();
	}

	std::vector<mavlink_param_value_t> comparing_params;
	for (auto const &[_, ap_param] : _ap_params) {
		if (new_params.contains(ap_param.param_id) /*&&
				new_params[ap_param.param_id] != ap_param.param_value*/) {
			auto comparing_param = ap_param;
			comparing_param.param_value = new_params[ap_param.param_id];
			comparing_params.push_back(comparing_param);
			if (new_params[ap_param.param_id] != ap_param.param_value) {
				_params_to_change.push_back(comparing_param);
			}
		}
	}

	if (_params_to_change.empty()) {
		QMessageBox::information(this, tr("Informataion"),
														 tr("Parameters are matching"));
	} else {
		const auto column_count = 3;
		// for (int i = 0; i < _ap_params_table->rowCount(); i++) {
		// 	const auto table_item = _ap_params_table->item(i, column_count - 1);
		// 	if (table_item != nullptr) {
		// 		table_item->setText("");
		// 	}
		// }
		_ap_params_table->removeColumn(column_count - 1);
		_ap_params_table->setColumnCount(column_count);
		const auto loaded_values_label =
				new QTableWidgetItem(tr("Comparing value"));
		_ap_params_table->setHorizontalHeaderItem(column_count - 1,
																							loaded_values_label);
		for (auto &comparing_param : comparing_params) {
			const auto new_param_value_item = new QTableWidgetItem(
					QString::number(comparing_param.param_value), QTableWidgetItem::Type);
			for (const auto &[_, ap_param] : _ap_params) {
				if (ap_param.param_index == comparing_param.param_index &&
						ap_param.param_value != comparing_param.param_value) {
					new_param_value_item->setBackground(QBrush(Qt::darkRed));
				}
			}
			_ap_params_table->setItem(comparing_param.param_index, column_count - 1,
																new_param_value_item);
		}
		QMessageBox::warning(this, tr("Warning"),
												 tr("Parameters are not matching"));
		_upload_params_btn->setEnabled(true);
	}
	_update_params_btn->setEnabled(true);
	_compare_params_btn->setEnabled(true);
}

void ApParametersPage::_uploadApParam() {
	if (_params_to_change.empty()) {
		return;
	}
	auto param = _params_to_change.back();
	_mavlink_manager->sendParamSet(param);
}

void ApParametersPage::_handleApAllParamsReceive() {
	_autopilot->params_state = AutopilotParamsState::Received;
}

void ApParametersPage::_handleApParamReceive(
		mavlink_param_value_t param_value) {
	const auto written_param_index = 65535;
	if (param_value.param_index == written_param_index) {
		// this is a written value, return right param index
		param_value.param_index =
				_ap_params[std::string(param_value.param_id)].param_index;
	}
	if (param_value.param_index == 0) {
		_params_total_count = param_value.param_count;
		_ap_params_table->setRowCount(_params_total_count);
		_upload_params_progress_bar->setMaximum(_params_total_count);
		_upload_params_progress_bar->setVisible(true);
	}
	auto table_item_name = new QTableWidgetItem(
			QString::fromUtf8(param_value.param_id), QTableWidgetItem::Type);
	auto table_item_value = new QTableWidgetItem(
			QString::number(param_value.param_value), QTableWidgetItem::Type);
	_ap_params_table->setItem(param_value.param_index, 0, table_item_name);
	_ap_params_table->setItem(param_value.param_index, 1, table_item_value);
	_ap_params[std::string(param_value.param_id)] = param_value;
	_params_received++;
	_upload_params_progress_bar->setValue(static_cast<int32_t>(_params_received));
	if (_params_received == _params_total_count) {
		// emit apAllParamsReceived();
		_handleApAllParamsReceive();
		_update_params_btn->setEnabled(true);
		_compare_params_btn->setEnabled(true);
		_upload_params_progress_bar->setVisible(false);
		_params_received = 0;
	}

	if (_autopilot->params_send_state == AutopilotParamsSendState::Sending) {
		_send_param_timer->start(kSendParamTimeout);
		const auto param = param_value;
		qDebug() << "PARAM UPLOADED ID: " << param.param_id
						 << "VALUE: " << param.param_value << '\n';
		const auto uploaded_param = _params_to_change.back();
		if (uploaded_param.param_value != param.param_value) {
			_not_written_params.push_back(uploaded_param);
		}
		_params_to_change.pop_back();
		if (_params_to_change.empty()) {
			_autopilot->params_send_state = AutopilotParamsSendState::None;
			// emit apParamsUploaded(_not_written_params);
			_handleApParamsUploadCompletion(_not_written_params);
			_not_written_params.clear();
		} else {
			_uploadApParam();
		}
	}
}
