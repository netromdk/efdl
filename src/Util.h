#ifndef EFDL_UTIL_H
#define EFDL_UTIL_H

#include <QString>
#include <QNetworkReply>

class Util {
public:
  static void registerCustomTypes();
  static QString getErrorString(QNetworkReply::NetworkError error);
  static bool askProceed(const QString &msg);
  static QString formatSize(qint64 bytes, float digits = 2);
  static QString formatTime(qint64 secs);
};

#endif // EFDL_UTIL_H
