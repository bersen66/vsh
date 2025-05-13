#pragma once

#include <cstdint>
#include <vsh/histogram.hpp>
#include <vsh/key_iterator_fwd.hpp>
#include <vsh/eh_sketch.hpp>

#include <map>

namespace vsh {

class BarSplittingHistBuilder : public HistBuilder {
public:
    static constexpr double kMaxCoef = 1.7f;
public:

    struct Bar {
        EHSketch eh; 
        long double interval_min;
        long double interval_max;
        bool is_blocked = false;
    };

    using BarList = std::list<Bar>;
    using BarIter = BarList::iterator;
    using BarSearchMap = std::map<double, BarIter>;

    [[nodiscard]]std::size_t AggregateSize(BarIter it) const;
    
    [[nodiscard]]bool NeedSplit(BarIter it, std::size_t max_size) const ;

    [[nodiscard]]bool EmptyBar(BarIter it) const;
public:
    
    explicit BarSplittingHistBuilder(std::uint64_t buckets_num, 
                                     double scaling_factor = 4.f,
                                     std::uint64_t eh_sketch_precision = 100,
                                     std::uint64_t window_size = -1);

    BarSplittingHistBuilder(BarSplittingHistBuilder&&) noexcept = default;
    BarSplittingHistBuilder& operator=(BarSplittingHistBuilder&&) noexcept = default;
    BarSplittingHistBuilder(const BarSplittingHistBuilder&) = delete;
    BarSplittingHistBuilder& operator=(const BarSplittingHistBuilder&) = delete;
    virtual ~BarSplittingHistBuilder() = default;

    void HandleIteration(KeyIterator&, TypeAdapter&) override;

    HistType Build() override;

protected:

    void InsertIntoBar(double value);

    BarIter FindOrCreateBarFor(double value);

    void SplitBar(BarIter it, long double Sm, std::size_t max_bar_size);

    bool FindAndMergeBars(std::size_t max_bar_size);

    void Tick();

    std::pair<BarIter, BarIter> FindAdjacentBarsToMerge(std::size_t max_bar_size);

    bool FindAdjacentBothEmpty(BarIter& out1, BarIter& out2) const;

    bool FindAdjacentOneEmpty(BarIter& out1, BarIter& out2) const;

    bool FindTwoAdjacentLowestAgSize(std::size_t max_bar_size, BarIter& out1, BarIter& out2) const;

    std::size_t NonBlockedBarsCount() const {
        return search_map_.size();
    }
protected:
    std::list<Bar> bars_;
    std::map<double, BarIter> search_map_;
    std::uint64_t eh_sketch_precision_;
    std::uint64_t window_size_;
    std::uint64_t elements_visited_;
    std::uint64_t buckets_num_;
    double scaling_factor_;
};

} // namespace vsh
