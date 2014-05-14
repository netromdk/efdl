#ifndef EFDL_DOWNLOADER_H
#define EFDL_DOWNLOADER_H

#include <QUrl>
#include <QObject>
#include <QByteArray>
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
  void download();
  bool getChunk(qint64 start, qint64 end);
  
  QUrl url;
  qint64 contentLen;
  bool continuable;

  QNetworkAccessManager netmgr;
  QNetworkReply *reply;

  QByteArray data;
};

#endif // EFDL_DOWNLOADER_H
