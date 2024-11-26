#include "mavftppage.h"

static constexpr auto lua_scripts_ftp_directory = "/APM/scripts/";
static constexpr auto FTP_PAYLOAD_SIZE = 80;

MavftpPage::MavftpPage(QWidget *parent, MavlinkManager *mavlink_manager)
		: QWidget{parent},
			_layout(new QVBoxLayout(this)),
			_upload_lua_button(new QPushButton()),
			_tree_view(new QTreeView(this)),
			_mavlink_manager{mavlink_manager} {
	_layout->addWidget(_upload_lua_button);
	_layout->addWidget(_tree_view);

	_upload_lua_button->setText(tr("Upload lua scripts"));

	const auto tree_view_header = new QHeaderView(Qt::Horizontal);
	// const auto tree_view_header_view = new
	// tree_view_header->setModel(tree_view_header_model);

	_tree_view->setHeader(tree_view_header);
	// _tree_view->setModel();

	// _tree_view->setColumnCount(3);
	// _tree_view->setHeaderLabels({"Name", "Type", "Size"});
	// _tree_view->insertTopLevelItem(0, new
	// QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), {"/"}));

	// connections
	connect(_upload_lua_button, &QPushButton::clicked, this,
					&MavftpPage::_handleUploadLuaButtonClick);

	// mavlink manager connections
	connect(_mavlink_manager, &MavlinkManager::mavlinkMessageReceived, this,
					&MavftpPage::_handleMavlinkMessageReceive);
}

void MavftpPage::_handleUploadLuaButtonClick() {
	const auto file_path_list =
			QFileDialog::getOpenFileNames(this, tr("Select lua scripts"), "", "*.lua")
					.toVector();
	if (file_path_list.empty()) {
		return;
	}
	for (const auto &file_name : file_path_list) {
		_files_to_upload[file_name.mid(file_name.lastIndexOf('/') + 1)] = file_name;
	}

	current_ap_ftp_path = "/APM";
	_mavlink_manager->requestListDirectory(current_ap_ftp_path);
}

void MavftpPage::_handleMavlinkMessageReceive(
		const mavlink_message_t &mavlink_message) {
	switch (mavlink_message.msgid) {
	case MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL: {
		qDebug() << "FTP MESSAGE RECEIVED";
		mavlink_file_transfer_protocol_t ftp_message;
		mavlink_msg_file_transfer_protocol_decode(&mavlink_message, &ftp_message);

		FtpMessage message;
		memcpy(&message, ftp_message.payload, sizeof(FtpMessage));

		if (message.sequence != 0 &&
				message.sequence <= _mavlink_manager->ftp_message_sequence) {
			return;
		};
		_mavlink_manager->ftp_message_sequence = message.sequence;

		_handleFtpMessage(message);
	} break;
	}
}

void MavftpPage::_handleFtpMessage(const FtpMessage &message) {
	qDebug() << "PAYLOAD: " << std::string((char *)message.payload, 239)
					 << "\nOPCODE: " << static_cast<uint8_t>(message.opcode) << "\nOFFSET"
					 << message.offset << "\nSIZE" << message.size;

	switch (message.opcode) {
	case FtpMessage::Opcode::Ack: {
		qDebug() << "FTP ACK RECEIVED";
		_handleFtpAck(message);
	} break;
	case FtpMessage::Opcode::Nack: {
		qDebug() << "FTP NACK RECEIVED";
		_handleFtpNack(message);
	} break;
	}
}

