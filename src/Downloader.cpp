#include <QUrl>
#include <QDebug>
#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCoreApplication>

#include "Util.h"
#include "Downloader.h"

Downloader::Downloader(const QUrl &url) : url{url} {

}

void Downloader::start() {
  // Fetch HEAD to find out the size but also if it exists.
  auto *rep = getHead(url);
  if (!rep) {
    QCoreApplication::exit(-1);
  }
  //qDebug() << rep;

  emit finished();
}

QNetworkReply *Downloader::getHead(const QUrl &url) {
  qDebug() << "HEAD" << qPrintable(url.toString());

  QNetworkRequest req{url};
  auto *rep = netmgr.head(req);
  QEventLoop loop;
  connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (rep->error() != QNetworkReply::NoError) {
    qCritical() << "ERROR" << qPrintable(Util::getErrorString(rep->error()));
    return nullptr;
  }  

  int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  qDebug() << "CODE" << code;
  //qDebug() << rep->rawHeaderPairs();

  // Handle redirect.
  if (code >= 300 && code < 400) {
    QVariant res = rep->header(QNetworkRequest::LocationHeader);
    if (!res.isValid()) {
      qCritical() << "Could not resolve URL!";
      return nullptr;
    }    
    QUrl loc{res.toString(), QUrl::StrictMode};
    if (!loc.isValid()) {
      qCritical() << "Invalid redirection header:" <<
        qPrintable(loc.toString());
      return nullptr;
    }
    qDebug() << "REDIRECT" << qPrintable(loc.toString());
    rep->abort();
    rep = getHead(loc);
  }

  // Client errors.
  else if (code >= 400 && code < 500) {
    qDebug() << "CLIENT ERROR";
    return nullptr;
  }

  // Server errors.
  else if (code >= 500 && code < 600) {
    qDebug() << "SERVER ERROR";
    return nullptr;
  }

  return rep;
}
