#ifndef MAVFTPPPAGEH
#define MAVFTPPPAGEH

#include <QFileDialog>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
#include <ardupilotmega/mavlink.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mavlink_types.h>
#include <string>

#include "autopilot.hpp"
#include "mavftpmanager.hpp"

class MavftpPage : public QWidget {
	Q_OBJECT
public:
	explicit MavftpPage(QWidget *parent, Autopilot *autopilot,
											MavFtpManager *_mav_ftp_manager);

signals:

private slots:
	void _handleUploadLuaButtonClick();
	void _handleAutopilotStateUpdate(const AutopilotState &);
	void _handleFtpUploadCompletion(const FtpUploadResult &);
	void _handleFtpFileUpload(uint8_t files_to_upload_remaining_count);

private:
	enum class State {
		None,
		Uploading
	};
	void _resetState();

	QVBoxLayout *_layout = nullptr;
	QPushButton *_upload_lua_button = nullptr;
	QLabel *_upload_label = nullptr;
	QProgressBar *_upload_progress_bar = nullptr;

	Autopilot *_autopilot = nullptr;
	MavFtpManager *_mav_ftp_manager = nullptr;
	State _state = State::None;
};

#endif // MAVFTPPAGE_H
