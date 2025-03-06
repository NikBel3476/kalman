#include "fullsetuppage.hpp"

static constexpr int PAGE_MIN_WIDTH = 150;
static constexpr int PAGE_MAX_WIDTH = 250;
static constexpr auto kApParamsUpdateDelay = std::chrono::seconds{10};

FullSetupPage::FullSetupPage(QWidget *parent, Autopilot *autopilot,
														 ParametersManager *parameters_manager,
														 MavFtpManager *mav_ftp_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_start_full_setup_button(new QPushButton(this)),
			_labels_container(new QWidget(this)),
			_labels_container_layout(new QVBoxLayout(this)),
			_notification_label(new QLabel(_labels_container)),
			_firmware_upload_label(new QLabel(_labels_container)),
			_parameters_upload_label(new QLabel(_labels_container)),
			_lua_scripts_upload_label(new QLabel(_labels_container)),
			_autopilot(autopilot),
			_parameters_manager(parameters_manager),
			_mav_ftp_manager(mav_ftp_manager) {
	_layout->setAlignment(Qt::AlignCenter);
	_layout->addWidget(_start_full_setup_button);
	_layout->addWidget(_labels_container);

	_start_full_setup_button->setText(tr("Open archive"));
	_start_full_setup_button->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_start_full_setup_button->setMinimumWidth(PAGE_MIN_WIDTH);
	_start_full_setup_button->setMaximumWidth(PAGE_MAX_WIDTH);

	_labels_container->setLayout(_labels_container_layout);
	_start_full_setup_button->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_labels_container->setMinimumWidth(PAGE_MIN_WIDTH);
	_labels_container->setMaximumWidth(PAGE_MAX_WIDTH);
	_labels_container->setVisible(false);

	_labels_container_layout->addWidget(_notification_label);
	_labels_container_layout->addWidget(_firmware_upload_label);
	_labels_container_layout->addWidget(_parameters_upload_label);
	_labels_container_layout->addWidget(_lua_scripts_upload_label);

	_notification_label->setText(tr("Wait for setup completion"));
	_notification_label->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_notification_label->setMinimumWidth(PAGE_MIN_WIDTH);
	_notification_label->setMaximumWidth(PAGE_MAX_WIDTH);
	_notification_label->setAlignment(Qt::AlignHCenter);

	_firmware_upload_label->setText(tr("Firmware uploading..."));
	_firmware_upload_label->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_firmware_upload_label->setMinimumWidth(PAGE_MIN_WIDTH);
	_firmware_upload_label->setMaximumWidth(PAGE_MAX_WIDTH);
	_firmware_upload_label->setAlignment(Qt::AlignHCenter);

	_parameters_upload_label->setText(tr("Parameters uploading..."));
	_parameters_upload_label->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_parameters_upload_label->setMinimumWidth(PAGE_MIN_WIDTH);
	_parameters_upload_label->setMaximumWidth(PAGE_MAX_WIDTH);
	_parameters_upload_label->setAlignment(Qt::AlignHCenter);

	_lua_scripts_upload_label->setText(tr("Lua files uploading..."));
	_lua_scripts_upload_label->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_lua_scripts_upload_label->setMinimumWidth(PAGE_MIN_WIDTH);
	_lua_scripts_upload_label->setMaximumWidth(PAGE_MAX_WIDTH);
	_lua_scripts_upload_label->setAlignment(Qt::AlignHCenter);

	// connections
	connect(_start_full_setup_button, &QPushButton::clicked, this,
					&FullSetupPage::_startFullSetup);
	connect(this, &FullSetupPage::fullSetupCompleted, this,
					[this](const FullSetupResult &result) {
						Q_UNUSED(result);
						_start_full_setup_button->setVisible(true);
						_labels_container->setVisible(false);
						_archive_file.reset();
					});

	// parameters manager connections
	// connect(this, &FullSetupPage::requestResetParams,
	// 				_parameters_manager, &ParametersManager::handleResetParamsRequest);
	connect(this, &FullSetupPage::requestGetAllParams, _parameters_manager,
					&ParametersManager::updateApParameters);
	connect(_parameters_manager, &ParametersManager::allParamsReceived, this,
					&FullSetupPage::_handleAllParamsReceive);
	connect(this, &FullSetupPage::requestCompareParams, _parameters_manager,
					&ParametersManager::handleCompareParamsRequest);
	connect(_parameters_manager, &ParametersManager::paramsCompareCompleted, this,
					&FullSetupPage::_handleParamsCompareComplete);
	connect(this, &FullSetupPage::requestUploadParams, _parameters_manager,
					&ParametersManager::handleUploadParamsRequest);
	connect(_parameters_manager, &ParametersManager::paramsUploaded, this,
					&FullSetupPage::_handleParamsUploadComplete);

	// mavftp manager connections
	connect(_mav_ftp_manager, &MavFtpManager::ftpUploadCompleted, this,
					&FullSetupPage::_handleFtpUploadCompletion);
}

void FullSetupPage::handleAutopilotConnection() {
	switch (_state) {
	case State::FirmwareUpload: {
		QTimer::singleShot(kApParamsUpdateDelay, Qt::PreciseTimer, this, [this]() {
			// _state = State::ParametersReset;
			// qDebug() << "RESET PARAMS REQUEST";
			// emit requestResetParams();

			_state = State::ParametersUpload;
			// qDebug() << "GET ALL PARAMS REQUEST";
			emit requestGetAllParams();
		});
	} break;
	// case State::ParametersReset: {
	// 	QTimer::singleShot(kApParamsUpdateDelay, Qt::PreciseTimer, this, [this](){
	// 		_state = State::ParametersUpload;
	// 		// qDebug() << "GET ALL PARAMS REQUEST";
	// 		emit requestGetAllParams();
	// 	});
	// } break;
	case State::ParametersUpload: {

	} break;
	case State::None:
	case State::LuaScriptsUpload:
		break;
	}
}

void FullSetupPage::_startFullSetup() {
	const auto tar_file_path = QFileDialog::getOpenFileName(
			nullptr, tr("Select tar archive"), "", tr("Tar archive (*.tar)"));
	if (!tar_file_path.isEmpty()) {
		// TODO: full setup
		qDebug() << tar_file_path;

		_start_full_setup_button->setVisible(false);
		_labels_container->setVisible(true);
		_firmware_upload_label->setStyleSheet(
				"QLabel { background-color: transparent; }");
		_parameters_upload_label->setStyleSheet(
				"QLabel { background-color: transparent; }");
		_lua_scripts_upload_label->setStyleSheet(
				"QLabel { background-color: transparent; }");
		_fc_firmware_file_path.clear();
		_params_file_path.clear();
		_lua_files_paths.clear();
		_state = State::None;

		const auto file_path =
				std::filesystem::path(tar_file_path.toStdU32String());
		_tryFullSetup(file_path);
	}
}

void FullSetupPage::_handleAllParamsReceive(
		const std::unordered_map<std::string, mavlink_param_value_t> &ap_params) {
	Q_UNUSED(ap_params);
	if (_state != State::ParametersUpload ||
			_autopilot->getParamsUploadState() ==
					AutopilotParamsUploadState::Uploading) {
		return;
	}
	auto *const params_file_entry = _archive_file->find(_params_file_path);
	size_t params_file_start = 0;
	int params_file_size = 0;
	auto *const params_file_stream =
			params_file_entry->wantToExtract(&params_file_size, &params_file_start);

	QByteArray params_file_content(params_file_size, '\0');

	if (params_file_stream->is_open()) {
		params_file_stream->seekg(params_file_entry->getStartingByte(),
															std::ios_base::beg);
		params_file_stream->read(params_file_content.data(), params_file_size);
	}

	if (params_file_content.startsWith('\0')) {
		_state = State::None;
		QMessageBox::warning(nullptr, tr("Warning"),
												 tr("Failed to read parameters file"));
		return;
	}

	// qDebug() << "COMPARE PARAMS REQUEST ==============================";
	emit requestCompareParams(params_file_content);
}

void FullSetupPage::_handleParamsCompareComplete(
		const ParametersCompareResult &compare_result) {
	if (_state == State::None) {
		return;
	}
	switch (compare_result) {
	case ParametersCompareResult::Matching: {
		_parameters_upload_label->setStyleSheet(
				"QLabel { background-color: green; }");
		_state = State::LuaScriptsUpload;
		_tryUploadLuaScripts();
	} break;
	case ParametersCompareResult::NotMatching: {
		// qDebug() << "UPLOAD PARAMS REQUEST =============================";
		emit requestUploadParams();
	} break;
	}
}

void FullSetupPage::_handleParamsUploadComplete(
		const ParametersUploadResult &upload_result) {
	if (_state == State::None) {
		return;
	}
	switch (upload_result) {
	case ParametersUploadResult::Ok: {
		_parameters_upload_label->setStyleSheet(
				"QLabel { background-color: green; }");
		_state = State::LuaScriptsUpload;
		// qDebug() << "START LUA SCRIPT UPLOAD =================";
		_tryUploadLuaScripts();
	} break;
	case ParametersUploadResult::NotAllSaved: {
		_state = State::None;
		emit fullSetupCompleted(FullSetupResult::ParametersUploadError);
		QMessageBox::warning(nullptr, tr("Warning"),
												 tr("Not all parameters saved"));
	} break;
	}
}

void FullSetupPage::_handleFtpUploadCompletion(const FtpUploadResult &result) {
	if (_state == State::None) {
		return;
	}
	switch (result) {
	case FtpUploadResult::Ok: {
		_lua_scripts_upload_label->setStyleSheet(
				"QLabel { background-color: green; }");
		_state = State::None;
		emit fullSetupCompleted(FullSetupResult::Success);
	} break;
	case FtpUploadResult::RemoveFilesError: {
		QMessageBox::warning(this, tr("Warning"),
												 tr("Failed to remove scripts in autopilot"));
	} break;
	case FtpUploadResult::CreateDirectoryError: {
		QMessageBox::warning(this, tr("Warning"),
												 tr("Failed to create scripts directory"));
	} break;
	case FtpUploadResult::CreateFileError: {
		QMessageBox::warning(this, tr("Warning"), tr("Failed to create file"));
	} break;
	case FtpUploadResult::WriteFileError: {
		QMessageBox::warning(this, tr("Warning"), tr("Failed to write file"));
	} break;
	case FtpUploadResult::CrcVerificationError: {
		QMessageBox::warning(this, tr("Warning"), tr("File CRC mismatch"));
	} break;
	}

	if (result != FtpUploadResult::Ok) {
		emit fullSetupCompleted(FullSetupResult::LuaFilesUploadError);
		_state = State::None;
		return;
	}
}

void FullSetupPage::_tryFullSetup(
		const std::filesystem::path &archive_file_path) {
	auto tar_file_path = archive_file_path;
	_archive_file = std::make_unique<untar::tarFile>(tar_file_path, untar::File);
	// const auto archive_file =
	// std::make_unique<untar::tarFile>(tar_file_path.data(), untar::File);

	for (const auto &[file_name, entry] : _archive_file->entries) {
		qDebug() << file_name;
	}

	for (const auto &[file_name, entry] : _archive_file->entries) {
		if (file_name.ends_with(".apj")) {
			if (_fc_firmware_file_path.empty()) {
				_fc_firmware_file_path = file_name;
			} else {
				emit fullSetupCompleted(FullSetupResult::MultipleFcFirmwareFilesFound);
				return;
			}
		} else if (file_name.ends_with(".param")) {
			if (_params_file_path.empty()) {
				_params_file_path = file_name;
			} else {
				emit fullSetupCompleted(FullSetupResult::MultipleParamFilesFound);
				return;
			}
		} else if (file_name.ends_with(".lua")) {
			_lua_files_paths.push_back(file_name);
		}
	}

	if (_fc_firmware_file_path.empty()) {
		emit fullSetupCompleted(FullSetupResult::FcFirmwareNotFound);
		return;
	}
	if (_params_file_path.empty()) {
		emit fullSetupCompleted(FullSetupResult::ParamFileNotFound);
		return;
	}
	if (_lua_files_paths.empty()) {
		emit fullSetupCompleted(FullSetupResult::LuaFilesNotFound);
		return;
	}

	// const auto firmware_file_entry =
	// archive_file->find(_fc_firmware_file_path); size_t firmware_file_start = 0;
	// int firmware_file_size = 0;
	// const auto firmware_file_stream =
	// firmware_file_entry->wantToExtract(&firmware_file_size,
	// &firmware_file_start);

	// // QByteArray firmware_file_content(firmware_file_size, '\0');

	// if (firmware_file_stream->is_open()) {
	// 	firmware_file_stream->seekg(firmware_file_entry->getStartingByte(),
	// std::ios_base::beg);
	// 	firmware_file_stream->read(_firmware_file_content.data(),
	// firmware_file_size);
	// }

	// if (_firmware_file_content.startsWith('\0')) {
	// 	QMessageBox::warning(nullptr, tr("Warning"), tr("Failed to read parameters
	// file")); 	emit fullSetupCompleted(FullSetupResult::FileReadError); return;
	// }

	// auto *const params_file_entry = archive_file->find(_params_file_path);
	// size_t params_file_start = 0;
	// int params_file_size = 0;
	// auto *const params_file_stream =
	// params_file_entry->wantToExtract(&params_file_size, &params_file_start);

	// // QByteArray params_file_content(params_file_size, '\0');

	// if (params_file_stream->is_open()) {
	// 	params_file_stream->seekg(params_file_entry->getStartingByte(),
	// std::ios_base::beg); params_file_stream->read(_params_file_content.data(),
	// params_file_size);
	// }

	// if (_params_file_content.startsWith('\0')) {
	// 	QMessageBox::warning(nullptr, tr("Warning"), tr("Failed to read parameters
	// file")); 	emit fullSetupCompleted(FullSetupResult::FileReadError); return;
	// }

	// for (const auto &lua_file_path: _lua_files_paths) {
	// 	const auto lua_file_entry = archive_file->find(lua_file_path);
	// 	size_t lua_file_start = 0;
	// 	int lua_file_size = 0;
	// 	const auto lua_file_stream = lua_file_entry->wantToExtract(&lua_file_size,
	// &lua_file_start);

	// 	QByteArray lua_file_content(lua_file_size, '\0');

	// 	if (lua_file_stream->is_open()) {
	// 		lua_file_stream->seekg(lua_file_entry->getStartingByte(),
	// std::ios_base::beg); 		lua_file_stream->read(lua_file_content.data(),
	// lua_file_size);
	// 	}

	// 	if (lua_file_content.startsWith('\0')) {
	// 		QMessageBox::warning(nullptr, tr("Warning"), tr("Failed to read lua
	// file")); 		emit fullSetupCompleted(FullSetupResult::FileReadError);
	// return;
	// 	}
	// 	_lua_files[QString::fromStdString(lua_file_path)] = lua_file_content;
	// }

	_state = State::FirmwareUpload;
	_tryUploadFirmware();
}

void FullSetupPage::_tryUploadFirmware() {
	if (_state == State::None) {
		return;
	}
	emit firmwareUploadStarted();

	// firmware upload
	const auto firmware_file_entry = _archive_file->find(_fc_firmware_file_path);
	size_t firmware_file_start = 0;
	int firmware_file_size = 0;
	const auto firmware_file_stream = firmware_file_entry->wantToExtract(
			&firmware_file_size, &firmware_file_start);

	QByteArray firmware_file_content(firmware_file_size, '\0');

	if (firmware_file_stream->is_open()) {
		firmware_file_stream->seekg(firmware_file_entry->getStartingByte(),
																std::ios_base::beg);
		firmware_file_stream->read(firmware_file_content.data(),
															 firmware_file_size);
	}

	if (firmware_file_content.startsWith('\0')) {
		emit fullSetupCompleted(FullSetupResult::FileReadError);
		return;
	}

	auto firmware_upload_task =
			QtConcurrent::task([firmware_file_content]() {
				const auto firmware_uploader = std::make_unique<FirmwareUploader>();
				return firmware_uploader->upload(firmware_file_content);
			}).spawn();

	firmware_upload_task.then(
			this, [this](const FirmwareUploadResult &firmware_upload_result) {
				switch (firmware_upload_result) {
				case FirmwareUploadResult::Ok: {

				} break;
				case FirmwareUploadResult::FirmwareImageNotFound: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Firmware image not found"));
				} break;
				case FirmwareUploadResult::FirmwareSizeNotFound: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Firmware size not found"));
				} break;
				case FirmwareUploadResult::BoardIdNotFound: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Board id not found"));
				} break;
				case FirmwareUploadResult::BootloaderNotFound: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Bootloader not found"));
				} break;
				case FirmwareUploadResult::TooLargeFirmware: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Too large firmware"));
				} break;
				case FirmwareUploadResult::DecodeFail: {
					QMessageBox::warning(nullptr, tr("Upload failed"), tr("Decode fail"));
				} break;
				case FirmwareUploadResult::EraseFail: {
					QMessageBox::warning(nullptr, tr("Upload failed"), tr("Erase fail"));
				} break;
				case FirmwareUploadResult::ProgramFail: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Program fail"));
				} break;
				case FirmwareUploadResult::VerificationFail: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Verification fail"));
				} break;
				case FirmwareUploadResult::UnsupportedBoard: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Unsupported board"));
				} break;
				case FirmwareUploadResult::UnsupportedBootloader: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Unsupported bootloader"));
				} break;
				case FirmwareUploadResult::IncompatibleBoardType: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Incompatible board type"));
				} break;
				case FirmwareUploadResult::SerialPortError: {
					QMessageBox::warning(nullptr, tr("Upload failed"),
															 tr("Serial port error"));
				} break;
				}

				if (firmware_upload_result != FirmwareUploadResult::Ok) {
					emit fullSetupCompleted(FullSetupResult::FirmwareUploadError);
					_state = State::None;
					return;
				}

				_firmware_upload_label->setStyleSheet(
						"QLabel { background-color: green; }");
				emit uploadFirmwareSuccsessfullyCompleted();
			});
}