void MavftpPage::_handleFtpAck(const FtpMessage &message) {
	switch (message.requestOpcode) {
	case FtpMessage::Opcode::ListDirectory: {
		qDebug() << "LIST DIRECTORY ACK";
		const auto payload_str = std::string((char *)message.payload, 239);
		for (const auto &str : QString::fromStdString(payload_str).split('\0')) {
			if (str.startsWith('D') || str.startsWith('F') || str.startsWith('S')) {
				_fs_item_list.push_back(str);
				qDebug() << str;
			}
		}

		_mavlink_manager->ftp_offset += _fs_item_list.size();
		_mavlink_manager->requestListDirectory(current_ap_ftp_path);
	} break;
	case FtpMessage::Opcode::CreateDirectory: {
		qDebug() << "CREATE DIRECTORY ACK";
		if (!_files_to_upload.empty()) {
			// _mavlink_manager->requestCreateFile((lua_scripts_ftp_directory +
			// _file_names_to_upload.top()).toStdString()); _await_ack_state =
			// AwaitAck::CreateFile;
			_mavlink_manager->requestResetSessions();
			return;
		}
	} break;
	case FtpMessage::Opcode::CreateFile: {
		qDebug() << "CREATE FILE ACK";
		_file_upload_session = message.session;
		if (!_files_to_upload.empty()) {
			std::ifstream file(_files_to_upload[_uploading_file.first].toStdString());
			if (!file.is_open()) {
				_mavlink_manager->requestResetSessions();
				return;
			}
			qDebug() << "UPLOADING FILE: " << _uploading_file.first;
			std::string file_content((std::istreambuf_iterator<char>(file)),
															 std::istreambuf_iterator<char>());
			std::vector<std::string> file_chunks;
			// for (size_t i = 0; i < file_content.length(); i += FTP_PAYLOAD_SIZE) {
			// 	file_chunks.push_back(file_content.substr(i, FTP_PAYLOAD_SIZE));
			// }
			_uploading_file.second = file_content;
			const auto chunk = _uploading_file.second.substr(
					_uploading_chunk_index * FTP_PAYLOAD_SIZE, FTP_PAYLOAD_SIZE);
			const std::vector data(chunk.begin(), chunk.end());
			qDebug() << "DATA: " << data;
			_mavlink_manager->requestWriteFile(data, _file_upload_session,
																				 _uploading_chunk_index *
																						 FTP_PAYLOAD_SIZE);
			_uploading_chunk_index++;
			return;
		}

		// _files_to_upload.erase(_files_to_upload.cend());
		// if (!_files_to_upload.empty()) {
		// 	_await_ack_state = AwaitAck::CreateFile;
		// 	_mavlink_manager->requestCreateFile((lua_scripts_ftp_directory +
		// (*_files_to_upload.cend()).first).toStdString()); 	return;
		// }
	} break;
	case FtpMessage::Opcode::WriteFile: {
		qDebug() << "WRITE FILE ACK";
		if (_uploading_chunk_index * FTP_PAYLOAD_SIZE <
				_uploading_file.second.size()) {
			const auto chunk = _uploading_file.second.substr(
					_uploading_chunk_index * FTP_PAYLOAD_SIZE, FTP_PAYLOAD_SIZE);
			const std::vector data(chunk.begin(), chunk.end());
			_mavlink_manager->requestWriteFile(data, _file_upload_session,
																				 _uploading_chunk_index *
																						 FTP_PAYLOAD_SIZE);
			_uploading_chunk_index++;
		} else {
			qDebug() << "FILE " << _uploading_file.first << " UPLOADED";
			_uploading_chunk_index = 0;
			_files_to_upload.erase(_uploading_file.first);
			_file_to_crc_check = _uploading_file;
			_uploading_file.first.clear();
			_uploading_file.second.clear();
			// if (_files_to_upload.empty()) {
			// 	_await_ack_state = AwaitAck::None;
			// } else {
			// 	_uploading_file.first = (*_files_to_upload.begin()).first;
			// 	_uploading_file.second.clear();
			// 	_await_ack_state = AwaitAck::CreateFile;
			// 	qDebug() << "UPLOADING FILE: " << _uploading_file.first;
			// }
			// _await_ack_state = _files_to_upload.empty() ? AwaitAck::None :
			// AwaitAck::CreateFile;
			_mavlink_manager->requestResetSessions();
		}
	} break;
	case FtpMessage::Opcode::ResetSessions: {
		qDebug() << "SESSION RESET ACK";
		qDebug() << "FILES TO UPLOAD SIZE: " << _files_to_upload.size();
		if (!_file_to_crc_check.first.isEmpty() &&
				!_file_to_crc_check.second.empty()) {
			const auto ap_file_path =
					lua_scripts_ftp_directory + _file_to_crc_check.first;
			_mavlink_manager->requestCalcFileCrc32(ap_file_path.toStdString());
			return;
		}
		if (!_files_to_upload.empty()) {
			_uploading_file.first = (*_files_to_upload.begin()).first;
			qDebug() << "UPLOADING FILE: " << _uploading_file.first;
			_mavlink_manager->requestCreateFile(
					(lua_scripts_ftp_directory + _uploading_file.first).toStdString());
			return;
		}
	} break;
	case FtpMessage::Opcode::CalcFileCRC32: {
		QByteArray crc((char *)message.payload, message.size);
		qDebug() << "CRC: " << crc;
		const auto expected_crc = Crc::crc(_file_to_crc_check.second.length(),
																			 QByteArray(_file_to_crc_check.second));
		qDebug() << "EXPECTED CRC: " << expected_crc;
		if (crc != expected_crc) {
			QMessageBox::warning(
					this, tr("Warning"),
					tr("Verification %1 failed").arg(_file_to_crc_check.first));
		}
		_file_to_crc_check.first.clear();
		_file_to_crc_check.second.clear();
		_mavlink_manager->requestResetSessions();
		if (_files_to_upload.empty()) {
			QMessageBox::information(this, tr("Information"), tr("Files uploaded"));
		}
	} break;
		// case AwaitAck::None:
		// 	break;
	}
}

