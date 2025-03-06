#include "apparameterspage.hpp"

static constexpr int ap_params_table_column_count = 3;

ApParametersPage::ApParametersPage(QWidget *parent, Autopilot *autopilot,
																	 ParametersManager *parameters_manager)
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
			_parameters_manager(parameters_manager),
			_autopilot{autopilot} {
	auto *const buttons_layout = new QHBoxLayout();
	_layout->addLayout(buttons_layout);
	_layout->addWidget(_file_name_label), _layout->addWidget(_ap_params_table);
	_layout->addWidget(_download_params_progress_bar);
	_layout->addWidget(_upload_params_progress_wrapper);

	buttons_layout->addWidget(_update_params_btn);
	buttons_layout->addWidget(_compare_params_btn);
	buttons_layout->addWidget(_upload_params_btn);
	buttons_layout->addWidget(_reset_params_btn);

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
	_ap_params_table->setColumnWidth(ap_params_table_column_count - 3, 150);
	_ap_params_table->setColumnWidth(ap_params_table_column_count - 2, 100);
	_ap_params_table->setColumnWidth(ap_params_table_column_count - 1, 120);
	_ap_params_table->setHorizontalHeaderLabels(
			QStringList{tr("Name"), tr("Value"), tr("Comparing value")});
	_ap_params_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

	_download_params_progress_bar->setVisible(false);

	const auto &upload_params_progress_label = new QLabel();
	const auto &upload_params_progress_layout = new QHBoxLayout();

	upload_params_progress_label->setText(tr("Wait for upload completion"));

	upload_params_progress_layout->addWidget(upload_params_progress_label);
	upload_params_progress_layout->addWidget(_upload_params_progress_bar);

	_upload_params_progress_wrapper->setLayout(upload_params_progress_layout);
	_upload_params_progress_wrapper->setVisible(false);

	_upload_params_progress_bar->setMaximum(PARAMS_UPLOAD_ATTEMPT_MAX);

	// connections
	connect(_update_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleUpdateParamsButtonClick);
	connect(_compare_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleCompareParamsButtonClick);
	connect(_upload_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleUploadParamsButtonClick);
	connect(_reset_params_btn, &QPushButton::clicked, this,
					&ApParametersPage::_handleResetParamsButtonClick);

	// parameters manager connections
	connect(_parameters_manager, &ParametersManager::allParamsReceived, this,
					&ApParametersPage::_handleApParamsReceived);
	connect(_parameters_manager, &ParametersManager::paramsTotalCountUpdated,
					this, &ApParametersPage::_handleApParametersTotalCountUpdate);
	connect(_parameters_manager, &ParametersManager::paramsCountUpdated, this,
					&ApParametersPage::_handleApParametersCountUpdate);
	connect(_parameters_manager, &ParametersManager::paramsCompareCompleted, this,
					&ApParametersPage::_handleParamsCompareCompletion);
	connect(_parameters_manager, &ParametersManager::paramsUploaded, this,
					&ApParametersPage::_handleParamsUpload);
	connect(_parameters_manager, &ParametersManager::paramsUploadAttemptUpdated,
					this, &ApParametersPage::_handleParamsUploadAttemptUpdate);
}

void ApParametersPage::updateApParameters() {
	_upload_params_btn->setEnabled(false);
	_compare_params_btn->setEnabled(false);
	_download_params_progress_bar->setValue(0);
	_download_params_progress_bar->setVisible(true);
	_ap_params_table->setRowCount(0);
	_parameters_manager->updateApParameters();
}

void ApParametersPage::_handleApParamsReceived(
		const std::unordered_map<std::string, mavlink_param_value_t> &ap_params) {
	_fc_params = ap_params;
	_fill_fc_params_columns();
	_compare_params_btn->setEnabled(true);
	if (_autopilot->getParamsUploadState() !=
			AutopilotParamsUploadState::Uploading) {
		_download_params_progress_bar->setVisible(false);
	}
}

void ApParametersPage::_handleApParametersTotalCountUpdate(
		size_t parameters_total_count) {
	_download_params_progress_bar->setMaximum(parameters_total_count);
}

void ApParametersPage::_handleApParametersCountUpdate(size_t parameters_count) {
	_download_params_progress_bar->setValue(parameters_count);
}

