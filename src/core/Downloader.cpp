#include <sstream>
#include <iostream>

#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QEventLoop>
#include <QMutexLocker>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCoreApplication>

#include "Util.h"
#include "Downloader.h"
#include "DownloadTask.h"

BEGIN_NAMESPACE

Downloader::Downloader(const QUrl &url)
  : url{url}, conns{1}, chunks{-1}, chunkSize{-1}, downloadCount{0},
    rangeCount{0}, contentLen{0}, offset{0}, confirm{false}, resume{false},
    verbose{false}, dryRun{false}, showHeaders{false}, resumable{false},
    reply{nullptr}
  {
    connect(&commitThread, &CommitThread::finished,
            this, &Downloader::onCommitThreadFinished);
    connect(this, &Downloader::chunkToThread,
            &commitThread, &CommitThread::enqueueChunk,
            Qt::QueuedConnection);
  }

void Downloader::setHttpCredentials(const QString &user,
                                    const QString &pass) {
  httpUser = user;
  httpPass = pass;
}

void Downloader::start() {
  // Fetch HEAD to find out the size but also if it exists.
  reply = getHead(url);
  if (!reply) {
    QCoreApplication::exit(-1);
    return;
  }
  url = reply->url();

  // Find "Content-Length".
  if (!reply->hasRawHeader("Content-Length")) {
    qCritical() << "ERROR Invalid response from server. No"
                << "'Content-Length' header present!";
    QCoreApplication::exit(-1);
    return;
  }
  bool ok;
  contentLen =
    QString::fromUtf8(reply->rawHeader("Content-Length")).toLongLong(&ok);
  if (!ok || contentLen == 0) {
    qCritical() << "ERROR Invalid content length:" << contentLen;
    QCoreApplication::exit(-1);
    return;
  }

  // Check if the total is different than the Content-Length and, if
  // so, then change to that number.
  if (reply->hasRawHeader("Content-Range")) {
    QString range = QString::fromUtf8(reply->rawHeader("Content-Range"));
    QStringList elms = range.split("/", QString::SkipEmptyParts);
    if (elms.size() == 2) {
      qint64 tot = elms[1].toLongLong(&ok);
      if (ok && tot > 0 && tot != contentLen) {
        contentLen = tot;
      }
    }
  }

  QString type;
  if (reply->hasRawHeader("Content-Type")) {
    type = reply->rawHeader("Content-Type");
    int pos;
    if ((pos = type.indexOf(";")) != -1) {
      type = type.mid(0, pos);
    }
  }

  qDebug() << "File size" << qPrintable(Util::formatSize(contentLen, 1))
           << qPrintable(!type.isEmpty() ? "[" + type + "]" : "");

  // Check for header "Accept-Ranges" and whether it has "bytes"
  // supported.
  resumable = false;
  if (reply->hasRawHeader("Accept-Ranges")) {
    const QString ranges = QString::fromUtf8(reply->rawHeader("Accept-Ranges"));
    resumable = ranges.toLower().contains("bytes");
  }
  if (verbose) {
    qDebug() << qPrintable(QString("%1RESUMABLE").
                           arg(resumable ? "" : "NOT "));
  }

  if (!resumable && resume) {
    qCritical() << "ERROR Cannot resume because server doesn't support it!";
    QCoreApplication::exit(-1);
    return;
  }

  // Clean reply.
  reply->close();
  reply = nullptr;

  // If performing a dry run then stop now.
  if (dryRun) {
    emit finished();
    return;
  }

  if (!setupFile()) {
    QCoreApplication::exit(-1);
    return;
  }

  createRanges();
  setupThreadPool();

  emit information(outputPath, contentLen, rangeCount, conns, offset);

  // Start actual download.
  download();
}

void Downloader::onDownloadTaskFinished(int num, Range range, QByteArray *data) {
  QMutexLocker locker{&finishedMutex};
  chunksMap[range.first] = data;
  downloadCount++;
  saveChunk();

  emit chunkFinished(num, range);
}