void MavftpPage::_handleFtpNack(const FtpMessage &message) {
	switch (message.requestOpcode) {
	case FtpMessage::Opcode::ListDirectory: {
		qDebug() << "LIST DIRECTORY NACK";
		_mavlink_manager->ftp_offset = 0;
		if (std::find(_fs_item_list.cbegin(), _fs_item_list.cend(), "Dscripts") ==
				_fs_item_list.cend()) {
			_mavlink_manager->requestCreateDirectory(lua_scripts_ftp_directory);
			return;
		}
		if (!_files_to_upload.empty()) {
			// _await_ack_state = AwaitAck::CreateFile;
			// _mavlink_manager->requestCreateFile((lua_scripts_ftp_directory +
			// _file_names_to_upload.top()).toStdString());
			_mavlink_manager->requestResetSessions();
		}
	} break;
	case FtpMessage::Opcode::CreateDirectory: {
		qDebug() << "CREATE DIRECTORY NACK";
		QMessageBox::warning(this, tr("Warning"),
												 tr("Failed to create scripts directory"));
	} break;
	case FtpMessage::Opcode::CreateFile: {
		qDebug() << "CREATE FILE NACK";
		_mavlink_manager->requestTerminateSession();
		const auto file_name = _uploading_file.first;
		_files_to_upload.clear();
		QMessageBox::warning(this, tr("Warning"),
												 tr("Failed to create %1 file").arg(file_name));
	} break;
	}
}

// namespace FileTransferProtocol
// {
// 		class Worker : public QObject
// 		{
// 				Q_OBJECT

// 		public:
// 				explicit Worker(UUav *uav, QObject *parent = nullptr);

// 		public slots:
// 				void init(uint8_t uavID, uint8_t gcsID, uint8_t chan);
// 				void handleMessage(const mavlink_message_t &mavlinkMessage);

// 				void requestTerminateSession();
// 				void requestListDirectory(const QString &path);
// 				void requestOpenFileRO(const QString &source);
// 				void requestBurstReadFile(uint32_t offset);
// 				void requestRemoveFile(const QString &targetFile);
// 				void requestReadFile(uint32_t offset);

