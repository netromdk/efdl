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
#include <QCryptographicHash>

#include "Range.h"
#include "CommitThread.h"

class QUrl;
class QNetworkReply;

namespace efdl {
  class Downloader : public QObject {
    Q_OBJECT
  
  public:
    Downloader(const QUrl &url, const QString &outputDir, int conns, int chunks,
               int chunkSize, bool confirm, bool resume, bool connProg,
               bool verbose, bool showHeaders);

    void createChecksum(QCryptographicHash::Algorithm hashAlg);

  signals:
    void finished();

    // Internal signal.
    void chunkToThread(const QByteArray *data, bool last);
    
  public slots:
    void start();

  private slots:
    void onDownloadTaskStarted(int num);
    void onDownloadTaskProgress(int num, qint64 received, qint64 total);
    void onDownloadTaskFinished(int num, Range range, QByteArray *data);
    void onDownloadTaskFailed(int num, Range range, int httpCode,
                              QNetworkReply::NetworkError error);
    void onCommitThreadFinished();
  
  private:
    QNetworkReply *getHead(const QUrl &url);
    bool setupFile();
    void createRanges();
    void setupThreadPool();
    void download();
    void saveChunk();
    void updateConnsMap();
    void updateProgress();
    void printChecksum();
  
    QUrl url;
    QString outputDir, outputPath;
    int conns, chunks, chunkSize, downloadCount, rangeCount;
    qint64 contentLen, offset, bytesDown;
    bool confirm, resume, connProg, verbose, showHeaders, resumable, chksum;
    QDateTime started;
    QCryptographicHash::Algorithm hashAlg;

    QNetworkAccessManager netmgr;
    QNetworkReply *reply;

    QMutex finishedMutex;

    QQueue<Range> ranges;
    QThreadPool pool;
    QMap<qint64, QByteArray*> chunksMap; // range start -> data pointer
    QMap<int, Range> connsMap; // num -> download progress
    CommitThread commitThread;
  };
}

#endif // EFDL_DOWNLOADER_H
