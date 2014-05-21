#ifndef EFDL_RANGE_H
#define EFDL_RANGE_H

#include <QPair>
#include <QDebug>

namespace efdl {
  typedef QPair<qint64, qint64> Range; // (start, end)
}

QDebug operator<<(QDebug dbg, const efdl::Range &r);

#endif // EFDL_RANGE_H