// 		private slots:
// 				void sendMessage(Message message);

// 				void handleTerminateSession(const Message &message);
// 				void handleListDirectoryMessage(const Message &message);
// 				void handleOpenFileRO(const Message &message);
// 				void handleBurstReadFile(const Message &message);
// 				void handleRemoveFile(const Message &message);

// 		signals:
// 				void requestMessage(const mavlink_message_t &mavlinkMessage);

// 		private:
// 				UUav *_uav;
// 				uint8_t _uavID;
// 				uint8_t _gcsID;
// 				uint8_t _chan;

// 				const QLoggingCategory loggingCategory = \
// 						QLoggingCategory("FileTransferProtocol");
// 		};

// }

// //////////////////////////////////
// //////////////////////////////////
// #include "FileTransferProtocolWorker.hpp"

// namespace FileTransferProtocol
// {

// Worker:: Worker(UUav *uav, QObject *parent)
// 		: QObject(parent), _uav(uav)
// {

// 		connect(_uav->fileTransferProtocol(),
// &UFileTransferProtocol::requestListDirectory, this,
// &Worker::requestListDirectory); 		connect(_uav->fileTransferProtocol(),
// &UFileTransferProtocol::requestBurstReadFile, this,
// &Worker::requestOpenFileRO); 		connect(_uav->fileTransferProtocol(),
// &UFileTransferProtocol::requestRemoveFile, this, &Worker::requestRemoveFile);
// }

// void Worker::init(uint8_t uavID, uint8_t gcsID, uint8_t chan)
// {
// 		_uavID = uavID;
// 		_gcsID = gcsID;
// 		_chan  = chan;
// }

// void Worker::handleMessage(const mavlink_message_t &mavlinkMessage)
// {
// 		if (mavlinkMessage.msgid != MAVLINK_MSG_ID_FILE_TRANSFER_PROTOCOL) {
// return; }

// 		mavlink_file_transfer_protocol_t fileTransferProtocolMessage;
// 		mavlink_msg_file_transfer_protocol_decode(&mavlinkMessage,
// &fileTransferProtocolMessage);

// 		Message message;
// 		memcpy(&message, fileTransferProtocolMessage.payload, sizeof(Message));

// 		uint16_t sequence = _uav->fileTransferProtocol()->sequence();
// 		if (message.sequense != 0 && message.sequense <= sequence) { return; };
// 		_uav->fileTransferProtocol()->setSequence(message.sequense);

// 		qCDebug(loggingCategory) << message.sequense
// 														 << message.session
// 														 << (uint8_t)message.opcode
// 														 << message.size
// 														 << (uint8_t)message.requestOpcode
// 														 << message.offset;

// 		switch (message.requestOpcode)
// 		{
// 				case Message::Opcode::TerminateSession:
// 				{
// 						handleTerminateSession(message);
// 						break;
// 				}
// 				case Message::Opcode::ListDirectory:
// 				{
// 						handleListDirectoryMessage(message);
// 						break;
// 				}
// 				case Message::Opcode::OpenFileRO:
// 				{
// 						handleOpenFileRO(message);
// 						break;
// 				}
// 				case Message::Opcode::BurstReadFile:
// 				{
// 						handleBurstReadFile(message);
// 						break;
// 				}
// 				case Message::Opcode::RemoveFile:
// 				{
// 						handleRemoveFile(message);
// 						break;
// 				}
// 				default:
// 				{
// 						qCWarning(loggingCategory, "Unrealized: %d",
// (uint8_t)message.requestOpcode);
// 				}
// 		}
// }

// void Worker::requestTerminateSession()
// {
// 		qCDebug(loggingCategory) << "Обрыв текущей сесиии";

// 		Message message;
// 		message.opcode = Message::Opcode::TerminateSession;

