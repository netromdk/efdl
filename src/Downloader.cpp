#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCoreApplication>

#include "Util.h"
#include "Downloader.h"
#include "DownloadTask.h"

Downloader::Downloader(const QUrl &url, int conns)
  : url{url}, conns{conns}, contentLen{0}, continuable{false}, reply{nullptr}
{ }

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
  qDebug() << "finished" << range << data->size();
  delete data;
}

void Downloader::onDownloadTaskFailed(Range range, int httpCode,
                                      QNetworkReply::NetworkError error) {
  qDebug() << "failed" << range << Util::getErrorString(error);
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

  constexpr qint64 chunkSize = 1048576;
  for (qint64 start = 0; start < contentLen; start += chunkSize) {
    qint64 end = start + chunkSize;
    if (end >= contentLen) {
      end = contentLen;
    }
    ranges.enqueue(Range{start, end - 1});
  }

  qDebug() << "CHUNKS" << ranges.size();
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

  /*
  qDebug() << "RETRIEVED" << data.size();

  if (data.size() != contentLen) {
    qCritical() << "ERROR Downloaded data length does not match! Got"
                << data.size() << "vs." << contentLen;
    QCoreApplication::exit(-1);    
    return;
  }

  QFileInfo fi{url.path()};
  QDir dir{QCoreApplication::instance()->applicationDirPath()};
  QString path{dir.absoluteFilePath(fi.baseName())};
  QString suf{fi.suffix()};
  if (!suf.isEmpty()) {
    path.append("." + fi.suffix());
  }
  qDebug() << "FILE" << qPrintable(path);

  QFile file{path};
  if (file.exists()) {
    QFile::remove(path);
  }
  if (!file.open(QIODevice::WriteOnly)) {
    qCritical() << "ERROR Could not open file for writing!";
    QCoreApplication::exit(-1);
    return;
  }

  qint64 wrote;
  if ((wrote = file.write(data)) != data.size()) {
    qCritical() << "ERROR Could not write the entire data:"
                << wrote << "of" << data.size();
    QCoreApplication::exit(-1);
    return;    
  }
  file.close();

  qDebug() << "SUCCESS";
  emit finished();
  */
}