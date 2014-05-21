#ifndef EFDL_RANGE_H
#define EFDL_RANGE_H

#include <QPair>
#include <QDebug>

namespace efdl {
  /**
   * Encapsulates a file data range of the form [start, end[
   *
   * Note that "end" is not inclusive!
   */
  typedef QPair<qint64, qint64> Range;
}

QDebug operator<<(QDebug dbg, const efdl::Range &r);

#endif // EFDL_RANGE_H
