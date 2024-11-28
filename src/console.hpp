#ifndef CONSOLE_H
#define CONSOLE_H

#include <QPlainTextEdit>

#include "mavlinkmanager.hpp"

class Console : public QPlainTextEdit {
	Q_OBJECT

public:
	explicit Console(QWidget *parent, MavlinkManager *mavlink_manager);

	void putData(const QByteArray &data);

private:
	void _handleMavlinkMessageReceive(const mavlink_message_t &mavlink_message);

	MavlinkManager *_mavlink_manager = nullptr;
};

#endif // CONSOLE_H
