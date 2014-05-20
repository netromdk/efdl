#ifndef EFDL_UTIL_H
#define EFDL_UTIL_H

#include <QString>
#include <QNetworkReply>
#include <QCryptographicHash>

namespace efdl {
  class Util {
  public:
    static bool isStdinPipe();
    static void registerCustomTypes();
    static QString getErrorString(QNetworkReply::NetworkError error);
    static bool askProceed(const QString &msg);
    static QString formatSize(qint64 bytes, float digits = 2);
    static QString formatTime(qint64 secs);
    static bool stringToHashAlg(QString str, QCryptographicHash::Algorithm &alg);
    static QString formatHeaders(const QList<QNetworkReply::RawHeaderPair> &hdrs);
  };
}

#endif // EFDL_UTIL_H
