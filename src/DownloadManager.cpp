#include "DownloadManager.h"

#include "Downloader.h"
using namespace efdl;

DownloadManager::~DownloadManager() {
  qDeleteAll(queue);
}

void DownloadManager::add(Downloader *entry) {
  queue << entry;
}

void DownloadManager::start() {
  next();
}

void DownloadManager::next() {
  // Separate each download with a newline.
  static bool first{true};
  if (!first && !queue.isEmpty()) qDebug();
  if (first) first = false;
  
  if (queue.isEmpty()) {
    emit finished();
    return;
  }

  auto *entry = queue.dequeue();
  qDebug() << "Downloading" << qPrintable(entry->getUrl().toString());
  connect(entry, &Downloader::finished, this, &DownloadManager::next);
  entry->start();
}
