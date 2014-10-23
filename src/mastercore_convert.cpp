#include "mastercore_convert.h"

#include <cmath>
#include <stdint.h>

namespace mastercore
{

uint64_t rounduint64(double d)
{
    return (uint64_t)(std::abs(0.5 + d));
}

} // namespace mastercore
