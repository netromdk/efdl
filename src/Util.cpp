#include <QObject>
#include <QTextStream>

#include <iostream>
#ifdef WIN32
  #include <io.h> // _isatty()
  #define isATty _isatty
#else
  #include <unistd.h> // isatty()
  #define isATty isatty
#endif

#include "Util.h"
#include "Range.h"

bool Util::isStdinPipe() {
  return !isATty(fileno(stdin));
}

void Util::registerCustomTypes() {
  qRegisterMetaType<Range>("Range");
}

QString Util::getErrorString(QNetworkReply::NetworkError error) {
  switch (error) {
  case QNetworkReply::ConnectionRefusedError:
    return QObject::tr("Remote server refused the connection.");

  case QNetworkReply::RemoteHostClosedError:
    return QObject::tr("Remote server closed connection prematurely.");

  case QNetworkReply::HostNotFoundError:
    return QObject::tr("Remote host was not found.");

  case QNetworkReply::TimeoutError:
    return QObject::tr("Connection to remote server timed out.");

  case QNetworkReply::OperationCanceledError:
    return QObject::tr("Operation was canceled before it finished.");

  case QNetworkReply::SslHandshakeFailedError:
    return QObject::tr("SSL/TLS handshake failed and encrypted channel could not be established.");

  case QNetworkReply::TemporaryNetworkFailureError:
    return QObject::tr("Connection was broken due to disconnection from network, however the system has initiated roaming to another access point. The request should be resubmitted and processed when connection is re-established.");

  case QNetworkReply::NetworkSessionFailedError:
    return QObject::tr("Connection was broken due to disconnection from network or failure to start the network.");

  case QNetworkReply::BackgroundRequestNotAllowedError:
    return QObject::tr("Background request not allowed due to platform policy.");

  case QNetworkReply::ProxyConnectionRefusedError:
    return QObject::tr("Connection to proxy server was refused.");

  case QNetworkReply::ProxyConnectionClosedError:
    return QObject::tr("Proxy server closed connection prematurely.");

  case QNetworkReply::ProxyNotFoundError:
    return QObject::tr("Proxy host name was not found.");

  case QNetworkReply::ProxyTimeoutError:
    return QObject::tr("Connection to the proxy timed out.");

  case QNetworkReply::ProxyAuthenticationRequiredError:
    return QObject::tr("Proxy requires authentication in order to honour the request but did not accept any credentials offered (if any).");
    
  case QNetworkReply::ContentAccessDenied:
    return QObject::tr("Access to remote content denied.");

  case QNetworkReply::ContentOperationNotPermittedError:
    return QObject::tr("Operation not permitted on remote server.");

  case QNetworkReply::ContentNotFoundError:
    return QObject::tr("Remote content not found.");

  case QNetworkReply::AuthenticationRequiredError:
    return QObject::tr("Remote server requires authentication to serve the content but the credentials provided were not accepted (if any).");

  case QNetworkReply::ContentReSendError:
    return QObject::tr("Request needed to be sent again, but this failed.");

  case QNetworkReply::ProtocolUnknownError:
    return QObject::tr("Protocol unknown so request cannot be honoured.");

  case QNetworkReply::ProtocolInvalidOperationError:
    return QObject::tr("Requested operation is invalid for this protocol.");

  case QNetworkReply::UnknownNetworkError:
    return QObject::tr("Unknown network-related error.");

  case QNetworkReply::UnknownProxyError:
    return QObject::tr("Unknown proxy-related error.");

  case QNetworkReply::UnknownContentError:
    return QObject::tr("Unknown error related to the remote content.");

  case QNetworkReply::ProtocolFailure:
    return QObject::tr("A breakdown in protocol was detected.");

  default:
    return QString::number(error);
  }
}

bool Util::askProceed(const QString &msg) {
  using namespace std;
  cout << msg.toStdString().c_str();
  cout.flush();

  QTextStream stream(stdin);
  QString output = stream.readLine().toLower().trimmed();
  return output == "y" || output == "yes";
}

QString Util::formatSize(qint64 bytes, float digits) {
  constexpr double KB = 1024, MB = 1024 * KB, GB = 1024 * MB, TB = 1024 * GB;
  QString unit{"B"};
  double size = bytes;
  if (size >= TB) {
    size /= TB;
    unit = "TB";
  }
  else if (size >= GB) {
    size /= GB;
    unit = "GB";
  }
  else if (size >= MB) {
    size /= MB;
    unit = "MB";
  }
  else if (size >= KB) {
    size /= KB;
    unit = "KB";
  }
  return QString("%1 %2").arg(QString::number(size, 'f', digits)).arg(unit);
}

QString Util::formatTime(qint64 secs) {
  constexpr qint64 Minute = 60, Hour = Minute * 60, Day = Hour * 24,
    Week = Day * 7;
  QString res;
  if (secs >= Week) {
    qint64 weeks = secs / Week;
    res += QString("%1w").arg(weeks);
    secs -= weeks * Week;
  }
  if (secs >= Day) {
    qint64 days = secs / Day;
    res += QString("%1%2d").arg(!res.isEmpty() ? " " : "").arg(days);
    secs -= days * Day;
  }
  if (secs >= Hour) {
    qint64 hours = secs / Hour;
    res += QString("%1%2h").arg(!res.isEmpty() ? " " : "").arg(hours);
    secs -= hours * Hour;
  }
  if (secs >= Minute) {
    qint64 minutes = secs / Minute;
    res += QString("%1%2m").arg(!res.isEmpty() ? " " : "").arg(minutes);
    secs -= minutes * Minute;
  }
  if (secs > 0 || res.isEmpty()) {
    res += QString("%1%2s").arg(!res.isEmpty() ? " " : "").arg(secs);
  }
  return res;
}

bool Util::stringToHashAlg(QString str, QCryptographicHash::Algorithm &alg) {
  str = str.trimmed().toLower();
  if (str == "md4") {
    alg = QCryptographicHash::Md4;
  }
  else if (str == "md5") {
    alg = QCryptographicHash::Md5;
  }
  else if (str == "sha1") {
    alg = QCryptographicHash::Sha1;
  }
  else if (str == "sha2-224") {
    alg = QCryptographicHash::Sha224;
  }
  else if (str == "sha2-256") {
    alg = QCryptographicHash::Sha256;
  }
  else if (str == "sha2-384") {
    alg = QCryptographicHash::Sha384;
  }
  else if (str == "sha2-512") {
    alg = QCryptographicHash::Sha512;
  }
  else if (str == "sha3-224") {
    alg = QCryptographicHash::Sha3_224;
  }
  else if (str == "sha3-256") {
    alg = QCryptographicHash::Sha3_256;
  }
  else if (str == "sha3-384") {
    alg = QCryptographicHash::Sha3_384;
  }
  else if (str == "sha3-512") {
    alg = QCryptographicHash::Sha3_512;
  }
  else {
    return false;
  }
  return true;
}