void ApParametersPage::_handleParamsCompareCompletion(
		const ParametersCompareResult &compare_result,
		const std::unordered_map<std::string, float> not_matched_parameters) {
	if (_state != State::ParametersComparing) {
		return;
	}
	_state = State::None;
	_ap_params_table->setRowCount(0);
	_fill_fc_params_columns();

	switch (compare_result) {
	case ParametersCompareResult::Matching: {
		QMessageBox::information(this, tr("Informataion"),
														 tr("Parameters are matching"));
	} break;
	case ParametersCompareResult::NotMatching: {
		qDebug() << "NOT MATCHED PARAMETERS:" << not_matched_parameters.size()
						 << "==========================";
		auto not_matched_params = not_matched_parameters;
		for (int row_index = 0; row_index < _ap_params_table->rowCount();
				 row_index++) {
			const auto widgetItem =
					_ap_params_table->item(row_index, ap_params_table_column_count - 3);
			for (const auto &[param_name, param_value] : not_matched_params) {
				const auto param_name_str = QString::fromUtf8(param_name.data(), 16);
				const auto param_name_formatted =
						param_name_str.left(param_name_str.indexOf(QChar('\0')));
				if (widgetItem != nullptr &&
						widgetItem->text() == param_name_formatted) {
					const auto comparingWidgetItem = new QTableWidgetItem(
							QString::number(param_value), QTableWidgetItem::Type);
					_ap_params_table->item(row_index, ap_params_table_column_count - 2)
							->setBackground(QBrush(Qt::red));
					_ap_params_table->setItem(row_index, ap_params_table_column_count - 1,
																		comparingWidgetItem);
					not_matched_params.erase(param_name);
					break;
				}
			}
		}

		if (!not_matched_params.empty()) {
			auto item_index = _ap_params_table->rowCount();
			_ap_params_table->setRowCount(_ap_params_table->rowCount() +
																		not_matched_params.size());
			for (const auto &[not_found_param_id, not_found_param_value] :
					 not_matched_params) {
				const auto param_name =
						QString::fromUtf8(not_found_param_id.data(), 16);
				const auto param_name_formatted =
						param_name.left(param_name.indexOf(QChar('\0')));
				const auto &param_id_item =
						new QTableWidgetItem(param_name_formatted, QTableWidgetItem::Type);
				param_id_item->setBackground(QBrush(Qt::transparent));
				_ap_params_table->setItem(item_index, ap_params_table_column_count - 3,
																	param_id_item);

				const auto &ap_param_value_item =
						new QTableWidgetItem(tr("Not found"), QTableWidgetItem::Type);
				ap_param_value_item->setBackground(QBrush(Qt::red));
				_ap_params_table->setItem(item_index, ap_params_table_column_count - 2,
																	ap_param_value_item);

				const auto &not_found_param_item = new QTableWidgetItem(
						QString::number(not_found_param_value), QTableWidgetItem::Type);
				not_found_param_item->setBackground(QBrush(Qt::transparent));
				_ap_params_table->setItem(item_index, ap_params_table_column_count - 1,
																	not_found_param_item);
				item_index++;
			}
		}

		QMessageBox::warning(this, tr("Warning"),
												 tr("Parameters are not matching"));
		_upload_params_btn->setEnabled(true);
	} break;
	}
}

void ApParametersPage::_handleParamsUpload(
		const ParametersUploadResult &upload_result,
		std::unordered_map<std::string, float> not_saved_parameters) {
	if (_state != State::ParametersUploading) {
		return;
	}
	_ap_params_table->setRowCount(0);
	_fill_fc_params_columns();

	switch (upload_result) {
	case ParametersUploadResult::Ok: {
		QMessageBox::information(this, tr("Information"), tr("Parameters saved"));
	} break;
	case ParametersUploadResult::NotAllSaved: {
		auto not_saved_params = not_saved_parameters;
		for (int row_index = 0; row_index < _ap_params_table->rowCount();
				 row_index++) {
			const auto widgetItem =
					_ap_params_table->item(row_index, ap_params_table_column_count - 3);
			for (const auto &[param_name, param_value] : not_saved_params) {
				const auto param_name_str = QString::fromUtf8(param_name.data(), 16);
				const auto param_name_formatted =
						param_name_str.left(param_name_str.indexOf(QChar('\0')));
				if (widgetItem != nullptr &&
						widgetItem->text() == param_name_formatted) {
					const auto comparingWidgetItem = new QTableWidgetItem(
							QString::number(param_value), QTableWidgetItem::Type);
					_ap_params_table->item(row_index, ap_params_table_column_count - 2)
							->setBackground(QBrush(Qt::red));
					_ap_params_table->setItem(row_index, ap_params_table_column_count - 1,
																		comparingWidgetItem);
					not_saved_params.erase(param_name);
					break;
				}
			}
		}

		if (!not_saved_params.empty()) {
			auto item_index = _ap_params_table->rowCount();
			_ap_params_table->setRowCount(_ap_params_table->rowCount() +
																		not_saved_params.size());
			for (const auto &[not_found_param_id, not_found_param_value] :
					 not_saved_params) {
				const auto param_name =
						QString::fromUtf8(not_found_param_id.data(), 16);
				const auto param_name_formatted =
						param_name.left(param_name.indexOf(QChar('\0')));
				const auto &param_id_item =
						new QTableWidgetItem(param_name_formatted, QTableWidgetItem::Type);
				param_id_item->setBackground(QBrush(Qt::transparent));
				_ap_params_table->setItem(item_index, ap_params_table_column_count - 3,
																	param_id_item);

				const auto &ap_param_value_item =
						new QTableWidgetItem(tr("Not found"), QTableWidgetItem::Type);
				ap_param_value_item->setBackground(QBrush(Qt::red));
				_ap_params_table->setItem(item_index, ap_params_table_column_count - 2,
																	ap_param_value_item);

				const auto &not_found_param_item = new QTableWidgetItem(
						QString::number(not_found_param_value), QTableWidgetItem::Type);
				not_found_param_item->setBackground(QBrush(Qt::transparent));
				_ap_params_table->setItem(item_index, ap_params_table_column_count - 1,
																	not_found_param_item);
				item_index++;
			}
		}

		QMessageBox::warning(this, tr("Warning"), tr("Not all parameters saved"));
	} break;
	}

	_compare_params_btn->setEnabled(true);
	_download_params_progress_bar->setVisible(false);
	_upload_params_progress_wrapper->setVisible(false);
}

