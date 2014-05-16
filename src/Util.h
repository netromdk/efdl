#ifndef EFDL_UTIL_H
#define EFDL_UTIL_H

#include <QString>
#include <QNetworkReply>

class Util {
public:
  static void registerCustomTypes();
  static QString getErrorString(QNetworkReply::NetworkError error);
  static bool askProcess(const QString &msg);
};

#endif // EFDL_UTIL_H
