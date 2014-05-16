#ifndef EFDL_DOWNLOAD_TASK_H
#define EFDL_DOWNLOAD_TASK_H

#include <QObject>
#include <QRunnable>
#include <QNetworkReply>

#include "Range.h"

class QUrl;

class DownloadTask : public QObject, public QRunnable {
  Q_OBJECT

public:
  DownloadTask(const QUrl &url, Range range);

signals:
  void finished(Range range, QByteArray *data);
  void failed(Range range, int httpCode, QNetworkReply::NetworkError error);

private:
  void run() override;

  const QUrl &url;
  Range range;
};

#endif // EFDL_DOWNLOAD_TASK_H