void FullSetupPage::_tryUploadLuaScripts() {
	if (_state == State::None) {
		return;
	}
	std::unordered_map<QString, QByteArray> lua_files;
	for (const auto &lua_file_path : _lua_files_paths) {
		const auto lua_file_entry = _archive_file->find(lua_file_path);
		size_t lua_file_start = 0;
		int lua_file_size = 0;
		const auto lua_file_stream =
				lua_file_entry->wantToExtract(&lua_file_size, &lua_file_start);

		QByteArray lua_file_content(lua_file_size, '\0');

		if (lua_file_stream->is_open()) {
			lua_file_stream->seekg(lua_file_entry->getStartingByte(),
														 std::ios_base::beg);
			lua_file_stream->read(lua_file_content.data(), lua_file_size);
		}

		if (lua_file_content.startsWith('\0')) {
			emit fullSetupCompleted(FullSetupResult::FileReadError);
			return;
		}
		lua_files[QString::fromStdString(lua_file_path)] = lua_file_content;
	}

	_mav_ftp_manager->uploadFiles(lua_files);
}

// void FullSetupPage::_handleFirmwareUploadStateUpdate(const
// FirmwareUploadState &new_state) { 	switch (new_state) { 	case
// FirmwareUploadState::Rebooting: {
// 		_firmware_upload_status_label->setVisible(true);
// 		_firmware_upload_status_label->setText(tr("Rebooting..."));
// 	} break;
// 	case FirmwareUploadState::BootloaderSearching: {
// 		_firmware_upload_status_label->setVisible(true);
// 		_firmware_upload_status_label->setText(tr("Searching bootloader..."));
// 	} break;
// 	case FirmwareUploadState::Erasing: {
// 		_firmware_upload_status_label->setVisible(true);
// 		_firmware_upload_status_label->setText(tr("Erasing..."));
// 		_progress_bar->setVisible(true);
// 	} break;
// 	case FirmwareUploadState::Flashing: {
// 		_firmware_upload_status_label->setVisible(true);
// 		_firmware_upload_status_label->setText(tr("Flashing..."));
// 		_progress_bar->setVisible(true);
// 	} break;
// 	case FirmwareUploadState::None: {
// 		_progress_bar->setVisible(false);
// 	}
// 	}
// }
