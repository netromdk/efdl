#ifndef EFDL_DOWNLOAD_MANAGER_H
#define EFDL_DOWNLOAD_MANAGER_H

#include <QQueue>
#include <QMutex>
#include <QObject>
#include <QDateTime>
#include <QNetworkReply>
#include <QCryptographicHash>

#include "Range.h"

namespace efdl {
  class Downloader;
}

class Chunk {
public:
  Chunk(efdl::Range range, QDateTime started)
    : range{range}, started{started}
  { }

  bool isDone() const {
    return range.first > 0 && range.first == range.second;
  }

  efdl::Range range;
  QDateTime started, ended;
};

typedef QPair<QCryptographicHash::Algorithm, QString> HashPair;

class DownloadManager : public QObject {
  Q_OBJECT
  
public:
  DownloadManager(bool dryRun = false, bool connProg = false);
  ~DownloadManager();

  void add(efdl::Downloader *entry);

  void setVerifcations(const QList<HashPair> &pairs);
  void createChecksum(QCryptographicHash::Algorithm hashAlg);

signals:
  void finished();
                                         
public slots:
  void start();

private slots:
  void next();

  void onInformation(const QString &outputPath, qint64 size, int chunksAmount,
                     int conns, qint64 offset);
  void onChunkStarted(int num);
  void onChunkProgress(int num, qint64 received, qint64 total);
  void onChunkFinished(int num, efdl::Range range);
  void onChunkFailed(int num, efdl::Range range, int httpCode,
                     QNetworkReply::NetworkError error);

private:
  void cleanup();
  void updateChunkMap();
  void updateProgress();
  void verifyIntegrity(const HashPair &pair);
  void printChecksum();

  QQueue<efdl::Downloader*> queue;

  bool dryRun, connProg, chksum;
  QString outputPath;
  int conns, chunksAmount, chunksFinished;
  qint64 size, offset, bytesDown;
  QDateTime started, lastProgress;
  QList<HashPair> verifyList;
  QCryptographicHash::Algorithm hashAlg;
  efdl::Downloader *downloader;
  QMutex chunkMutex;
  QMap<int, Chunk*> chunkMap; // num -> download progress
};

#endif // EFDL_DOWNLOAD_MANAGER_H
