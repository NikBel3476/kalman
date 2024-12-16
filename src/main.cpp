#include "mainwindow.hpp"

#include <QApplication>
#include <QTranslator>
#include <ctime>
#include <format>

QtMessageHandler originalHandler = nullptr;

void logToFile(QtMsgType type, const QMessageLogContext &context,
							 const QString &msg) {
	QString message = qFormatLogMessage(type, context, msg);
	static FILE *f = fopen("log.txt", "a");
	fprintf(
			f, "%s %s\n",
			// std::ctime(&current_time),
			std::format("{:%Y-%m-%d %X}", std::chrono::system_clock::now()).c_str(),
			qPrintable(message));
	fflush(f);

	if (originalHandler)
		originalHandler(type, context, msg);
}

int main(int argc, char *argv[]) {
	originalHandler = qInstallMessageHandler(logToFile);
	// QCoreApplication::addLibraryPath("/lib");
	// QApplication::addLibraryPath("/lib");

#ifdef _WIN32
	QApplication::setStyle("fusion");
#endif
	QApplication a(argc, argv);

	QTranslator translator;
	const auto uiLanguages = QLocale::system().uiLanguages();
	// For translations check
	// const auto uiLanguages = QLocale(QLocale::Language::Russian).uiLanguages();
	for (const QString &locale : uiLanguages) {
		const QString baseName = "autopilot_selfcheck_" + QLocale(locale).name();
		if (translator.load(":/i18n/" + baseName)) {
			QApplication::installTranslator(&translator);
			break;
		}
	}

	MainWindow w;
	w.show();
	return QApplication::exec();
}
