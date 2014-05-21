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
               int chunkSize, bool confirm, bool resume, bool verbose,
               bool showHeaders);

    void createChecksum(QCryptographicHash::Algorithm hashAlg);

    QUrl getUrl() const { return url; }

  signals:
    void finished();
    void information(qint64 size, int chunksAmount, int conns, qint64 offset);

    // Signals for individual chunks.
    void chunkStarted(int num);
    void chunkProgress(int num, qint64 received, qint64 total);
    void chunkFinished(int num, Range range);
    void chunkFailed(int num, Range range, int httpCode,
                    QNetworkReply::NetworkError error);

    // Internal signal.
    void chunkToThread(const QByteArray *data, bool last);
    
  public slots:
    void start();

  private slots:
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
    void printChecksum();
  
    QUrl url;
    QString outputDir, outputPath;
    int conns, chunks, chunkSize, downloadCount, rangeCount;
    qint64 contentLen, offset;
    bool confirm, resume, verbose, showHeaders, resumable, chksum;
    QCryptographicHash::Algorithm hashAlg;

    QNetworkAccessManager netmgr;
    QNetworkReply *reply;

    QMutex finishedMutex;

    QQueue<Range> ranges;
    QThreadPool pool;
    QMap<qint64, QByteArray*> chunksMap; // range start -> data pointer
    CommitThread commitThread;
  };
}

#endif // EFDL_DOWNLOADER_H
