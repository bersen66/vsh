#pragma once

#include <vector>
#include <vsh/key_iterator_fwd.hpp>
#include <DataSketches/quantiles_sketch.hpp>

namespace vsh {

using HistType = std::vector<double>;

struct HistBuilder {
    virtual ~HistBuilder() = default;
    virtual void HandleIteration(KeyIterator&, TypeAdapter&) = 0;
    virtual HistType Build() = 0;
};

class QuantileHistBuilder final : public HistBuilder {
public:
    static constexpr std::size_t kPrecision = 4096;
public:
    explicit QuantileHistBuilder(std::size_t buckets_num);
    void HandleIteration(KeyIterator&, TypeAdapter&) override;
    void Merge(QuantileHistBuilder&& other);
    HistType Build() override;
private:
    datasketches::quantiles_sketch<double> quantiles_sketch_;
    std::size_t buckets_num_;
};


[[nodiscard]] HistType MakeEquiDepthHistogram(HistBuilder& builder, KeyIterator& keys_range);

} // namespace vsh
