#include <QUrl>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "Util.h"
#include "DownloadTask.h"

namespace efdl {
  DownloadTask::DownloadTask(const QUrl &url, Range range, int num,
                             const QString &httpUser, const QString &httpPass)
    : url{url}, range{range}, num{num}, httpUser{httpUser}, httpPass{httpPass}
  {
    setAutoDelete(true);
  }

  void DownloadTask::onProgress(qint64 received, qint64 total) {
    emit progress(num, received, total);
  }

  void DownloadTask::run() {
    qint64 start = range.first, end = range.second;
    QString rangeHdr = QString("bytes=%1-%2").arg(start).arg(end);
    //qDebug() << "RANGE" << qPrintable(rangeHdr);

    QNetworkRequest req{url};
    req.setRawHeader("Range", rangeHdr.toUtf8());
    req.setRawHeader("Accept-Encoding", "identity");

    if (!httpUser.isEmpty() && !httpPass.isEmpty()) {
      req.setRawHeader("Authorization",
                       Util::createHttpAuthHeader(httpUser, httpPass));
    }

    QNetworkAccessManager netmgr;
    auto *rep = netmgr.get(req);
    emit started(num);

    connect(rep, &QNetworkReply::downloadProgress,
            this, &DownloadTask::onProgress);
  
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
      emit finished(num, range, dataPtr);
    }
    else {
      emit failed(num, range, code, error);
    }
  }
}
