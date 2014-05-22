#include <QDebug>
#include <QMutexLocker>
#include <QCoreApplication>

#include <sstream>
#include <iostream>

#include "DownloadManager.h"

#include "Util.h"
#include "Downloader.h"
using namespace efdl;

DownloadManager::DownloadManager(bool connProg)
  : connProg{connProg}, chksum{false}, conns{0}, chunksAmount{0},
  chunksFinished{0}, size{0}, offset{0}, bytesDown{0},
  hashAlg{QCryptographicHash::Sha3_512}, downloader{nullptr}
{ }

DownloadManager::~DownloadManager() {
  cleanup();
  qDeleteAll(queue);
}

void DownloadManager::add(Downloader *entry) {
  queue << entry;
}

void DownloadManager::createChecksum(QCryptographicHash::Algorithm hashAlg) {
  this->hashAlg = hashAlg;
  chksum = true;
}

void DownloadManager::start() {
  next();
}

void DownloadManager::next() {
  static bool first{true};
  if (!first) {
    // Update progress with total download time with no connection
    // lines.
    bool tmp{connProg};
    connProg = false;
    lastProgress = QDateTime(); // Force update.
    updateProgress();
    connProg = tmp;

    if (chksum) printChecksum();

    // Separate each download with a newline.
    if (!queue.isEmpty()) qDebug();
  }
  if (first) first = false;
  
  if (queue.isEmpty()) {
    emit finished();
    return;
  }

  cleanup();

  downloader = queue.dequeue();
  qDebug() << "Downloading" << qPrintable(downloader->getUrl().toString());
  connect(downloader, &Downloader::finished, this, &DownloadManager::next);
  connect(downloader, &Downloader::information,
          this, &DownloadManager::onInformation);
  connect(downloader, &Downloader::chunkStarted,
          this, &DownloadManager::onChunkStarted);
  connect(downloader, &Downloader::chunkProgress,
          this, &DownloadManager::onChunkProgress);
  connect(downloader, &Downloader::chunkFinished,
          this, &DownloadManager::onChunkFinished);
  connect(downloader, &Downloader::chunkFailed,
          this, &DownloadManager::onChunkFailed);
  downloader->start();

  started = QDateTime::currentDateTime();
}
void DownloadManager::onInformation(const QString &outputPath, qint64 size,
                                    int chunksAmount, int conns,
                                    qint64 offset) {
  this->outputPath = outputPath;
  this->size = size;
  this->chunksAmount = chunksAmount;
  this->conns = conns;
  this->offset = offset;
  updateProgress();
}

void DownloadManager::onChunkStarted(int num) {
  QMutexLocker locker{&chunkMutex};
  connsMap[num] = Range{0, 0};
  updateConnsMap();
  updateProgress();
}

void DownloadManager::onChunkProgress(int num, qint64 received, qint64 total) {
  QMutexLocker locker{&chunkMutex};
  const Range &r = connsMap[num];
  bytesDown += received - r.first;
  connsMap[num] = Range{received, total};
  updateConnsMap();
  updateProgress();
}

void DownloadManager::onChunkFinished(int num, Range range) {
  QMutexLocker locker{&chunkMutex};
  chunksFinished++;
  updateConnsMap();
  updateProgress();
}

void DownloadManager::onChunkFailed(int num, Range range, int httpCode,
                                    QNetworkReply::NetworkError error) {
  QMutexLocker locker{&chunkMutex};
  qCritical() << "Chunk" << num << "failed on range" << range;
  qCritical() << "HTTP code:" << httpCode;
  qCritical() << "Error:" << qPrintable(Util::getErrorString(error));
  qCritical() << "Aborting..";

  cleanup();
  QCoreApplication::exit(-1);
}

void DownloadManager::cleanup() {
  chunksAmount = chunksFinished = size = offset = bytesDown = 0;
  connsMap.clear();

  if (downloader) {
    downloader->disconnect();
    downloader->deleteLater();
    downloader = nullptr;
  }
}

void DownloadManager::updateConnsMap() {
  if (connsMap.size() <= conns) {
    return;
  }

  bool rem = false;
  foreach (const auto &key, connsMap.keys()) {
    const auto &value = connsMap[key];
    if (value.first > 0 && value.first == value.second) {
      rem = true;
      connsMap.remove(key);
      break;
    }
  }
  if (!rem) {
    connsMap.remove(connsMap.firstKey());
  }

  if (connsMap.size() > conns) {
    updateConnsMap();
  }
}

void DownloadManager::updateProgress() {
  // Only update progress every half second.
  QDateTime now{QDateTime::currentDateTime()};
  if (!lastProgress.isNull() && lastProgress.msecsTo(now) < 500) {
    return;
  }
  lastProgress = now;

  using namespace std;
  stringstream sstream;

  // Set fixed float formatting to one decimal digit.
  sstream.precision(1);
  sstream.setf(ios::fixed, ios::floatfield);

  sstream << "[ ";

  if (bytesDown == 0) {
    sstream << "Starting up download..";
  }
  else {
    bool done = (bytesDown + offset == size);
    qint64 secs{started.secsTo(now)}, bytesPrSec{0}, secsLeft{0};
    if (secs > 0) {
      bytesPrSec = bytesDown / secs;
      if (bytesPrSec > 0) {
        secsLeft = (!done ? (size - bytesDown - offset) / bytesPrSec
                    : secs);
      }
    }

    float perc = (long double)(bytesDown + offset) / (long double)size * 100.0;

    sstream << perc << "% | "
            << Util::formatSize(bytesDown + offset, 1).toStdString() << " / "
            << Util::formatSize(size, 1).toStdString() << " @ "
            << Util::formatSize(bytesPrSec, 1).toStdString() << "/s | "
            << Util::formatTime(secsLeft).toStdString() << " "
            << (!done ? "left" : "total") << " | "
            << "chunk " << chunksFinished << " / " << chunksAmount;
  }

  sstream << " ]";

  if (connProg) {
    foreach (const int &num, connsMap.keys()) {
      const auto &value = connsMap[num];
      qint64 received = value.first, total = value.second;
      float perc{0};
      if (received > 0 && total > 0) {
        perc = (long double) received / (long double) total * 100.0;
      }
      sstream << "\n{ chunk #" << num << ": " << perc << "% ";
      if (total > 0) {
        sstream << "| " << Util::formatSize(received, 1).toStdString() << " / "
                << Util::formatSize(total, 1).toStdString() << " ";
      }
      sstream << "}";
    }
  }

  sstream << '\n';

  static int lastLines{0};
  string msg{sstream.str()};

  // Remove additional lines, if any.
  for (int i = 0; i < lastLines; i++) {
    cout << "\033[A" // Go up a line (\033 = ESC, [ = CTRL).
         << "\033[2K"; // Clear line.
  }

  // Rewind to beginning with carriage return and write actual
  // message.
  cout << '\r' << msg;
  cout.flush();

  lastLines = QString(msg.c_str()).split("\n").size() - 1;
}

void DownloadManager::printChecksum() {
  QCryptographicHash hasher{hashAlg};
  QFile file{outputPath};
  if (!file.open(QIODevice::ReadOnly)) {
    qCritical() << "ERROR Checksum generation failed: could not open output"
                << "file for reading.";
     return;
  }
  if (!hasher.addData(&file)) {
    qCritical() << "ERROR Failed to do checksum of file.";
     return;
  }
  qDebug() << "Checksum:" << qPrintable(hasher.result().toHex());
}
