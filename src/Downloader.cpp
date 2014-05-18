#include <sstream>
#include <iostream>

#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QEventLoop>
#include <QMutexLocker>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QCoreApplication>

#include "Util.h"
#include "Downloader.h"
#include "DownloadTask.h"

Downloader::Downloader(const QUrl &url, const QString &outputDir, int conns,
                       int chunks, int chunkSize, bool confirm, bool resume,
                       bool connProg, bool verbose)
  : url{url}, outputDir{outputDir}, conns{conns}, chunks{chunks},
  chunkSize{chunkSize}, downloadCount{0}, rangeCount{0}, contentLen{0},
  offset{0}, bytesDown{0}, confirm{confirm}, resume{resume}, connProg{connProg},
  verbose{verbose}, resumable{false}, chksum{false},
  hashAlg{QCryptographicHash::Sha3_512}, reply{nullptr}
{
  connect(&commitThread, &CommitThread::finished,
          this, &Downloader::onCommitThreadFinished);
  connect(this, &Downloader::chunkToThread,
          &commitThread, &CommitThread::enqueueChunk,
          Qt::QueuedConnection);
}

void Downloader::createChecksum(QCryptographicHash::Algorithm hashAlg) {
  this->hashAlg = hashAlg;
  chksum = true;
}

void Downloader::start() {
  // Fetch HEAD to find out the size but also if it exists.
  reply = getHead(url);
  if (!reply) {
    QCoreApplication::exit(-1);
    return;
  }
  url = reply->url();

  // Find "Content-Length".
  if (!reply->hasRawHeader("Content-Length")) {
    qCritical() << "ERROR Invalid response from server. No"
                << "'Content-Length' header present!";
    QCoreApplication::exit(-1);
    return;
  }
  bool ok;
  contentLen =
    QString::fromUtf8(reply->rawHeader("Content-Length")).toLongLong(&ok);
  if (!ok || contentLen == 0) {
    qCritical() << "ERROR Invalid content length:" << contentLen;
    QCoreApplication::exit(-1);
    return;    
  }
  qDebug() << "File size" << qPrintable(Util::formatSize(contentLen, 1));

  // Check for header "Accept-Ranges" and whether it has "bytes"
  // supported.
  resumable = false;
  if (reply->hasRawHeader("Accept-Ranges")) {
    const QString ranges = QString::fromUtf8(reply->rawHeader("Accept-Ranges"));
    resumable = ranges.toLower().contains("bytes");
  }
  if (verbose) {
    qDebug() << qPrintable(QString("%1RESUMABLE").
                           arg(resumable ? "" : "NOT "));
  }

  if (!resumable && resume) {
    qCritical() << "ERROR Cannot resume because server doesn't support it!";
    QCoreApplication::exit(-1);
    return;
  }

  // Clean reply.
  reply->close();
  reply = nullptr;

  if (!setupFile()) {
    QCoreApplication::exit(-1);
    return;
  }

  createRanges();
  setupThreadPool();

  // Start actual download.
  download();
}

void Downloader::onDownloadTaskStarted(int num) {
  QMutexLocker locker{&finishedMutex};
  connsMap[num] = Range{0, 0};
  updateConnsMap();
  updateProgress();
}

void Downloader::onDownloadTaskProgress(int num, qint64 received, qint64 total) {
  QMutexLocker locker{&finishedMutex};
  connsMap[num] = Range{received, total};
  updateConnsMap();
  updateProgress();
}

void Downloader::onDownloadTaskFinished(int num, Range range, QByteArray *data) {
  QMutexLocker locker{&finishedMutex};
  chunksMap[range.first] = data;
  updateConnsMap();
  downloadCount++;
  bytesDown += data->size();
  updateProgress();
  saveChunk();
}

void Downloader::onDownloadTaskFailed(int num, Range range, int httpCode,
                                      QNetworkReply::NetworkError error) {
  qDebug() << "failed" << range << Util::getErrorString(error);
}

void Downloader::onCommitThreadFinished() {
  connProg = false;
  updateProgress();
  if (chksum) printChecksum();
  emit finished();
}

