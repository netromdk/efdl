#include <QUrl>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "DownloadTask.h"

DownloadTask::DownloadTask(const QUrl &url, Range range)
  : url{url}, range{range}
{
  setAutoDelete(true);
}

void DownloadTask::run() {
  qint64 start = range.first, end = range.second;
  QString rangeHdr = QString("bytes=%1-%2").arg(start).arg(end);
  //qDebug() << "RANGE" << qPrintable(rangeHdr);

  QNetworkRequest req{url};
  req.setRawHeader("Range", rangeHdr.toUtf8());

  QNetworkAccessManager netmgr;
  auto *rep = netmgr.get(req);
  
  QEventLoop loop;
  QObject::connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  //qDebug() << "CODE" << code;
  //qDebug() << "HEADERS" << rep->rawHeaderPairs();

  // Partial download.
  bool ok = false;
  QByteArray *dataPtr{nullptr};
  if (code == 206) {
    ok = true;
    dataPtr = new QByteArray;
    *dataPtr = rep->readAll();
  }

  auto error = rep->error();
  rep->close();

  if (ok) {
    emit finished(range, dataPtr);
  }
  else {
    emit failed(range, code, error);
  }
}
