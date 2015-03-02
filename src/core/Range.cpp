#include "Range.h"
USE_NAMESPACE

QDebug operator<<(QDebug dbg, const Range &r) {
  dbg.nospace() << "[" << r.first << ", " << r.second << "]";
  return dbg.space();
}
