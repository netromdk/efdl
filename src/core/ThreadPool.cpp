#include <QDebug>
#include <QMutexLocker>

#include "ThreadPool.h"
#include "DownloadTask.h"

BEGIN_NAMESPACE

TaskThread::TaskThread(DownloadTask *task) : task(task) {
}

void TaskThread::run() {
  qDebug() << "TASK THREAD: running task=" << task;
  task->start();
  qDebug() << "TASK THREAD: finished task=" << task;
}

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
  qDebug() << "POOL: stopping!";

  {
    QMutexLocker locker(&taskMutex);
    tasks.clear();
  }

  {
    QMutexLocker locker(&runMutex);
    foreach (auto *thread, running) {
      thread->requestInterruption();
      thread->deleteLater();
    }
    running.clear();
  }
}

void ThreadPool::onTaskFinished() {
  auto *thread = qobject_cast<TaskThread*>(sender());
  if (!thread) return;

  qDebug() << "onTaskFinished:" << thread;
  {
    QMutexLocker locker(&runMutex);
    running.removeAll(thread);
  }
  startTask();
}

void ThreadPool::startTask() {
  // 1. Take task from queue, if any (lock queue)
  // 2. Create thread for the task and connect finished/error signals locally
  // 3. Add to "running" list
  // 4. Start thread

  {
    QMutexLocker locker(&runMutex);
    if (running.size() >= maxCount) {
      qDebug() << "POOL: have max running tasks:" << running.size();
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
  if (!task) {
    qDebug() << "POOL: no more tasks";
    return;
  }

  {
    QMutexLocker locker(&runMutex);
    auto *thread = new TaskThread(task);
    connect(thread, &QThread::finished, this, &ThreadPool::onTaskFinished);
    running << thread;
    qDebug() << "POOL: starting task=" << task << "on thread=" << thread;
    thread->start();
  }
}

END_NAMESPACE