void ApParametersPage::_handleParamsUploadAttemptUpdate(uint8_t attempt) {
	if (_state != State::ParametersUploading) {
		return;
	}
	_upload_params_progress_bar->setValue(attempt);
}

void ApParametersPage::_handleUpdateParamsButtonClick() {
	updateApParameters();
}

void ApParametersPage::_handleCompareParamsButtonClick() {
	const auto params_file_name = QFileDialog::getOpenFileName(
			this, tr("Choose parameters file"), "", "*.param");
	if (!params_file_name.isEmpty()) {
		auto params_file = QFile(params_file_name);
		if (!params_file.open(QIODevice::ReadOnly)) {
			QMessageBox::warning(this, tr("Warning"), tr("Failed to open file"));
			return;
		}
		const auto file_content = params_file.readAll();
		params_file.close();

		_update_params_btn->setEnabled(false);
		_compare_params_btn->setEnabled(false);
		_upload_params_btn->setEnabled(false);
		_file_name_label->setText(
				tr("Comparing file: %1")
						.arg(params_file_name.mid(params_file_name.lastIndexOf(QChar('/')) +
																			1)));
		_file_name_label->setVisible(true);
		_reset_state();
		_compare_params_btn->setEnabled(true);

		_state = State::ParametersComparing;
		_parameters_manager->handleCompareParamsRequest(file_content);
	}
}

void ApParametersPage::_handleUploadParamsButtonClick() {
	// _uploadParameters();
	_state = State::ParametersUploading;
	_compare_params_btn->setEnabled(false);
	_upload_params_btn->setEnabled(false);
	_download_params_progress_bar->setValue(0);
	_download_params_progress_bar->setVisible(true);
	_upload_params_progress_bar->setValue(0);
	_upload_params_progress_wrapper->setVisible(true);
	_parameters_manager->handleUploadParamsRequest();
}

void ApParametersPage::_handleResetParamsButtonClick() {
	QMessageBox::information(
			this, tr("Information"),
			tr("Parameters have been reset. Autopilot will be rebooted"));
	_fc_params.clear();
	_ap_params_table->setRowCount(0);
	_compare_params_btn->setEnabled(false);
	_upload_params_btn->setEnabled(false);
	_parameters_manager->handleResetParamsRequest();
}

void ApParametersPage::_fill_fc_params_columns() {
	_ap_params_table->setRowCount(_fc_params.size());
	for (const auto &[param_name, param] : _fc_params) {
		const auto param_name_str = QString::fromUtf8(param.param_id, 16);
		const auto param_name_formatted =
				param_name_str.left(param_name_str.indexOf(QChar('\0')));
		const auto &table_item_name =
				new QTableWidgetItem(param_name_formatted, QTableWidgetItem::Type);
		const auto &table_item_value = new QTableWidgetItem(
				QString::number(param.param_value), QTableWidgetItem::Type);
		_ap_params_table->setItem(
				param.param_index, ap_params_table_column_count - 3, table_item_name);
		_ap_params_table->setItem(
				param.param_index, ap_params_table_column_count - 2, table_item_value);
	}
}

void ApParametersPage::_reset_state() {
	// _fc_params.clear();
	_autopilot->setParamsSendState(AutopilotParamsUploadState::None);
	_autopilot->setParamsState(AutopilotParamsState::None);
	_update_params_btn->setEnabled(true);
	_compare_params_btn->setEnabled(false);
	_upload_params_btn->setEnabled(false);
	_upload_params_progress_bar->setValue(0);
	_upload_params_progress_wrapper->setVisible(false);
}
