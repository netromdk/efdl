#include <QUrl>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>
#include <QCommandLineParser>

#include "Util.h"
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

  QCommandLineOption verboseOpt(QStringList{"verbose"},
                                QObject::tr("Verbose mode."));
  parser.addOption(verboseOpt);

  QCommandLineOption confirmOpt(QStringList{"confirm"},
                                QObject::tr("Confirm to download on redirections."));
  parser.addOption(confirmOpt);

  QCommandLineOption outputOpt(QStringList{"o", "output"},
                              QObject::tr("Where to save file. (defaults to "
                                          "current directory)"),
                              QObject::tr("dir"));
  parser.addOption(outputOpt);

  QCommandLineOption connsOpt(QStringList{"c", "conns"},
                              QObject::tr("Number of simultaneous connections to"
                                          " use. (defaults to 1)"),
                              QObject::tr("num"));
  parser.addOption(connsOpt);

  QCommandLineOption chunksOpt(QStringList{"chunks"},
                               QObject::tr("Number of chunks to split the download"
                                           "up into. Cannot be used with --chunk-size."),
                               QObject::tr("num"));
  parser.addOption(chunksOpt);

  QCommandLineOption chunkSizeOpt(QStringList{"chunk-size"},
                                  QObject::tr("Size of each chunk which dictates "
                                              "how many to use. Cannot be used "
                                              "with --chunks."),
                                  QObject::tr("bytes"));
  parser.addOption(chunkSizeOpt);

  QCommandLineOption connProgOpt(QStringList{"show-conn-progress"},
                                 QObject::tr("Shows progress information for each "
                                             "connection."));
  parser.addOption(connProgOpt);

  // Process CLI arguments.
  parser.process(app);
  const QStringList args = parser.positionalArguments();
  if (args.size() != 1) {
    parser.showHelp(-1);
  }

  int conns{1}, chunks{-1}, chunkSize{-1};
  bool ok{false}, confirm{parser.isSet(confirmOpt)},
    verbose{parser.isSet(verboseOpt)},
    connProg{parser.isSet(connProgOpt)};
  QString dir;

  if (parser.isSet(outputOpt)) {
    dir = parser.value(outputOpt);
    if (!QDir().exists(dir)) {
      qCritical() << "ERROR Output directory does not exist:" << dir;
      return -1;
    }
  }

  if (parser.isSet(connsOpt)) {
    conns = parser.value(connsOpt).toInt(&ok);
    if (!ok || conns <= 0) {
      qCritical() << "ERROR Number of connections must be a positive number!";
      return -1;
    }
  }

  if (parser.isSet(chunksOpt)) {
    chunks = parser.value(chunksOpt).toInt(&ok);
    if (!ok || chunks <= 0) {
      qCritical() << "ERROR Number of chunks must be a positive number!";
      return -1;
    }
  }

  if (parser.isSet(chunkSizeOpt)) {
    chunkSize = parser.value(chunkSizeOpt).toInt(&ok);
    if (!ok || chunkSize <= 0) {
      qCritical() << "ERROR Chunk size must be a positive number!";
      return -1;
    }
  }

  if (chunks != -1 && chunkSize != -1) {
    qCritical() << "ERROR --chunks and --chunk-size cannot be used at the same time!";
    return -1;
  }

  QUrl url{args[0], QUrl::StrictMode};
  if (!url.isValid()) {
    qCritical() << "ERROR Invalid URL!";
    return -1;
  }

  const QStringList schemes{"http", "https"};
  if (!schemes.contains(url.scheme().toLower())) {
    qCritical() << "ERROR Invalid scheme! Valid ones are:" <<
      qPrintable(schemes.join(" "));
    return -1;
  }

  Util::registerCustomTypes();

  // Begin transfer in event loop.
  Downloader dl{url, dir, conns, chunks, chunkSize, confirm, connProg, verbose};
  QObject::connect(&dl, &Downloader::finished, &app, &QCoreApplication::quit);
  QTimer::singleShot(0, &dl, SLOT(start()));

  return app.exec();
}
