#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  QTranslator translator;
  const QStringList uiLanguages =
      QLocale::system().uiLanguages()
      << QLocale(QLocale::Language::Russian).uiLanguages();
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
