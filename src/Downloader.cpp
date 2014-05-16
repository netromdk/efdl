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

Downloader::Downloader(const QUrl &url, int conns, bool confirm)
  : url{url}, conns{conns}, downloadCount{0}, rangeCount{0}, contentLen{0},
  confirm{confirm}, continuable{false}, reply{nullptr}
{
  connect(&commitThread, &CommitThread::finished,
          this, &Downloader::onCommitThreadFinished);
  connect(this, &Downloader::chunkToThread,
          &commitThread, &CommitThread::enqueueChunk,
          Qt::QueuedConnection);
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
  qDebug() << "SIZE" << contentLen;

  // Check for header "Accept-Ranges" and whether it has "bytes"
  // supported.
  continuable = false;
  if (reply->hasRawHeader("Accept-Ranges")) {
    const QString ranges = QString::fromUtf8(reply->rawHeader("Accept-Ranges"));
    continuable = ranges.toLower().contains("bytes");
  }
  qDebug() << qPrintable(QString("%1CONTINUABLE").
                         arg(continuable ? "" : "NOT "));

  // Clean reply.
  reply->close();
  reply = nullptr;

  createRanges();
  setupThreadPool();

  // Start actual download.
  download();
}

void Downloader::onDownloadTaskFinished(Range range, QByteArray *data) {
  QMutexLocker locker{&finishedMutex};
  chunks[range.first] = data;
  downloadCount++;
  qDebug() << "DL" << float(downloadCount) / float(rangeCount) * 100.0 << "%";
  saveChunk();
}

void Downloader::onDownloadTaskFailed(Range range, int httpCode,
                                      QNetworkReply::NetworkError error) {
  qDebug() << "failed" << range << Util::getErrorString(error);
}

void Downloader::onCommitThreadFinished() {
  qDebug() << "SUCCESS";
  emit finished();
}

QNetworkReply *Downloader::getHead(const QUrl &url) {
  qDebug() << "HEAD" << qPrintable(url.toString());

  QNetworkRequest req{url};
  auto *rep = netmgr.head(req);
  QEventLoop loop;
  connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (rep->error() != QNetworkReply::NoError) {
    rep->abort();
    qCritical() << "ERROR" << qPrintable(Util::getErrorString(rep->error()));
    return nullptr;
  }  

  int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  qDebug() << "CODE" << code;
  //qDebug() << "HEADERS" << rep->rawHeaderPairs();

  static bool didRedir = false;

  if (code >= 200 && code < 300) {
    qDebug() << "Resolved URL:" << qPrintable(url.toString());
    if (confirm && didRedir) {
      if (!Util::askProceed(tr("Do you want to continue?") + " [y/N] ")) {
        rep->abort();
        qCritical() << "Aborting..";
        return nullptr;
      }
    }
  }

  // Handle redirect.
  if (code >= 300 && code < 400) {
    if (!rep->hasRawHeader("Location")) {
      rep->abort();
      qCritical() << "Could not resolve URL!";
      return nullptr;
    }    

    QString locHdr = QString::fromUtf8(rep->rawHeader("Location"));

    QUrl loc{locHdr, QUrl::StrictMode};
    if (!loc.isValid()) {
      rep->abort();
      qCritical() << "Invalid redirection header:" <<
        qPrintable(loc.toString());
      return nullptr;
    }

    // If relative then try resolving with the previous URL.
    if (loc.isRelative()) {
      loc = url.resolved(loc);
    }

    qDebug() << "REDIRECT" << qPrintable(loc.toString());
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

void Downloader::createRanges() {
  ranges.clear();
  chunks.clear();

  constexpr qint64 chunkSize = 1048576;
  for (qint64 start = 0; start < contentLen; start += chunkSize) {
    qint64 end = start + chunkSize;
    if (end >= contentLen) {
      end = contentLen;
    }
    ranges.enqueue(Range{start, end - 1});
    chunks[start] = nullptr;
  }
  rangeCount = ranges.size();

  qDebug() << "CHUNKS" << rangeCount;
}

void Downloader::setupThreadPool() {
  // Cap connections to the amount of chunks to download.
  if (conns > ranges.size()) {
    int old{conns};
    conns = ranges.size();
    qDebug() << "CAP connection amount capped to amount of chunks:"
             << old << "->" << conns;
  }

  pool.setMaxThreadCount(conns);
}

void Downloader::download() {
  qDebug() << "DOWNLOAD" << qPrintable(url.path());

  QFileInfo fi{url.path()};
  QDir dir{QCoreApplication::instance()->applicationDirPath()};
  QString path{dir.absoluteFilePath(fi.baseName())};
  QString suf{fi.suffix()};
  if (!suf.isEmpty()) {
    path.append("." + fi.suffix());
  }
  qDebug() << "FILE" << qPrintable(path);

  auto *file = new QFile{path};
  if (file->exists()) {
    QFile::remove(path);
  }
  if (!file->open(QIODevice::WriteOnly)) {
    qCritical() << "ERROR Could not open file for writing!";
    delete file;
    QCoreApplication::exit(-1);
    return;
  }
  commitThread.setFile(file);

  // Fill queue with tasks and start immediately.
  while (!ranges.empty()) {
    auto range = ranges.dequeue();
    auto *task = new DownloadTask{url, range};
    connect(task, &DownloadTask::finished,
            this, &Downloader::onDownloadTaskFinished);
    connect(task, &DownloadTask::failed,
            this,  &Downloader::onDownloadTaskFailed);
    pool.start(task);
  }
}

void Downloader::saveChunk() {
  // Lock on chunks map is required to be acquired going into this
  // method!

  if (chunks.isEmpty()) {
    // Wait for commit thread to be done.
    return;
  }

  auto key = chunks.firstKey();
  const auto *value = chunks[key];
  if (value != nullptr) {
    emit chunkToThread(value, chunks.size() == 1);
    if (!commitThread.isRunning()) {
      commitThread.start();
    }
    chunks.remove(key);
  }

  // If everything has been downloaded then call method again.
  if (rangeCount == downloadCount) {
    saveChunk();
  }
}
