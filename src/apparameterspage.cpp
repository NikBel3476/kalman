#include "apparameterspage.hpp"

static constexpr int ap_params_table_column_count = 3;
static constexpr auto kSendParamTimeout = std::chrono::seconds{1};
static constexpr auto kApParamsUpdateDelay = std::chrono::seconds{10};
static constexpr uint8_t _params_upload_attempt_max =
		6; // params conuter is zero based

ApParametersPage::ApParametersPage(QWidget *parent,
																	 MavlinkManager *mavlink_manager,
																	 Autopilot *autopilot)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_upload_params_progress_wrapper(new QWidget()),
			_update_params_btn(new QPushButton()),
			_compare_params_btn(new QPushButton()),
			_upload_params_btn(new QPushButton()),
			_reset_params_btn(new QPushButton()),
			_file_name_label(new QLabel()),
			_ap_params_table(new QTableWidget()),
			_download_params_progress_bar(new QProgressBar()),
			_upload_params_progress_bar(new QProgressBar()),
			_send_param_timer{new QTimer(this)},
			_update_params_on_ap_connect_timer{new QTimer(this)},
			_mavlink_manager{mavlink_manager},
			_autopilot{autopilot} {
	const auto buttons_layout = new QHBoxLayout();
	_layout->addLayout(buttons_layout);
	_layout->addWidget(_file_name_label), _layout->addWidget(_ap_params_table);
	_layout->addWidget(_download_params_progress_bar);
	_layout->addWidget(_upload_params_progress_wrapper);

	buttons_layout->addWidget(_update_params_btn);
	buttons_layout->addWidget(_compare_params_btn);
	buttons_layout->addWidget(_upload_params_btn);
	buttons_layout->addWidget(_reset_params_btn);

	const auto &upload_params_progress_label = new QLabel();
	const auto &upload_params_progress_layout = new QHBoxLayout();
	upload_params_progress_layout->addWidget(upload_params_progress_label);
	upload_params_progress_layout->addWidget(_upload_params_progress_bar);

	upload_params_progress_label->setText(tr("Wait for upload completion"));

	_update_params_btn->setText(tr("Update"));
	// _update_params_btn->setEnabled(false);
	_compare_params_btn->setText(tr("Compare parameters"));
	_compare_params_btn->setEnabled(false);
	_upload_params_btn->setText(tr("Upload parameters"));
	_upload_params_btn->setEnabled(false);
	_reset_params_btn->setText(tr("Reset parameters"));
	_reset_params_btn->setEnabled(true);

	_file_name_label->setVisible(false);

	_ap_params_table->setColumnCount(ap_params_table_column_count);
	_ap_params_table->setHorizontalHeaderLabels(
			QStringList{tr("Name"), tr("Value"), tr("Comparing value")});
	_ap_params_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

	_download_params_progress_bar->setVisible(false);

	_upload_params_progress_wrapper->setLayout(upload_params_progress_layout);
	_upload_params_progress_wrapper->setVisible(false);

	_upload_params_progress_bar->setMaximum(_params_upload_attempt_max);

	_send_param_timer->setSingleShot(true);
	_update_params_on_ap_connect_timer->setSingleShot(true);

	// connections
	connect(_update_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleUpdateParamsButtonClick);
	connect(_compare_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleCompareParamsButtonClick);
	connect(_upload_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleUploadParamsButtonClick);
	connect(_reset_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleResetParamsButtonClick);
	connect(_send_param_timer, &QTimer::timeout, this,
					&ApParametersPage::_handleParameterSendTimeout);
	connect(_update_params_on_ap_connect_timer, &QTimer::timeout, this,
					&ApParametersPage::updateApParameters);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&ApParametersPage::_handleMavlinkMessageReceive);
	connect(this, &ApParametersPage::requestDownloadParams, _mavlink_manager,
					&MavlinkManager::sendParamRequestList);
}

void ApParametersPage::clearParamsToUpload() {
	for (int row_index = 0; row_index < _ap_params_table->rowCount();
			 row_index++) {
		const auto blank_table_widget =
				new QTableWidgetItem("", QTableWidgetItem::Type);
		_ap_params_table->setItem(row_index, ap_params_table_column_count - 1,
															blank_table_widget);
	}
	_params_to_upload.clear();
}

void ApParametersPage::clearNotSavedParams() {
	_not_saved_params.clear();
}

