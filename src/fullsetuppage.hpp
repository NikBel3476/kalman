#ifndef FULLSETUPPAGE_HPP
#define FULLSETUPPAGE_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent>

#include <memory.h>
#include <untar/untar.h>
#include <ardupilotmega/mavlink.h>
#include <filesystem>

#include "firmwareuploader.hpp"
#include "parametersmanager.hpp"
#include "mavftpmanager.hpp"
#include "autopilot.hpp"

enum class FullSetupResult {
	Success,
	FcFirmwareNotFound,
	MultipleFcFirmwareFilesFound,
	ParamFileNotFound,
	MultipleParamFilesFound,
	LuaFilesNotFound,
	FileReadError,
	FirmwareUploadError,
	ParametersUploadError,
	LuaFilesUploadError
};

class FullSetupPage : public QWidget {
	Q_OBJECT
public:
	explicit FullSetupPage(QWidget *parent, Autopilot *autopilot, ParametersManager *parameters_manager, MavFtpManager *mav_ftp_manager);
	void handleAutopilotConnection();

signals:
	void firmwareUploadStarted();
	void uploadFirmwareSuccsessfullyCompleted();
	void requestResetParams();
	void requestGetAllParams();
	void requestCompareParams(const QByteArray &params_file_content);
	void requestUploadParams();
	void requestUploadLuaScripts(const std::unordered_map<std::string, QByteArray> &scripts);
	void fullSetupCompleted(const FullSetupResult &);

private slots:
	void _startFullSetup();
	void _handleAllParamsReceive(const std::unordered_map<std::string, mavlink_param_value_t> &ap_params);
	void _handleParamsCompareComplete(const ParametersCompareResult &);
	void _handleParamsUploadComplete(const ParametersUploadResult &);
	void _handleFtpUploadCompletion(const FtpUploadResult &);
	// void _handleFirmwareUploadStateUpdate(const FirmwareUploadState &new_state);

private:
	enum class State {
		None,
		FirmwareUpload,
		// ParametersReset,
		ParametersUpload,
		LuaScriptsUpload
	};

	void _tryFullSetup(const std::filesystem::path &archive_file_path);
	void _tryUploadFirmware();
	void _tryUploadLuaScripts();

	QVBoxLayout *_layout = nullptr;
	QPushButton *_start_full_setup_button = nullptr;
	QWidget *_labels_container = nullptr;
	QVBoxLayout *_labels_container_layout = nullptr;
	QLabel *_notification_label = nullptr;
	QLabel *_firmware_upload_label = nullptr;
	QLabel *_parameters_upload_label = nullptr;
	QLabel *_lua_scripts_upload_label = nullptr;

	Autopilot *_autopilot = nullptr;
	ParametersManager *_parameters_manager = nullptr;
	MavFtpManager *_mav_ftp_manager = nullptr;
	std::unique_ptr<untar::tarFile> _archive_file = nullptr;
	std::string _fc_firmware_file_path;
	std::string _params_file_path;
	std::vector<std::string> _lua_files_paths;
	// QByteArray _firmware_file_content;
	// QByteArray _params_file_content;
	// std::unordered_map<QString, QByteArray> _lua_files;
	State _state = State::None;
};

#endif // FULLSETUPPAGE_HPP
