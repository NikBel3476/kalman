#include "mavftppage.hpp"

const int MIN_PAGE_WIDTH = 150;
const int MAX_PAGE_WIDTH = 250;

MavftpPage::MavftpPage(QWidget *parent, Autopilot *autopilot,
											 MavFtpManager *mav_ftp_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_upload_lua_button(new QPushButton()),
			_upload_label(new QLabel()),
			_upload_progress_bar(new QProgressBar()),
			_autopilot{autopilot},
			_mav_ftp_manager(mav_ftp_manager) {
	_layout->setAlignment(Qt::AlignCenter);
	_layout->addWidget(_upload_lua_button);
	_layout->addWidget(_upload_label);
	_layout->addWidget(_upload_progress_bar);

	_upload_lua_button->setText(tr("Upload lua scripts"));
	_upload_lua_button->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	_upload_lua_button->setMinimumWidth(MIN_PAGE_WIDTH);
	_upload_lua_button->setMaximumWidth(MAX_PAGE_WIDTH);

	_upload_label->setVisible(false);
	_upload_label->setText(tr("Uploading..."));
	_upload_label->setAlignment(Qt::AlignCenter);

	_upload_progress_bar->setVisible(false);

	// connections
	connect(_upload_lua_button, &QPushButton::clicked, this,
					&MavftpPage::_handleUploadLuaButtonClick);

	// autopilot connections
	connect(_autopilot, &Autopilot::stateUpdated, this,
					&MavftpPage::_handleAutopilotStateUpdate);

	// mav ftp manager connections
	connect(_mav_ftp_manager, &MavFtpManager::ftpUploadCompleted, this,
					&MavftpPage::_handleFtpUploadCompletion);
	connect(_mav_ftp_manager, &MavFtpManager::ftpFileUploaded, this,
					&MavftpPage::_handleFtpFileUpload);
}

void MavftpPage::_handleUploadLuaButtonClick() {
	_resetState();
	const auto file_path_list =
			QFileDialog::getOpenFileNames(this, tr("Select lua scripts"), "", "*.lua")
					.toVector();
	if (file_path_list.empty()) {
		return;
	}
	std::unordered_map<QString, QByteArray> files_to_upload;
	for (const auto &file_name : file_path_list) {
		const auto file_path = std::filesystem::path(file_name.toStdU32String());
		std::ifstream file_stream(file_path);
		if (file_stream.is_open()) {
			const auto file_size = std::filesystem::file_size(file_path);
			QByteArray file_content(file_size, '\0');

			file_stream.read(file_content.data(), file_size);
			if (file_content.startsWith('\0')) {
				QMessageBox::warning(nullptr, tr("Warning"), tr("Failed to read file"));
				_resetState();
				return;
			}

			files_to_upload[file_name.mid(file_name.lastIndexOf(QChar('/')) + 1)] =
					file_content;
		} else {
			QMessageBox::warning(this, tr("Warning"), tr("Failed to open file"));
			_resetState();
			return;
		}
	}

	_state = State::Uploading;
	_upload_label->setVisible(true);
	_upload_lua_button->setVisible(false);
	_upload_progress_bar->setValue(0);
	_upload_progress_bar->setMaximum(static_cast<int>(files_to_upload.size()));
	_upload_progress_bar->setVisible(true);
	_mav_ftp_manager->uploadFiles(files_to_upload);
}

void MavftpPage::_handleAutopilotStateUpdate(const AutopilotState &state) {
	switch (state) {
	case AutopilotState::None: {
		_resetState();
	} break;
	case AutopilotState::Alive:
	case AutopilotState::Flashing:
		break;
	}
}

void MavftpPage::_handleFtpUploadCompletion(const FtpUploadResult &result) {
	if (_state == State::None) {
		return;
	}
	switch (result) {
	case FtpUploadResult::Ok: {
		QMessageBox::information(this, tr("Information"), tr("Files uploaded"));
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
	_resetState();
}

void MavftpPage::_handleFtpFileUpload(uint8_t files_to_upload_remaining_count) {
	_upload_progress_bar->setValue(_upload_progress_bar->maximum() -
																 files_to_upload_remaining_count);
}

void MavftpPage::_resetState() {
	_state = State::None;
	_upload_lua_button->setVisible(true);
	_upload_label->setVisible(false);
	_upload_progress_bar->setValue(0);
	_upload_progress_bar->setVisible(false);
}