void ApParametersPage::updateApParameters() {
	// _update_params_btn->setEnabled(false);
	_upload_params_btn->setEnabled(false);
	_compare_params_btn->setEnabled(false);
	_autopilot->setParamsState(AutopilotParamsState::Receiving);
	_ap_params.clear();
	_ap_params_table->setRowCount(0);
	_mavlink_manager->sendParamRequestList();
}

void ApParametersPage::_handleApParamsUploadCompletion() {
	emit parametersWritten();
	_upload_params_progress_bar->setValue(_params_upload_attempt_counter);
	_params_upload_attempt_counter++;
}

void ApParametersPage::_handleParameterSendTimeout() {
	// QMessageBox::warning(this, tr("Warning"), tr("Parameter write timeout"));
}

void ApParametersPage::handleAutopilotConnection() {
	qDebug() << "AUTOPILOT CONNECTED. STATE: "
					 << static_cast<int>(_autopilot->getState());
	if (!_not_saved_params.empty() &&
			_autopilot->getParamsSendState() == AutopilotParamsSendState::Sending) {
		_update_params_on_ap_connect_timer->start(
				kApParamsUpdateDelay); // wait for autopilot parameters update
		qDebug() << "NOT_SAVED_PARAMETERS IS NOT EMPTY";
	}
}

void ApParametersPage::_handleUpdateParamsButtonClick() {
	updateApParameters();
}

void ApParametersPage::_handleCompareParamsButtonClick() {
	auto fileContentReady = [this](const QString &file_name,
																 const QByteArray &file_content) {
		if (!file_name.isEmpty()) {
			_update_params_btn->setEnabled(false);
			_compare_params_btn->setEnabled(false);
			_upload_params_btn->setEnabled(false);
			_file_name_label->setText(
					tr("Comparing file: %1")
							.arg(file_name.mid(file_name.lastIndexOf(QChar('/')) + 1)));
			_file_name_label->setVisible(true);
			clearNotSavedParams();
			clearParamsToUpload();
			_parseApParameters(file_content);
		}
	};
	QFileDialog::getOpenFileContent("*.param", fileContentReady);
}

void ApParametersPage::_handleUploadParamsButtonClick() {
	_uploadParameters();
}

