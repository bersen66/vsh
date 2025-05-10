#pragma once

#include <vsh/key_iterator_fwd.hpp>
#include <DataSketches/quantiles_sketch.hpp>
#include <vsh/histogram.hpp>

namespace vsh {


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


} // namespace vsh
