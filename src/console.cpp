#include "console.hpp"

#include <QScrollBar>

Console::Console(QWidget *parent)
		: QPlainTextEdit(parent) {
	setWindowTitle(tr("Mavlink messages"));
	// document()->setMaximumBlockCount(100);
	QPalette p = palette();
	p.setColor(QPalette::Base, Qt::black);
	p.setColor(QPalette::Text, Qt::green);
	setPalette(p);
}

void Console::putData(const QByteArray &data) {
	insertPlainText(data);

	QScrollBar *bar = verticalScrollBar();
	bar->setValue(bar->maximum());
}