void ApParametersPage::_handleResetParamsButtonClick() {
	mavlink_param_value_t format_version_param{.param_value = 0.0,
																						 .param_count = _params_total_count,
																						 .param_index = 0,
																						 .param_id = {0},
																						 .param_type = MAV_PARAM_TYPE_INT8};
	memcpy(format_version_param.param_id, "FORMAT_VERSION", 14);

	_mavlink_manager->sendParamSet(format_version_param);
	std::this_thread::sleep_for(std::chrono::milliseconds{1000});
	clearNotSavedParams();
	_autopilot->setParamsState(AutopilotParamsState::None);
	_autopilot->setParamsSendState(AutopilotParamsSendState::None);
	QMessageBox::information(
			this, tr("Information"),
			tr("Parameters have been reset. Autopilot will be rebooted"));
	emit paramsResetRequest();
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
	static const auto comment_regex = QRegularExpression("^[^#].*");
	const auto params_str =
			file_str.split(line_regex, Qt::SkipEmptyParts).filter(comment_regex);

	std::unordered_map<std::string, float> params_from_file;
	for (const auto &param_str : params_str) {
		const auto param_value_str = param_str.split(',');
		auto key = QString(16, '\0');
		key.replace(0, param_value_str[0].length(), param_value_str[0]);
		params_from_file[key.toStdString()] = param_value_str[1].toFloat();
	}
	_not_saved_params = params_from_file;
	qDebug() << "NOT_SAVED_PARAMS TOTAL COUNT: " << _not_saved_params.size();

	std::vector<mavlink_param_value_t> comparing_params;
	std::vector<std::string> equal_param_ids;
	std::unordered_map<std::string, float> not_found_params;
	for (auto const &not_saved_param : _not_saved_params) {
		if (_ap_params.contains(not_saved_param.first)) {
			const auto ap_param = _ap_params[not_saved_param.first];
			auto comparing_param = ap_param;
			comparing_param.param_value = not_saved_param.second;
			comparing_params.push_back(comparing_param);
			if (not_saved_param.second != ap_param.param_value) {
				_params_to_upload.push_back(comparing_param);
			} else {
				equal_param_ids.push_back(not_saved_param.first);
			}
		} else {
			not_found_params.insert(not_saved_param);
		}
	}

	for (const auto &equal_param_id : equal_param_ids) {
		_not_saved_params.erase(equal_param_id);
	}

	_ap_params_table->setRowCount(static_cast<int>(_ap_params.size()));
	if (_params_to_upload.empty()) {
		if (_not_saved_params.empty()) {
			QMessageBox::information(this, tr("Informataion"),
															 tr("Parameters are matching"));
		} else {
			QMessageBox::information(this, tr("Warning"),
															 tr("Not all parameters found"));
		}
		qDebug() << "NOT SAVED PARAMS SIZE: " << _not_saved_params.size();
		for (const auto &not_saved_param : _not_saved_params) {
			qDebug() << "PARAM " << not_saved_param.first << "NOT FOUND";
		}
	} else {
		for (const auto &comparing_param : comparing_params) {
			const auto param_name = QString::number(comparing_param.param_value);
			const auto param_name_formatted =
					param_name.left(param_name.indexOf(QChar('\0')));
			const auto &new_param_value_item =
					new QTableWidgetItem(param_name_formatted, QTableWidgetItem::Type);
			for (const auto &[_, ap_param] : _ap_params) {
				if (ap_param.param_index == comparing_param.param_index &&
						ap_param.param_value != comparing_param.param_value) {
					new_param_value_item->setBackground(QBrush(Qt::red));
				}
			}
			_ap_params_table->setItem(comparing_param.param_index,
																ap_params_table_column_count - 1,
																new_param_value_item);
		}
		QMessageBox::warning(this, tr("Warning"),
												 tr("Parameters are not matching"));
		_upload_params_btn->setEnabled(true);
	}

	// show not found params
	auto item_index = _ap_params_table->rowCount();
	_ap_params_table->setRowCount(_ap_params_table->rowCount() +
																static_cast<int>(not_found_params.size()));
	for (const auto &[not_found_param_id, not_found_param_value] :
			 not_found_params) {
		const auto param_name = QString::fromStdString(not_found_param_id);
		const auto param_name_formatted =
				param_name.left(param_name.indexOf(QChar('\0')));
		const auto &param_id_item =
				new QTableWidgetItem(param_name_formatted, QTableWidgetItem::Type);
		param_id_item->setBackground(QBrush(Qt::transparent));
		_ap_params_table->setItem(item_index, 0, param_id_item);

		const auto &ap_param_value_item =
				new QTableWidgetItem(tr("Not found"), QTableWidgetItem::Type);
		ap_param_value_item->setBackground(QBrush(Qt::transparent));
		_ap_params_table->setItem(item_index, 1, ap_param_value_item);

		const auto &not_found_param_item = new QTableWidgetItem(
				QString::number(not_found_param_value), QTableWidgetItem::Type);
		not_found_param_item->setBackground(QBrush(Qt::red));
		_ap_params_table->setItem(item_index, ap_params_table_column_count - 1,
															not_found_param_item);
		item_index++;
	}

	_update_params_btn->setEnabled(true);
	_compare_params_btn->setEnabled(true);
	_reset();
}

void ApParametersPage::_uploadApParam() {
	if (_params_to_upload.empty()) {
		_send_param_timer->stop();
		_autopilot->setParamsSendState(AutopilotParamsSendState::None);
		return;
	}
	auto param = _params_to_upload.back();
	_mavlink_manager->sendParamSet(param);
}

