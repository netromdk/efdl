#include <QUrl>
#include <QDebug>
#include <QCoreApplication>
#include <QCommandLineParser>

#include "Version.h"

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
  QUrl url(args[0]);

  return app.exec();
}
