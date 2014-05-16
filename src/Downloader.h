#ifndef EFDL_DOWNLOADER_H
#define EFDL_DOWNLOADER_H

#include <QUrl>
#include <QMap>
#include <QPair>
#include <QMutex>
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
  void saveChunk();
  
  QUrl url;
  int conns, downloadCount, rangeCount;
  qint64 contentLen;
  bool continuable;

  QNetworkAccessManager netmgr;
  QNetworkReply *reply;

  QMutex finishedMutex;

  QQueue<Range> ranges;
  QThreadPool pool;
  QMap<qint64, QByteArray*> chunks; // range start -> data pointer
};

#endif // EFDL_DOWNLOADER_H
