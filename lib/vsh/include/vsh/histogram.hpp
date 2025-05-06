#pragma once

#include <arrow/type_fwd.h>
#include <map>
#include <memory>

namespace vsh {

using float128 = long double;
static_assert(sizeof(long double) == 16);

struct Bucket {
    bool isHeavyHitter;
};


using Histogram = std::map<float128, std::shared_ptr<Bucket>>;

class KeyIterator;

[[nodiscard]]Histogram MakeEquiDepthHistogram(KeyIterator& keys_range);

} // namespace vsh
