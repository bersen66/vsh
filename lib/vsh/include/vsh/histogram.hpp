#pragma once

#include <vector>
#include <vsh/key_iterator_fwd.hpp>

namespace vsh {

using HistType = std::vector<double>;

struct HistBuilder {
    virtual ~HistBuilder() = default;
    virtual void HandleIteration(KeyIterator&, TypeAdapter&) = 0;
    virtual HistType Build() = 0;
};

[[nodiscard]] HistType MakeEquiDepthHistogram(HistBuilder& builder, KeyIterator& keys_range);

} // namespace vsh
