#include <QDebug>
#include <QDateTime>

#include "CommitThread.h"

BEGIN_NAMESPACE

CommitThread::CommitThread() : file{nullptr}, last{false} { }

CommitThread::~CommitThread() {
  cleanup();
}

void CommitThread::enqueueChunk(const QByteArray *data, bool last) {
  QMutexLocker locker(&queueMutex);
  this->last = last;
  queue.enqueue(data);
}

void CommitThread::run() {
  QDateTime lastTime{QDateTime::currentDateTime()};
  for (;;) {
    // Only check for interrupt every half second.
    QDateTime now{QDateTime::currentDateTime()};
    if (lastTime.msecsTo(now) > 500) {
      if (isInterruptionRequested()) {
        break;
      }
      lastTime = now;
    }

    const QByteArray *data{nullptr};
    int amountLeft = 0;
    {
      QMutexLocker locker(&queueMutex);
      if (!queue.isEmpty()) {
        data = queue.dequeue();
      }
      amountLeft = queue.size();
    }
    if (!data) {
      msleep(10);
      continue;
    }

    qint64 wrote;
    if ((wrote = file->write(*data)) != data->size()) {
      qCritical() << "ERROR Could not write the entire data:"
                  << wrote << "of" << data->size();
      // TODO: handle this in a different way!
      return;
    }
    delete data;

    if (last && amountLeft == 0) {
      break;
    }
    msleep(10);
  }

  cleanup();
}

void CommitThread::cleanup() {
  if (file) {
    file->close();
    delete file;
    file = nullptr;
  }
}

END_NAMESPACE
