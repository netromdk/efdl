#ifndef EFDL_DOWNLOADER_H
#define EFDL_DOWNLOADER_H

#include <QUrl>
#include <QPair>
#include <QQueue>
#include <QObject>
#include <QByteArray>
#include <QThreadPool>
#include <QNetworkReply>
#include <QNetworkAccessManager>

#include "Range.h"

class QUrl;
class QNetworkReply;

class Downloader : public QObject {
  Q_OBJECT
  
public:
  Downloader(const QUrl &url, int conns);

signals: 
  void finished();
    
public slots:
  void start();

private slots:
  void onDownloadTaskFinished(Range range, QByteArray *data);
  void onDownloadTaskFailed(Range range, int httpCode,
                            QNetworkReply::NetworkError error);
  
private:
  QNetworkReply *getHead(const QUrl &url);
  void createRanges();
  void setupThreadPool();
  void download();
  
  QUrl url;
  int conns;
  qint64 contentLen;
  bool continuable;

  QNetworkAccessManager netmgr;
  QNetworkReply *reply;

  QQueue<Range> ranges;
  QThreadPool pool;
};

#endif // EFDL_DOWNLOADER_H