void Downloader::onDownloadTaskFailed(int num, Range range, int httpCode,
                                      QNetworkReply::NetworkError error) {
  emit chunkFailed(num, range, httpCode, error);
}

void Downloader::onCommitThreadFinished() {
  emit finished();
}

QNetworkReply *Downloader::getHead(const QUrl &url) {
  if (verbose) {
    qDebug() << "HEAD" << qPrintable(url.toString(QUrl::FullyEncoded));
  }

  // Emulating a HEAD by doing a GET which only retrieves range
  // 0-0. This is necessary because some sites return different
  // headers for HEAD/GET even though they should be the same!

  QNetworkRequest req{url};
  req.setRawHeader("Range", QString("bytes=0-0").toUtf8());
  req.setRawHeader("Accept-Encoding", "identity");

  if (!httpUser.isEmpty() && !httpPass.isEmpty()) {
    req.setRawHeader("Authorization",
                     Util::createHttpAuthHeader(httpUser, httpPass));
  }

  //auto *rep = netmgr.head(req);
  auto *rep = netmgr.get(req);

  QEventLoop loop;
  connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (rep->error() != QNetworkReply::NoError) {
    rep->abort();
    qCritical() << "ERROR" << qPrintable(Util::getErrorString(rep->error()));
    return nullptr;
  }

  int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (verbose) {
    qDebug() << "CODE" << code;
    if (showHeaders) {
      qDebug() << "HEADERS"
               << qPrintable(Util::formatHeaders(rep->rawHeaderPairs()));
    }
  }

  static bool didRedir = false;

  if (code >= 200 && code < 300) {
    if (url != this->url) {
      qDebug() << "Resolved to"
               << qPrintable(url.toString(QUrl::FullyEncoded));
    }

    if (confirm && didRedir) {
      if (!Util::askProceed(tr("Do you want to continue?") + " [y/N] ")) {
        rep->abort();
        qCritical() << "Aborting..";
        return nullptr;
      }
    }
  }

  // Handle redirect.
  else if (code >= 300 && code < 400) {
    if (!rep->hasRawHeader("Location")) {
      rep->abort();
      qCritical() << "ERROR Could not resolve URL!";
      return nullptr;
    }

    QString locHdr = QString::fromUtf8(rep->rawHeader("Location"));
    QUrl loc{locHdr};
    if (!loc.isValid()) {
      rep->abort();
      qCritical() << "ERROR Invalid redirection header:"
                  << qPrintable(loc.toString(QUrl::FullyEncoded));
      return nullptr;
    }

    // If relative then try resolving with the previous URL.
    if (loc.isRelative()) {
      loc = url.resolved(loc);
    }

    if (verbose) {
      qDebug() << "REDIRECT" << qPrintable(loc.toString(QUrl::FullyEncoded));
    }

    didRedir = true;
    rep->abort();
    rep = getHead(loc);
  }

  // Client errors.
  else if (code >= 400 && code < 500) {
    rep->abort();
    qDebug() << "CLIENT ERROR";
    return nullptr;
  }

  // Server errors.
  else if (code >= 500 && code < 600) {
    rep->abort();
    qDebug() << "SERVER ERROR";
    return nullptr;
  }

  return rep;
}

