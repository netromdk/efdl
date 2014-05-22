#include "Range.h"

QDebug operator<<(QDebug dbg, const efdl::Range &r) {
  dbg.nospace() << "[" << r.first << ", " << r.second << "]";
  return dbg.space();
}