QNetworkReply *Downloader::getHead(const QUrl &url) {
  if (verbose) {
    qDebug() << "HEAD" << qPrintable(url.toString());
  }

  QNetworkRequest req{url};
  auto *rep = netmgr.head(req);
  QEventLoop loop;
  connect(rep, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  loop.exec();

  if (rep->error() != QNetworkReply::NoError) {
    rep->abort();
    qCritical() << "ERROR" << qPrintable(Util::getErrorString(rep->error()));
    return nullptr;
  }  

  int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (verbose) {
    qDebug() << "CODE" << code;
    //qDebug() << "HEADERS" << rep->rawHeaderPairs();
  }

  static bool didRedir = false;

  if (code >= 200 && code < 300) {
    if (url != this->url) {
      qDebug() << "Resolved to" << qPrintable(url.toString());
    }

    if (confirm && didRedir) {
      if (!Util::askProceed(tr("Do you want to continue?") + " [y/N] ")) {
        rep->abort();
        qCritical() << "Aborting..";
        return nullptr;
      }
    }
  }

  // Handle redirect.
  if (code >= 300 && code < 400) {
    if (!rep->hasRawHeader("Location")) {
      rep->abort();
      qCritical() << "ERROR Could not resolve URL!";
      return nullptr;
    }    

    QString locHdr = QString::fromUtf8(rep->rawHeader("Location"));

    QUrl loc{locHdr, QUrl::StrictMode};
    if (!loc.isValid()) {
      rep->abort();
      qCritical() << "ERROR Invalid redirection header:" <<
        qPrintable(loc.toString());
      return nullptr;
    }

    // If relative then try resolving with the previous URL.
    if (loc.isRelative()) {
      loc = url.resolved(loc);
    }

    if (verbose) {
      qDebug() << "REDIRECT" << qPrintable(loc.toString());
    }

    didRedir = true;
    rep->abort();
    rep = getHead(loc);
  }

  // Client errors.
  else if (code >= 400 && code < 500) {
    rep->abort();
    qDebug() << "CLIENT ERROR";
    return nullptr;
  }

  // Server errors.
  else if (code >= 500 && code < 600) {
    rep->abort();
    qDebug() << "SERVER ERROR";
    return nullptr;
  }

  return rep;
}

bool Downloader::setupFile() {
  QFileInfo fi{url.path()};
  QDir dir = (outputDir.isEmpty() ? QDir::current() : outputDir);
  outputPath = dir.absoluteFilePath(fi.fileName());
  qDebug() << "Saving to" << qPrintable(outputPath);

  auto *file = new QFile{outputPath};
  if (file->exists() && !resume) {
    if (!QFile::remove(outputPath)) {
      qCritical() << "ERROR Could not truncate output file!";
      delete file;
      return false;
    }
  }

  if (resume) {
    qint64 fileSize{file->size()};
    if (fileSize >= contentLen) {
      qCritical() << "Cannot resume download because the size is larger than or"
                  << "equal:" << qPrintable(Util::formatSize(fileSize, 1))
                  << "vs." << qPrintable(Util::formatSize(contentLen, 1));
      if (confirm &&
          !Util::askProceed("Do you want to truncate file and continue? [y/N] ")) {
        qCritical() << "Aborting..";
        delete file;
        return false;
      }
      if (!confirm) {
        qDebug() << "Truncating file";
      }
      resume = false;
    }
    else if (fileSize > 0) {
      offset = fileSize;
      float perc = (long double)offset / (long double)contentLen * 100.0;
      qDebug() << "Resuming at offset"
               << qPrintable(Util::formatSize(offset, 1))
               << qPrintable(QString("(%1%)").arg(perc, 0, 'f', 1));
    }
  }

  QIODevice::OpenMode openMode{QIODevice::WriteOnly};
  if (resume) {
    openMode |= QIODevice::Append;
  }
  else {
    openMode |= QIODevice::Truncate;
  }

  if (!file->open(openMode)) {
    qCritical() << "ERROR Could not open file for writing!";
    delete file;
    return false;
  }
  commitThread.setFile(file);

  return true;
}

void Downloader::createRanges() {
  ranges.clear();
  chunksMap.clear();

  qint64 size = 1048576;
  if (chunkSize != -1) {
    size = chunkSize;
  }
  else if (chunks != -1) {
    size = (contentLen - offset) / chunks;
  }
  else if (conns >= 8) {
    size = (contentLen - offset) / conns;
    constexpr qint64 MB{10485760}; // 10 MB
    if (size > MB) size = MB;
  }

  if (verbose) {
    qDebug() << "CHUNK SIZE" << qPrintable(Util::formatSize(size, 1));
  }

  for (qint64 start = offset; start < contentLen; start += size) {
    qint64 end = start + size;
    if (end >= contentLen) {
      end = contentLen;
    }
    ranges.enqueue(Range{start, end - 1});
    chunksMap[start] = nullptr;
  }
  rangeCount = ranges.size();

  if (verbose) {
    qDebug() << "CHUNKS" << rangeCount;
  }
}

void Downloader::setupThreadPool() {
  // Cap connections to the amount of chunks to download.
  if (conns > ranges.size()) {
    int old{conns};
    conns = ranges.size();
    qDebug() << "CAP connection amount capped to amount of chunks:"
             << old << "->" << conns;
  }

  pool.setMaxThreadCount(conns);
}

void Downloader::download() {
  // Fill queue with tasks and start immediately.
  started = QDateTime::currentDateTime();
  updateProgress();
  int num{1};
  while (!ranges.empty()) {
    auto range = ranges.dequeue();
    auto *task = new DownloadTask{url, range, num++};
    connect(task, &DownloadTask::started,
            this, &Downloader::onDownloadTaskStarted);
    connect(task, &DownloadTask::progress,
            this, &Downloader::onDownloadTaskProgress);
    connect(task, &DownloadTask::finished,
            this, &Downloader::onDownloadTaskFinished);
    connect(task, &DownloadTask::failed,
            this, &Downloader::onDownloadTaskFailed);
    pool.start(task);
  }
}

void Downloader::saveChunk() {
  // Lock on chunks map is required to be acquired going into this
  // method!

  if (chunksMap.isEmpty()) {
    // Wait for commit thread to be done.
    return;
  }

  auto key = chunksMap.firstKey();
  const auto *value = chunksMap[key];
  if (value != nullptr) {
    emit chunkToThread(value, chunksMap.size() == 1);
    if (!commitThread.isRunning()) {
      commitThread.start();
    }
    chunksMap.remove(key);
  }

  // If everything has been downloaded then call method again.
  if (rangeCount == downloadCount) {
    saveChunk();
  }
}

void Downloader::updateConnsMap() {
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

void Downloader::updateProgress() {
  // Lock on chunks map is required to be acquired going into this
  // method!

  using namespace std;
  stringstream sstream;

  // Set fixed float formatting to one decimal digit.
  sstream.precision(1);
  sstream.setf(ios::fixed, ios::floatfield);

  sstream << "[ ";

  if (bytesDown == 0) {
    sstream << "Downloading.. Awaiting first chunk";
  }
  else {
    bool done = (bytesDown + offset == contentLen);
    QDateTime now{QDateTime::currentDateTime()};
    qint64 secs{started.secsTo(now)}, bytesPrSec{0}, secsLeft{0};
    if (secs > 0) {
      bytesPrSec = bytesDown / secs;
      secsLeft = (!done ? (contentLen - bytesDown - offset) / bytesPrSec
                  : secs);
    }

    float perc = (long double)(bytesDown + offset) / (long double)contentLen * 100.0;

    sstream << perc << "% | "
            << Util::formatSize(bytesDown + offset, 1).toStdString() << " / "
            << Util::formatSize(contentLen, 1).toStdString() << " @ "
            << Util::formatSize(bytesPrSec, 1).toStdString() << "/s | "
            << Util::formatTime(secsLeft).toStdString() << " "
            << (!done ? "left" : "total") << " | "
            << "chunk " << downloadCount << " / " << rangeCount;
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

void Downloader::printChecksum() {
  QCryptographicHash hasher{hashAlg};
  QFile file{outputPath};
  if (!file.open(QIODevice::ReadOnly)) {
    qCritical() << "ERROR Checksum generation failed: could not open output"
                << "file for reading.";
    QCoreApplication::exit(-1);
    return;
  }
  if (!hasher.addData(&file)) {
    qCritical() << "ERROR Failed to do checksum of file.";
    QCoreApplication::exit(-1);
    return;
  }
  qDebug() << "Checksum:" << qPrintable(hasher.result().toHex());
}
