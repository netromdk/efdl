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
#include <QCryptographicHash>
#include <QNetworkAccessManager>

#include "Range.h"
#include "EfdlGlobal.h"
#include "ThreadPool.h"
#include "CommitThread.h"

class QUrl;
class QNetworkReply;

BEGIN_NAMESPACE

class Downloader : public QObject {
  Q_OBJECT

public:
  Downloader(const QUrl &url);

  QUrl getUrl() const { return url; }

  void setOutputDir(const QString &outputDir) { this->outputDir = outputDir; }
  void setConnections(int conns) { this->conns = conns; }
  void setChunks(int chunks) { this->chunks = chunks; }
  void setChunkSize(int size) { this->chunkSize = size; }
  void setConfirm(bool confirm) { this->confirm = confirm; }
  void setResume(bool resume) { this->resume = resume; }
  void setVerbose(bool verbose) { this->verbose = verbose; }
  void setDryRun(bool dryRun) { this->dryRun = dryRun; }
  void setShowHeaders(bool show) { this->showHeaders = show; }
  void setHttpCredentials(const QString &user, const QString &pass);

signals:
  void finished();
  void information(const QString &outputPath, qint64 size, int chunksAmount,
                   int conns, qint64 offset);

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
  void stop();

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
  
  QUrl url;
  QString outputDir, outputPath, httpUser, httpPass, fileOverride;
  int conns, chunks, chunkSize, downloadCount, rangeCount;
  qint64 contentLen, offset;
  bool confirm, resume, verbose, dryRun, showHeaders, resumable;

  QNetworkAccessManager netmgr;
  QNetworkReply *reply;

  QMutex finishedMutex;

  QQueue<Range> ranges;
  ThreadPool pool;
  QMap<qint64, QByteArray*> chunksMap; // range start -> data pointer
  CommitThread commitThread;
};

END_NAMESPACE

#endif // EFDL_DOWNLOADER_H
