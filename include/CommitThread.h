#ifndef EFDL_COMMIT_THREAD_H
#define EFDL_COMMIT_THREAD_H

#include <QFile>
#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QString>

#include "EfdlGlobal.h"

BEGIN_NAMESPACE

class CommitThread : public QThread {
  Q_OBJECT
  
public:
  CommitThread();
  ~CommitThread();

  // Takes ownership.
  void setFile(QFile *file) { this->file = file; }

public slots:
  void enqueueChunk(const QByteArray *data, bool last = false);

private:
  void run() override;
  void cleanup();
  
  QFile *file;
  bool last;
  QQueue<const QByteArray*> queue;
  QMutex queueMutex;
};

END_NAMESPACE

#endif // EFDL_COMMIT_THREAD_H
