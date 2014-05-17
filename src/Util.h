#ifndef EFDL_UTIL_H
#define EFDL_UTIL_H

#include <QString>
#include <QNetworkReply>

class Util {
public:
  static void registerCustomTypes();
  static QString getErrorString(QNetworkReply::NetworkError error);
  static bool askProceed(const QString &msg);
  static QString sizeToString(qint64 bytes, float digits = 2);
};

#endif // EFDL_UTIL_H
