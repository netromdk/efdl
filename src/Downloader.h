#ifndef EFDL_DOWNLOADER_H
#define EFDL_DOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>

class QUrl;
class QNetworkReply;

class Downloader : public QObject {
  Q_OBJECT
  
public:
  Downloader(const QUrl &url);

signals: 
  void finished();
    
public slots:
  void start();
  
private:
  QNetworkReply *getHead(const QUrl &url);
  
  const QUrl &url;
  QNetworkAccessManager netmgr;
};

#endif // EFDL_DOWNLOADER_H
