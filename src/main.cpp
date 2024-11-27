#include "mainwindow.hpp"

#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[]) {
	QCoreApplication::addLibraryPath("/lib");
	QApplication a(argc, argv);

	// QApplication::setStyle("fusion");
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
