#ifndef EFDL_DOWNLOAD_TASK_H
#define EFDL_DOWNLOAD_TASK_H

#include <QObject>
#include <QRunnable>
#include <QNetworkReply>

#include "Range.h"

class QUrl;

namespace efdl {
  class DownloadTask : public QObject, public QRunnable {
    Q_OBJECT

  public:
    DownloadTask(const QUrl &url, Range range, int num,
                 const QString &httpUser = QString(),
                 const QString &httpPass = QString());

  signals:
    void started(int num);
    void progress(int num, qint64 received, qint64 total);
    void finished(int num, Range range, QByteArray *data);
    void failed(int num, Range range, int httpCode,
                QNetworkReply::NetworkError error);

  private slots:
    void onProgress(qint64 received, qint64 total);

  private:
    void run() override;

    const QUrl &url;
    Range range;
    int num;
    const QString &httpUser, &httpPass;
  };
}

#endif // EFDL_DOWNLOAD_TASK_H
