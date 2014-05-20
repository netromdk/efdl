#ifndef EFDL_DOWNLOAD_MANAGER_H
#define EFDL_DOWNLOAD_MANAGER_H

#include <QQueue>
#include <QObject>

namespace efdl {
  class Downloader;
}

class DownloadManager : public QObject {
  Q_OBJECT
  
public:
  ~DownloadManager();

  void add(efdl::Downloader *entry);

signals:
  void finished();
                                         
public slots:
  void start();

private slots:
  void next();

private:
  QQueue<efdl::Downloader*> queue;
};

#endif // EFDL_DOWNLOAD_MANAGER_H
