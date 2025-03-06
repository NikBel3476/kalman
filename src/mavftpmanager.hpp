#ifndef MAVFTPMANAGER_HPP
#define MAVFTPMANAGER_HPP

#include <QObject>

#include <ardupilotmega/mavlink.h>
#include <mavlink_types.h>
#include <string>
#include <iostream>

#include "autopilot.hpp"
#include "mavlinkmanager.hpp"
#include "crc.hpp"

enum class FtpUploadResult {
	Ok,
	RemoveFilesError,
	CreateDirectoryError,
	CreateFileError,
	WriteFileError,
	CrcVerificationError
};

class MavFtpManager : public QObject {
	Q_OBJECT
public:
	explicit MavFtpManager(QWidget *parent, MavlinkManager *mavlink_manager,
											Autopilot *autopilot);

	std::string current_ap_ftp_path;

signals:
	void ftpUploadCompleted(const FtpUploadResult &);
	void ftpFileUploaded(uint8_t files_to_upload_remaining_count);

public slots:
	void uploadFiles(const std::unordered_map<QString, QByteArray> &files_to_upload);

private slots:
	void _handleAutopilotStateUpdate(const AutopilotState &);
	void _handleFtpRemoveFileTimeout();
	void _handleFtpWriteFileTimeout();

private:
	void _handleMavlinkMessageReceive(const mavlink_message_t &);
	void _handleFtpMessage(const FtpMessage &);
	void _handleFtpAck(const FtpMessage &);
	void _handleFtpNack(const FtpMessage &);
	void _resetState();

	MavlinkManager *_mavlink_manager = nullptr;
	Autopilot *_autopilot = nullptr;
	std::unique_ptr<QTimer> _ftp_remove_file_timer = nullptr;
	std::unique_ptr<QTimer> _ftp_write_file_timer = nullptr;

	std::vector<QString> _fs_item_list;
	/// key - file name | value - file content
	std::unordered_map<QString, QByteArray> _files_to_upload;
	/// first - file name | second - file content
	std::pair<QString, std::string>_uploading_file;
	size_t _uploading_chunk_index = 0;
	uint8_t _file_upload_session = 0;
	std::pair<QString, std::string> _file_to_crc_check;
	bool _files_upload_success = false;
};

#endif // MAVFTPMANAGER_HPP
