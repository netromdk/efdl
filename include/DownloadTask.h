#ifndef EFDL_DOWNLOAD_TASK_H
#define EFDL_DOWNLOAD_TASK_H

#include <QTimer>
#include <QThread>
#include <QNetworkReply>

#include "Range.h"
#include "EfdlGlobal.h"

class QUrl;

BEGIN_NAMESPACE

class DownloadTask : public QThread {
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

protected:
  void run();

private slots:
  void onProgress(qint64 received, qint64 total);

private:
  const QUrl &url;
  Range range;
  int num;
  const QString &httpUser, &httpPass;
};

END_NAMESPACE

#endif // EFDL_DOWNLOAD_TASK_H
