#include <vsh/histogram.hpp>
#include <vsh/key_iterator.hpp>

namespace vsh {

QuantileHistBuilder::QuantileHistBuilder(std::size_t buckets_num)
    : quantiles_sketch_(kPrecision)
    , buckets_num_(buckets_num)
{}

void QuantileHistBuilder::HandleIteration(KeyIterator& iter, TypeAdapter& conv) {
    quantiles_sketch_.update(conv.AsDouble(iter.Value()));
}

void QuantileHistBuilder::Merge(QuantileHistBuilder&& other) {
    quantiles_sketch_.merge(other.quantiles_sketch_);
}

HistType QuantileHistBuilder::Build() {
    HistType result;
    result.reserve(buckets_num_);

    for (std::size_t i = 1; i < buckets_num_; i++) {
        double rank = static_cast<double>(i) / static_cast<double>(buckets_num_);
        result.push_back(quantiles_sketch_.get_quantile(rank));
    }
    
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}


HistType MakeEquiDepthHistogram(HistBuilder& builder, KeyIterator& key_range) {
    for(auto conv = key_range.ValuesAdapter(); key_range.HasNext(); key_range.StepForward()) {
        builder.HandleIteration(key_range, *conv);
    } 
    return builder.Build();
}

} // namespace vsh
