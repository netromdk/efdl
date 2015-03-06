#include <QDebug>
#include <QMutexLocker>

#include "ThreadPool.h"
#include "DownloadTask.h"

BEGIN_NAMESPACE

ThreadPool::ThreadPool() : maxCount(1) {
  
}

ThreadPool::~ThreadPool() {
  
}

void ThreadPool::start(DownloadTask *task) {
  {
    QMutexLocker locker(&taskMutex);
    tasks.enqueue(task);
  }
  startTask();
}

void ThreadPool::stop() {
  {
    QMutexLocker locker(&taskMutex);
    tasks.clear();
  }

  {
    QMutexLocker locker(&runMutex);
    foreach (auto *task, running) {
      task->requestInterruption();
    }
    foreach (auto *task, running) {
      if (task->isRunning()) {
        task->wait();
      }
      task->deleteLater();
    }
    running.clear();
  }
}

void ThreadPool::onTaskFinished() {
  auto *task = qobject_cast<DownloadTask*>(sender());
  if (!task) return;

  {
    QMutexLocker locker(&runMutex);
    running.removeAll(task);
  }
  startTask();
}

void ThreadPool::startTask() {
  {
    QMutexLocker locker(&runMutex);
    if (running.size() >= maxCount) {
      return;
    }
  }

  DownloadTask *task = nullptr;
  {
    QMutexLocker locker(&taskMutex);
    if (!tasks.isEmpty()) {
      task = tasks.dequeue();
    }
  }
  if (!task) return;

  {
    QMutexLocker locker(&runMutex);
    connect(task, &QThread::finished, this, &ThreadPool::onTaskFinished);
    running << task;
  }
  task->start();
}

END_NAMESPACE
