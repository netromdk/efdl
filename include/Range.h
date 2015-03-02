#ifndef EFDL_RANGE_H
#define EFDL_RANGE_H

#include <QPair>
#include <QDebug>

#include "EfdlGlobal.h"

BEGIN_NAMESPACE

/**
 * Encapsulates a file data range of the form [start, end[
 *
 * Note that "end" is not inclusive!
 */
typedef QPair<qint64, qint64> Range;

END_NAMESPACE

QDebug operator<<(QDebug dbg, const EFDL_NAMESPACE::Range &r);

#endif // EFDL_RANGE_H
