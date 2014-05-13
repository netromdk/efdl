#include <QUrl>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include <QCommandLineParser>

#include "Version.h"
#include "Downloader.h"

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("efdl");
  QCoreApplication::setApplicationVersion(versionString(false));

  QCommandLineParser parser;
  parser.setApplicationDescription(QObject::tr("Efficient downloading application."));
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument("URL", QObject::tr("URL to download."));

  // Process CLI arguments.
  parser.process(app);
  const QStringList args = parser.positionalArguments();

  QUrl url{args[0], QUrl::StrictMode};
  if (!url.isValid()) {
    qCritical() << "Invalid URL!";
    return -1;
  }

  const QStringList schemes{"http", "https"};
  if (!schemes.contains(url.scheme().toLower())) {
    qCritical() << "Invalid scheme! Valid ones are:" <<
      qPrintable(schemes.join(" "));
    return -1;
  }

  // Begin transfer in event loop.
  Downloader dl{url};
  QObject::connect(&dl, &Downloader::finished, &app, &QCoreApplication::quit);
  QTimer::singleShot(0, &dl, SLOT(start()));

  return app.exec();
}