void ApParametersPage::_handleApParamReceive(
		mavlink_param_value_t param_value) {
	const auto written_param_index = 65535;
	if (param_value.param_index == written_param_index &&
			_autopilot->getParamsState() != AutopilotParamsState::Receiving) {
		// param write ack
		_handleParamUploadAck(param_value);
		return;
	}

	if (param_value.param_count != _params_total_count) {
		_params_total_count_have_been_changed = true;
		_params_total_count = param_value.param_count;
		qDebug() << "PARAM TOTAL COUNT CHANGED";
	}
	if (param_value.param_index == 0) {
		_ap_params_table->setRowCount(_params_total_count);
		_download_params_progress_bar->setMaximum(_params_total_count);
		_download_params_progress_bar->setVisible(true);
		qDebug() << "PARAMS TOTAL COUNT: " << param_value.param_count;
		qDebug() << "AP PARAMS SIZE: " << _ap_params.size();
	}
	// update values inside table
	const auto param_name = QString::fromUtf8(param_value.param_id, 16);
	const auto param_name_formatted =
			param_name.left(param_name.indexOf(QChar('\0')));
	const auto &table_item_name =
			new QTableWidgetItem(param_name_formatted, QTableWidgetItem::Type);
	const auto &table_item_value = new QTableWidgetItem(
			QString::number(param_value.param_value), QTableWidgetItem::Type);
	_ap_params_table->setItem(param_value.param_index, 0, table_item_name);
	_ap_params_table->setItem(param_value.param_index, 1, table_item_value);
	_ap_params[std::string(param_value.param_id, 16)] = param_value;
	_download_params_progress_bar->setValue(
			static_cast<int32_t>(_ap_params.size()));

	if (_ap_params.size() == _params_total_count) {
		// check saved parameters from last write operation
		for (const auto &ap_param : _ap_params) {
			const auto &param_id = std::string(ap_param.second.param_id, 16);
			if (_not_saved_params.contains(param_id)) {
				auto not_saved_param_value = _not_saved_params[param_id];
				auto ap_param_value = ap_param.second.param_value;
				if (not_saved_param_value == ap_param_value) {
					_not_saved_params.erase(param_id);
					qDebug() << "PARAM SAVED: " << param_id;
					_params_have_been_saved = true;
				}
			}
			if (_cannot_save_params.contains(ap_param.first) &&
					_cannot_save_params[ap_param.first].param_value ==
							ap_param.second.param_value) {
				_cannot_save_params.erase(ap_param.first);
			}
		}
		_autopilot->setParamsState(AutopilotParamsState::Received);
		_update_params_btn->setEnabled(true);
		_compare_params_btn->setEnabled(true);
		_download_params_progress_bar->setVisible(false);
		qDebug() << "ALL PARAMETERS RECEIVED";
		qDebug() << "NOT SAVED PARAMS SIZE: " << _not_saved_params.size();
		if (_autopilot->getParamsSendState() != AutopilotParamsSendState::Sending) {
			return;
		}
		if (!_not_saved_params.empty()) {
			// TODO: move loop to separate function
			for (auto const &not_saved_param : _not_saved_params) {
				if (_ap_params.contains(not_saved_param.first)) {
					const auto &ap_param = _ap_params[not_saved_param.first];
					auto comparing_param = ap_param;

					comparing_param.param_value = not_saved_param.second;
					if (not_saved_param.second != ap_param.param_value) {
						_params_to_upload.push_back(comparing_param);
					}
				} else {
					qDebug() << "PARAM " << not_saved_param.first << "NOT FOUND";
				}
			}

			if (_params_upload_attempt_counter >= _params_upload_attempt_max) {
				qDebug() << "PARAMS UPLOAD MAX ATTEMPT NUMBER REACHED";
				QMessageBox::warning(this, tr("Warning"),
														 tr("Not all parameters saved"));
				_autopilot->setParamsSendState(AutopilotParamsSendState::None);
				for (const auto &[_, cannot_save_param] : _cannot_save_params) {
					qDebug() << std::string(cannot_save_param.param_id, 16)
									 << "CANNOT SAVE" << cannot_save_param.param_index;
				}
				_showUploadResult();
				_reset();
				return;
			}

			qDebug() << "PARAMS TO UPLOAD SIZE: " << _params_to_upload.size();
			qDebug() << "PARAMS HAVE BEEN SAVED " << _params_have_been_saved;
			if (_params_to_upload.empty()) {
				if (_params_have_been_saved) {
					qDebug() << "PARAMS TO UPLOAD IS EMPTY. REBOOTING...";
					_params_have_been_saved = false;
					_handleApParamsUploadCompletion();
				} else if (_params_total_count_have_been_changed) {
					qDebug() << "PARAMS TOTAL COUNT CHANGED. REBOOTING...";
					_params_total_count_have_been_changed = false;
					_handleApParamsUploadCompletion();
				} else {
					if (_not_saved_params.empty()) {
						QMessageBox::information(this, tr("Information"),
																		 tr("Parameters saved"));
					} else {
						QMessageBox::warning(this, tr("Warning"),
																 tr("Not all parameters saved"));
					}
					_autopilot->setParamsSendState(AutopilotParamsSendState::None);
					for (const auto &[_, cannot_save_param] : _cannot_save_params) {
						qDebug() << std::string(cannot_save_param.param_id, 16)
										 << "CANNOT SAVE" << cannot_save_param.param_index;
					}
					_showUploadResult();
					_reset();
				}
				return;
			}
			_uploadParameters();
			_params_have_been_saved = false;
			return;
		} else { // _not_saved_params is empty
			QMessageBox::information(this, tr("Information"), tr("Parameters saved"));
			_autopilot->setParamsSendState(AutopilotParamsSendState::None);
			_showUploadResult();
			_reset();
		}
	}
}

