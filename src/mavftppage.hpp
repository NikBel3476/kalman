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
#include <algorithm>
#include <ardupilotmega/mavlink.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mavlink_types.h>
#include <stack>
#include <string>

#include "autopilot.hpp"
#include "crc.hpp"
#include "mavlinkmanager.hpp"

class MavftpPage : public QWidget {
	Q_OBJECT
public:
	explicit MavftpPage(QWidget *parent, MavlinkManager *mavlink_manager,
											Autopilot *autopilot);

	std::string current_ap_ftp_path;

signals:

private slots:
	void _handleUploadLuaButtonClick();
	void _handleAutopilotStateUpdate(const AutopilotState &);
	void _handleFtpRemoveFileTimeout();
	void _handleFtpWriteFileTimeout();

private:
	void _handleMavlinkMessageReceive(const mavlink_message_t &);
	void _handleFtpMessage(const FtpMessage &);
	void _handleFtpAck(const FtpMessage &);
	void _handleFtpNack(const FtpMessage &);
	void _resetState();

	QVBoxLayout *_layout = nullptr;
	QPushButton *_upload_lua_button = nullptr;
	QLabel *_upload_label = nullptr;
	QProgressBar *_upload_progress_bar = nullptr;

	MavlinkManager *_mavlink_manager = nullptr;
	Autopilot *_autopilot = nullptr;
	std::unique_ptr<QTimer> _ftp_remove_file_timer = nullptr;
	std::unique_ptr<QTimer> _ftp_write_file_timer = nullptr;

	std::vector<QString> _fs_item_list;
	std::unordered_map<QString, QString>
			_files_to_upload; // key - file name | value - full file path on host
												// system
	std::pair<QString, std::string>
			_uploading_file; // first - file name | second - file content
	size_t _uploading_chunk_index = 0;
	uint8_t _file_upload_session = 0;
	std::pair<QString, std::string> _file_to_crc_check;
	bool _files_upload_success = false;
};

#endif // MAVFTPPAGE_H
