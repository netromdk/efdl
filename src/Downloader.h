#ifndef EFDL_DOWNLOADER_H
#define EFDL_DOWNLOADER_H

#include <QUrl>
#include <QMap>
#include <QPair>
#include <QMutex>
#include <QQueue>
#include <QObject>
#include <QDateTime>
#include <QByteArray>
#include <QThreadPool>
#include <QNetworkReply>
#include <QNetworkAccessManager>

#include "Range.h"
#include "CommitThread.h"

class QUrl;
class QNetworkReply;

class Downloader : public QObject {
  Q_OBJECT
  
public:
  Downloader(const QUrl &url, const QString &outputDir, int conns, int chunks,
             int chunkSize, bool confirm, bool verbose);

signals: 
  void finished();

  // Internal signal.
  void chunkToThread(const QByteArray *data, bool last);
    
public slots:
  void start();

private slots:
  void onDownloadTaskFinished(Range range, QByteArray *data);
  void onDownloadTaskFailed(Range range, int httpCode,
                            QNetworkReply::NetworkError error);
  void onCommitThreadFinished();
  
private:
  QNetworkReply *getHead(const QUrl &url);
  void createRanges();
  void setupThreadPool();
  void download();
  void saveChunk();
  void updateProgress();
  
  QUrl url;
  QString outputDir;
  int conns, chunks, chunkSize, downloadCount, rangeCount;
  qint64 contentLen, bytesDown;
  bool confirm, verbose, continuable;
  QDateTime started;

  QNetworkAccessManager netmgr;
  QNetworkReply *reply;

  QMutex finishedMutex;

  QQueue<Range> ranges;
  QThreadPool pool;
  QMap<qint64, QByteArray*> chunksMap; // range start -> data pointer
  CommitThread commitThread;
};

#endif // EFDL_DOWNLOADER_H
