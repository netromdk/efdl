#include <QUrl>
#include <QDebug>
#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCoreApplication>

#include "Downloader.h"

Downloader::Downloader(const QUrl &url) : url{url} {

}

void Downloader::start() {
  // Fetch HEAD to find out the size but also if it exists.

  emit finished();
}
