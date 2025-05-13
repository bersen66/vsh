#include <vsh/histogram.hpp>
#include <vsh/key_iterator.hpp>

namespace vsh {

HistType MakeEquiDepthHistogram(HistBuilder& builder, KeyIterator& key_range) {
    for(auto conv = key_range.ValuesAdapter(); key_range.HasNext(); key_range.StepForward()) {
        builder.HandleIteration(key_range, *conv);
    }
    return builder.Build();
}

} // namespace vsh
