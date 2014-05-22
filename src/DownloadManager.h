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

class DownloadManager : public QObject {
  Q_OBJECT
  
public:
  DownloadManager(bool connProg);
  ~DownloadManager();

  void add(efdl::Downloader *entry);

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
  void updateConnsMap();
  void updateProgress();
  void printChecksum();

  QQueue<efdl::Downloader*> queue;

  bool connProg, chksum;
  QString outputPath;
  int conns, chunksAmount, chunksFinished;
  qint64 size, offset, bytesDown;
  QDateTime started, lastProgress;
  QCryptographicHash::Algorithm hashAlg;
  efdl::Downloader *downloader;
  QMutex chunkMutex;
  QMap<int, efdl::Range> connsMap; // num -> download progress
};

#endif // EFDL_DOWNLOAD_MANAGER_H