// 		sendMessage(message);
// }

// void Worker::requestListDirectory(const QString &path)
// {
// 		qCDebug(loggingCategory) << "Запрос содержимого директории:" << path;

// 		Message message;
// 		message.opcode = Message::Opcode::ListDirectory;
// 		message.size = path.size();
// 		memcpy(message.payload, path.toLocal8Bit(), path.size());

// 		sendMessage(message);
// }

// void Worker::requestOpenFileRO(const QString &source)
// {
// 		qCDebug(loggingCategory) << "Запрос на открытие файла:" << source;

// 		if (_uav->fileTransferProtocol()->readFileInfo()->lastReadOffset != 0)
// 		{
// 				emit _uav->fileTransferProtocol()->replyError("Дождитесь загрузки
// текущего файла!"); 				return;
// 		};

// 		Message message;
// 		message.opcode = Message::Opcode::OpenFileRO;
// 		message.size = source.size();
// 		memcpy(message.payload, source.toLocal8Bit(), source.size());

// 		sendMessage(message);
// }

// void Worker::requestBurstReadFile(uint32_t offset)
// {
// 		qCDebug(loggingCategory) << "Чтение файла с позиции:" << offset;

// 		Message message;
// 		message.opcode = Message::Opcode::BurstReadFile;
// 		message.size = sizeof(message.payload);
// 		message.offset = offset;

// 		sendMessage(message);
// }

// void Worker::requestRemoveFile(const QString &targetFile)
// {
// 		qCDebug(loggingCategory) << "Удаление файла:" << targetFile;

// 		Message message;
// 		message.opcode = Message::Opcode::RemoveFile;
// 		message.size = targetFile.size();
// 		memcpy(message.payload, targetFile.toLocal8Bit(), targetFile.size());

// 		sendMessage(message);
// }

// void Worker::requestReadFile(uint32_t offset)
// {
// 		qCDebug(loggingCategory) << "Чтение куска файла с позиции:" << offset;

// 		Message message;
// 		message.opcode = Message::Opcode::ReadFile;
// 		message.size = sizeof(message.payload);
// 		message.offset = offset;

// 		sendMessage(message);
// }

// void Worker::sendMessage(Message message)
// {
// 		message.session = _uav->fileTransferProtocol()->session();
// 		message.sequense = (_uav->fileTransferProtocol()->sequence() + 1) %
// 0xFFFF; 		_uav->fileTransferProtocol()->setSequence(message.sequense);

// 		mavlink_file_transfer_protocol_t fileTransferProtocolMessage;
// 		fileTransferProtocolMessage.target_network = 0;
// 		fileTransferProtocolMessage.target_system = 1;
// 		fileTransferProtocolMessage.target_component = 1;
// 		memcpy(fileTransferProtocolMessage.payload, &message, sizeof(Message));

// 		mavlink_message_t mavlinkMessage;
// 		mavlink_msg_file_transfer_protocol_encode(255, 190, &mavlinkMessage,
// &fileTransferProtocolMessage);

// 		emit requestMessage(mavlinkMessage);
// }

// void Worker::handleTerminateSession(const Message &message)
// {
// 		if (message.opcode == Message::Opcode::Ack)
// 		{
// 				qCDebug(loggingCategory, "Текущая сессия сброшена: %d",
// message.session);
// _uav->fileTransferProtocol()->setSession((message.session + 1) % 0xFF);
// _uav->fileTransferProtocol()->setSequence(0);
// 		}
// }

// void Worker::handleListDirectoryMessage(const Message &message)
// {
// 		switch (message.opcode)
// 		{
// 				case Message::Opcode::Ack:
// 				{
// 						qCDebug(loggingCategory, "Парсинг содержимого директории");

// 						QList<QString> content;
// 						QByteArray payload = QByteArray((char*)message.payload,
// message.size); 						for (QByteArray name : payload.split(0x00))
// 						{
// 								if (name.size() == 0) { continue; };

