#ifndef EFDL_DOWNLOADER_H
#define EFDL_DOWNLOADER_H

#include <QUrl>
#include <QPair>
#include <QQueue>
#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>

class QUrl;
class QNetworkReply;

class Downloader : public QObject {
  Q_OBJECT

  typedef QPair<qint64, qint64> Range; // (start, end)
  
public:
  Downloader(const QUrl &url);

signals: 
  void finished();
    
public slots:
  void start();
  
private:
  QNetworkReply *getHead(const QUrl &url);
  void createRanges();
  void download();
  bool getChunk(qint64 start, qint64 end);
  
  QUrl url;
  qint64 contentLen;
  bool continuable;

  QNetworkAccessManager netmgr;
  QNetworkReply *reply;

  QByteArray data;//temp

  QQueue<Range> ranges;
};

#endif // EFDL_DOWNLOADER_H