bool Downloader::setupFile() {
  QFileInfo fi{url.path()};
  QDir dir = (outputDir.isEmpty() ? QDir::current() : outputDir);
  outputPath = dir.absoluteFilePath(fi.fileName());
  qDebug() << "Saving to" << qPrintable(outputPath);

  auto *file = new QFile{outputPath};
  if (file->exists() && !resume) {
    if (!QFile::remove(outputPath)) {
      qCritical() << "ERROR Could not truncate output file!";
      delete file;
      return false;
    }
  }

  if (resume) {
    qint64 fileSize{file->size()};
    if (fileSize >= contentLen) {
      qCritical() << "Cannot resume download because the size is larger than or"
                  << "equal:" << qPrintable(Util::formatSize(fileSize, 1))
                  << "vs." << qPrintable(Util::formatSize(contentLen, 1));
      if (confirm &&
          !Util::askProceed("Do you want to truncate file and continue? [y/N] ")) {
        qCritical() << "Aborting..";
        delete file;
        return false;
      }
      if (!confirm) {
        qDebug() << "Truncating file";
      }
      resume = false;
    }
    else if (fileSize > 0) {
      offset = fileSize;
      float perc = (long double)offset / (long double)contentLen * 100.0;
      qDebug() << "Resuming at offset"
               << qPrintable(Util::formatSize(offset, 1))
               << qPrintable(QString("(%1%)").arg(perc, 0, 'f', 1));
    }
  }

  QIODevice::OpenMode openMode{QIODevice::WriteOnly};
  if (resume) {
    openMode |= QIODevice::Append;
  }
  else {
    openMode |= QIODevice::Truncate;
  }

  if (!file->open(openMode)) {
    qCritical() << "ERROR Could not open file for writing!";
    delete file;
    return false;
  }
  commitThread.setFile(file);

  return true;
}

void Downloader::createRanges() {
  ranges.clear();
  chunksMap.clear();

  qint64 size = 1048576;
  if (chunkSize != -1) {
    size = chunkSize;
  }
  else if (chunks != -1) {
    size = (contentLen - offset) / chunks;
  }
  else if (conns >= 8) {
    size = (contentLen - offset) / conns;
    constexpr qint64 MB{10485760}; // 10 MB
    if (size > MB) size = MB;
  }

  if (verbose) {
    qDebug() << "CHUNK SIZE" << qPrintable(Util::formatSize(size, 1));
  }

  for (qint64 start = offset; start < contentLen; start += size) {
    qint64 end = start + size;
    if (end >= contentLen) {
      end = contentLen;
    }
    ranges.enqueue(Range{start, end - 1});
    chunksMap[start] = nullptr;
  }
  rangeCount = ranges.size();

  if (verbose) {
    qDebug() << "CHUNKS" << rangeCount;
  }
}

void Downloader::setupThreadPool() {
  // Cap connections to the amount of chunks to download.
  if (conns > ranges.size()) {
    int old{conns};
    conns = ranges.size();
    qDebug() << "Connections capped to chunks:" << old << "->" << conns;
  }

  pool.setMaxThreadCount(conns);
}

void Downloader::download() {
  // Fill queue with tasks and start immediately.
  int num{1};
  while (!ranges.empty()) {
    auto range = ranges.dequeue();
    auto *task = new DownloadTask{url, range, num++, httpUser, httpPass};
    connect(task, SIGNAL(started(int)), SIGNAL(chunkStarted(int)));
    connect(task, SIGNAL(progress(int, qint64, qint64)),
            SIGNAL(chunkProgress(int, qint64, qint64)));
    connect(task, &DownloadTask::finished,
            this, &Downloader::onDownloadTaskFinished);
    connect(task, &DownloadTask::failed,
            this, &Downloader::onDownloadTaskFailed);
    pool.start(task);
  }
}

void Downloader::saveChunk() {
  // Lock on chunks map is required to be acquired going into this
  // method!

  if (chunksMap.isEmpty()) {
    // Wait for commit thread to be done.
    return;
  }

  auto key = chunksMap.firstKey();
  const auto *value = chunksMap[key];
  if (value != nullptr) {
    emit chunkToThread(value, chunksMap.size() == 1);
    if (!commitThread.isRunning()) {
      commitThread.start();
    }
    chunksMap.remove(key);
  }

  // If everything has been downloaded then call method again.
  if (rangeCount == downloadCount) {
    saveChunk();
  }
}

END_NAMESPACE