// 								content.append(name);
// 						}
// 						qCDebug(loggingCategory) << content;
// 						_uav->fileTransferProtocol()->handleListDirectory(content);

// 						QString path = _uav->fileTransferProtocol()->listDirectoryPath();
// 						uint8_t offset =
// _uav->fileTransferProtocol()->listDirectoryOffset();

// 						Message message;
// 						message.opcode = Message::Opcode::ListDirectory;
// 						message.size = path.size();
// 						message.requestOpcode = Message::Opcode::None;
// 						message.offset = offset;
// 						memcpy(message.payload, path.toLocal8Bit(), path.size());

// 						sendMessage(message);
// 						break;
// 				}
// 				default:
// 				{
// 						_uav->fileTransferProtocol()->handleListDirectory({});
// 				}
// 		}
// }

// void Worker::handleOpenFileRO(const Message &message)
// {
// 		switch (message.opcode)
// 		{
// 				case Message::Opcode::Ack:
// 				{
// 						uint32_t size;
// 						memcpy(&size, message.payload, message.size);
// 						qCDebug(loggingCategory) << "Размер файла:" << size;
// 						_uav->fileTransferProtocol()->handleOpenFileRO(size);

// 						QFile
// file(_uav->fileTransferProtocol()->readFileInfo()->targetFileName); if
// (!file.open(QFile::WriteOnly)) { 								emit
// _uav->fileTransferProtocol()->replyError("Не удалось создать файл!");
// return;
// 						};

// 						requestBurstReadFile(0);
// 						break;
// 				}
// 				default:
// 				{

// 				}
// 		}
// }

// void Worker::handleBurstReadFile(const Message &message)
// {
// 		switch (message.opcode)
// 		{
// 				case Message::Opcode::Ack:
// 				{
// 						_uav->fileTransferProtocol()->handleBurstReadFile(message.offset,
// 																															message.size,
// 																															(uint8_t*)message.payload);

// 						if (!_uav->fileTransferProtocol()->readFileInfo()->lostReadChunk)
// 						{
// 								QFile
// file(_uav->fileTransferProtocol()->readFileInfo()->targetFileName); if
// (file.open(QFile::Append)) { file.write((char*)message.payload,
// message.size); 										file.close();
// 								}
// 						}
// 						else
// 						{
// 								qCDebug(loggingCategory) << "Игнорирование пакета";
// 						}

// 						if (message.burstComplete)
// 						{
// 								requestBurstReadFile(_uav->fileTransferProtocol()->readFileInfo()->lastReadOffset);
// 						}

// 						break;
// 				}
// 				case Message::Opcode::Nack:
// 				{
// 						if (message.payload[0] == (uint8_t)Message::Error::EndOfFile)
// 						{
// 								if
// (!_uav->fileTransferProtocol()->readFileInfo()->lostReadChunk)
// 								{
// 										_uav->fileTransferProtocol()->handleBurstReadFileFinished();
// 								}
// 								else
// 								{
// 										requestBurstReadFile(_uav->fileTransferProtocol()->readFileInfo()->lastReadOffset);
// 										break;
// 								}
// 						}
// 						else
// 						{
// 								emit _uav->fileTransferProtocol()->replyError("Ошибка в
// загрузке файла");
// 						}
// 				}
// 				default:
// 				{
// 						requestTerminateSession();
// 				}
// 		}
// }

// void Worker::handleRemoveFile(const Message &message)
// {
// 		switch (message.opcode) {
// 				case Message::Opcode::Ack:
// 				{
// 						_uav->fileTransferProtocol()->handleRemoveFile();
// 						break;
// 				}
// 				case Message::Opcode::Nack:
// 				{
// 						emit _uav->fileTransferProtocol()->replyError("Не удалось удалить
// файл!");
// 				}
// 				default: { break; }
// 		}
// }

// }
