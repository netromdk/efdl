#include <QUrl>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QCoreApplication>
#include <QNetworkAccessManager>

#include "Util.h"
#include "DownloadTask.h"

BEGIN_NAMESPACE

DownloadTask::DownloadTask(const QUrl &url, Range range, int num,
                           const QString &httpUser, const QString &httpPass)
  : url{url}, range{range}, num{num}, httpUser{httpUser}, httpPass{httpPass}
{ }

void DownloadTask::onProgress(qint64 received, qint64 total) {
  emit progress(num, received, total);
}

void DownloadTask::run() {
  QNetworkRequest req{url};
  req.setRawHeader("Accept-Encoding", "identity");

  // Only set range information if appropriate.
  qint64 start = range.first, end = range.second;
  if (!(start == 0 && end == 0)) {
    QString rangeHdr = QString("bytes=%1-%2").arg(start).arg(end);
    //qDebug() << "RANGE" << qPrintable(rangeHdr);
    req.setRawHeader("Range", rangeHdr.toUtf8());
  }

  if (!httpUser.isEmpty() && !httpPass.isEmpty()) {
    req.setRawHeader("Authorization",
                     Util::createHttpAuthHeader(httpUser, httpPass));
  }

  QNetworkAccessManager netmgr;
  auto *rep = netmgr.get(req);
  emit started(num);

  connect(rep, &QNetworkReply::downloadProgress,
          this, &DownloadTask::onProgress);

  QDateTime lastTime{QDateTime::currentDateTime()};
  for (;;) {
    // Only check for interrupt every half second.
    QDateTime now{QDateTime::currentDateTime()};
    if (lastTime.msecsTo(now) > 500) {
      if (isInterruptionRequested()) {
        rep->abort();
        return;
      }
      lastTime = now;
    }

    if (rep->isFinished()) {
      break;
    }

    QCoreApplication::processEvents();
    msleep(10);
  }

  int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  //qDebug() << "CODE" << code;
  //qDebug() << "HEADERS" << rep->rawHeaderPairs();

  // Direct or partial download.
  bool ok = false;
  QByteArray *dataPtr{nullptr};
  if (code == 200 || code == 206) {
    ok = true;
    dataPtr = new QByteArray;
    *dataPtr = rep->readAll();
  }

  auto error = rep->error();
  rep->close();

  if (ok) {
    emit finished(num, range, dataPtr);
  }
  else {
    emit failed(num, range, code, error);
  }
}

END_NAMESPACE
