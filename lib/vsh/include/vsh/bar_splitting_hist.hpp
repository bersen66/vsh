#pragma once

#include <cstdint>
#include <memory_resource>
#include <vsh/histogram.hpp>
#include <vsh/key_iterator_fwd.hpp>
#include <vsh/eh_sketch.hpp>

#include <map>

namespace vsh {

class BarSplittingHistBuilder : public HistBuilder {
public:
    static constexpr double kMaxCoef = 1.7f;
public:

    std::pmr::memory_resource* RecommendedMemoryPool();

    struct Bar {
        EHSketch eh; 
        double interval_min;
        double interval_max;
        bool is_blocked = false;
    };

    using BarList = std::pmr::list<Bar>;
    using BarIter = BarList::iterator;
    using BarSearchMap = std::pmr::map<double, BarIter>;
    using BarIndexIter = BarSearchMap::iterator;

    [[nodiscard]]std::size_t AggregateSize(BarIter it) const;
    
    [[nodiscard]]bool NeedSplit(BarIter it, std::size_t max_size) const;

    [[nodiscard]]bool EmptyBar(BarIter it) const;
public:
    
    explicit BarSplittingHistBuilder(std::pmr::memory_resource* resource,
                                     std::uint64_t buckets_num, 
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

    void SplitBar(BarIter it, double Sm, std::size_t max_bar_size);

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
    std::pmr::memory_resource* mem_resource_;
    BarList bars_;
    BarSearchMap search_map_;
    std::uint64_t eh_sketch_precision_;
    std::uint64_t window_size_;
    std::uint64_t elements_visited_;
    std::uint64_t buckets_num_;
    double scaling_factor_;
    bool is_first_iter_;
};

} // namespace vsh
