#include <QUrl>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QTextStream>
#include <QCoreApplication>
#include <QCommandLineParser>

#include "Util.h"
#include "Version.h"
#include "Downloader.h"
using namespace efdl;

#include "DownloadManager.h"

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationName("efdl");
  QCoreApplication::setApplicationVersion(versionString(false));

  // Read possible URLs from STDIN.
  QString pipeIn;
  if (Util::isStdinPipe()) {
    QTextStream stream(stdin, QIODevice::ReadOnly);
    pipeIn = stream.readAll();
  }

  QCommandLineParser parser;
  parser.setApplicationDescription(QObject::tr("Efficient downloading application.") +
                                   "\n\n" +
                                   QObject::tr("If URLs are given through STDIN "
                                               "then the positional argument(s) "
                                               "are optional.") +
                                   "\n" +
                                   QObject::tr("Also note that piping URLs will "
                                               "make confirmations default to "
                                               "'no', if any."));
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addPositionalArgument("URLs", QObject::tr("URLs to download."), "URLs..");

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

  QCommandLineOption resumeOpt(QStringList{"r", "resume"},
                                QObject::tr("Resume download if file is present "
                                            "locally and the server supports it."));
  parser.addOption(resumeOpt);

  QCommandLineOption confirmOpt(QStringList{"confirm"},
                                QObject::tr("Will ask to confirm to download on "
                                            "redirections or whether to truncate"
                                            " a completed file when resuming."));
  parser.addOption(confirmOpt);

  QCommandLineOption verboseOpt(QStringList{"verbose"},
                                QObject::tr("Verbose mode."));
  parser.addOption(verboseOpt);

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

  QCommandLineOption showHeadersOpt(QStringList{"show-http-headers"},
                                    QObject::tr("Shows all HTTP headers. Implies --verbose."));
  parser.addOption(showHeadersOpt);

  QCommandLineOption chksumOpt(QStringList{"checksum"},
                               QObject::tr("Generate a checksum of the downloaded "
                                           "file using the given hash function. "
                                           "Options are: md4, md5, sha1, sha2-224,"
                                           " sha2-256, sha2-384, sha2-512, sha3-224,"
                                           " sha3-256, sha3-384, sha3-512"),
                               QObject::tr("fmt"));
  parser.addOption(chksumOpt);

  // Process CLI arguments.
  parser.process(app);
  QStringList args = parser.positionalArguments();
  args.append(pipeIn.split('\n', QString::SkipEmptyParts));
  if (args.size() < 1) {
    parser.showHelp(-1);
  }

  int conns{1}, chunks{-1}, chunkSize{-1};
  bool ok{false}, confirm{parser.isSet(confirmOpt)},
    verbose{parser.isSet(verboseOpt)},
    resume{parser.isSet(resumeOpt)},
    connProg{parser.isSet(connProgOpt)},
    showHeaders{parser.isSet(showHeadersOpt)};
  QString dir;
  bool chksum{false};
  QCryptographicHash::Algorithm hashAlg{QCryptographicHash::Sha3_512};

  if (showHeaders) verbose = true;

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

  if (parser.isSet(chksumOpt)) {
    QString alg{parser.value(chksumOpt).trimmed().toLower()};
    chksum = true;
    if (!Util::stringToHashAlg(alg, hashAlg)) {
      qCritical() << "ERROR Invalid hash function:" << qPrintable(alg);
      return -1;
    }
  }

  DownloadManager manager{connProg};
  const QStringList schemes{"http", "https"};
  foreach (const QString &arg, args) {
    QUrl url{arg.trimmed(), QUrl::StrictMode};
    if (!url.isValid()) {
      qCritical() << "ERROR Invalid URL:" << arg;
      return -1;
    }

    if (!schemes.contains(url.scheme().toLower())) {
      qCritical() << "ERROR Invalid scheme:" << qPrintable(arg);
      qCritical() << "Valid ones are:" << qPrintable(schemes.join(" "));
      return -1;
    }

    auto *dl = new Downloader{url, dir, conns, chunks, chunkSize, confirm,
                              resume, verbose, showHeaders};
    if (chksum) {
      dl->createChecksum(hashAlg);
    }

    manager.add(dl);
  }

  Util::registerCustomTypes();

  // Begin downloads in event loop.
  QObject::connect(&manager, &DownloadManager::finished,
                   &app, &QCoreApplication::quit);
  QTimer::singleShot(0, &manager, SLOT(start()));

  return app.exec();
}
