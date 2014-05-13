#include <QObject>

#include "Util.h"

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