void ApParametersPage::_handleParamUploadAck(mavlink_param_value_t &param) {
	if (_autopilot->getParamsSendState() == AutopilotParamsSendState::Sending &&
			!_params_to_upload.empty()) {
		_send_param_timer->start(kSendParamTimeout);
		const auto param_to_upload = _params_to_upload.back();
		if (param_to_upload.param_value != param.param_value) {
			_not_saved_params.erase(std::string(param.param_id, 16));
			// _cannot_save_params.push_back(param_to_upload);
			_cannot_save_params[std::string(param_to_upload.param_id, 16)] =
					param_to_upload;
		}

		_params_to_upload.pop_back();
		if (_params_to_upload.empty()) {
			_send_param_timer->stop();
			_handleApParamsUploadCompletion();
			_not_written_params.clear();
		} else {
			_uploadApParam();
		}
	}
}

void ApParametersPage::_uploadParameters() {
	_upload_params_btn->setEnabled(false);
	_upload_params_progress_wrapper->setVisible(true);
	// _upload_params_progress_bar->setValue(_params_upload_attempt_counter);

	_send_param_timer->start(kSendParamTimeout);
	_autopilot->setParamsSendState(AutopilotParamsSendState::Sending);
	// _params_upload_attempt_counter++;
	qDebug() << "PARAMS TO UPLOAD SIZE: " << _params_to_upload.size();
	qDebug() << "PARAMS UPLOAD ATTEMPT: " << _params_upload_attempt_counter;
	if (_params_to_upload.empty()) {
		return;
	}
	_uploadApParam();
}

void ApParametersPage::_showUploadResult() {
	_upload_params_progress_wrapper->setVisible(false);
	_upload_params_progress_bar->setValue(0);
	for (const auto &param_to_upload : _params_to_upload) {
		_cannot_save_params[std::string(param_to_upload.param_id, 16)] =
				param_to_upload;
	}
	clearParamsToUpload();
	for (const auto &[_, not_saved_param] : _cannot_save_params) {
		const auto not_saved_param_item = new QTableWidgetItem(
				QString::number(not_saved_param.param_value), QTableWidgetItem::Type);
		not_saved_param_item->setBackground(QBrush(Qt::red));
		_ap_params_table->setItem(
				_ap_params[std::string(not_saved_param.param_id, 16)].param_index,
				ap_params_table_column_count - 1, not_saved_param_item);
	}
	auto item_index = _ap_params_table->rowCount();
	_ap_params_table->setRowCount(_ap_params_table->rowCount() +
																static_cast<int>(_not_saved_params.size()));
	for (const auto &[not_found_param_id, not_found_param_value] :
			 _not_saved_params) {
		const auto param_name = QString::fromStdString(not_found_param_id);
		const auto param_name_formatted =
				param_name.left(param_name.indexOf(QChar('\0')));
		const auto &param_id_item =
				new QTableWidgetItem(param_name_formatted, QTableWidgetItem::Type);
		param_id_item->setBackground(QBrush(Qt::transparent));
		_ap_params_table->setItem(item_index, 0, param_id_item);

		const auto &ap_param_value_item =
				new QTableWidgetItem(tr("Not found"), QTableWidgetItem::Type);
		ap_param_value_item->setBackground(QBrush(Qt::transparent));
		_ap_params_table->setItem(item_index, 1, ap_param_value_item);

		const auto &not_found_param_item = new QTableWidgetItem(
				QString::number(not_found_param_value), QTableWidgetItem::Type);
		not_found_param_item->setBackground(QBrush(Qt::red));
		_ap_params_table->setItem(item_index, ap_params_table_column_count - 1,
															not_found_param_item);
		item_index++;
	}
}

void ApParametersPage::_reset() {
	_params_upload_attempt_counter = 0;
}
