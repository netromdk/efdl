#ifndef EFDL_THREAD_POOL_H
#define EFDL_THREAD_POOL_H

#include <QList>
#include <QMutex>
#include <QQueue>
#include <QObject>

#include "EfdlGlobal.h"

class QThread;

BEGIN_NAMESPACE

class DownloadTask;

class ThreadPool : public QObject {
  Q_OBJECT

public:
  ThreadPool();
  ~ThreadPool();

  void setMaxThreadCount(int max) { maxCount = max; }

public slots:
  void start(DownloadTask *task);
  void stop();

private slots:
  void onTaskFinished();

private:
  void startTask();
  
  int maxCount;
  QQueue<DownloadTask*> tasks;
  QList<DownloadTask*> running;
  QMutex taskMutex, runMutex;
};

END_NAMESPACE

#endif // EFDL_THREAD_POOL_H
