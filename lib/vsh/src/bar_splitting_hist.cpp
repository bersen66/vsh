#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <utility>

#include <vsh/eh_sketch.hpp>
#include <vsh/key_iterator.hpp>
#include <vsh/bar_splitting_hist.hpp>

namespace vsh {

using Bar = BarSplittingHistBuilder::Bar;
using BarList = BarSplittingHistBuilder::BarList;
using BarIter = BarSplittingHistBuilder::BarIter;
using BarsMap = BarSplittingHistBuilder::BarSearchMap;

bool BarSplittingHistBuilder::NeedSplit(BarIter it, std::size_t max_size) const {
     return AggregateSize(it) > max_size;
}

void AppendBar(Bar& bar, double value) {
    bar.interval_min = std::min(bar.interval_min, value);
    bar.interval_max = std::max(bar.interval_max, value);
    bar.eh.Increment();
}

bool BarSplittingHistBuilder::EmptyBar(BarIter it) const {
    return AggregateSize(it) == 0;
}

BarSplittingHistBuilder::BarSplittingHistBuilder(std::uint64_t buckets_num,
                                                 double scaling_factor,
                                                 std::uint64_t eh_sketch_precision,
                                                 std::uint64_t window_size)
    : eh_sketch_precision_(eh_sketch_precision) 
    , window_size_(window_size)
    , elements_visited_(0)
    , buckets_num_(buckets_num)
    , scaling_factor_(scaling_factor)
{}

std::size_t BarSplittingHistBuilder::AggregateSize(BarIter start) const {
    std::size_t result=0;

    if (start == bars_.end()) {
        return result;
    } 

    do {
        result += start->eh.Count();
        start++;
    } while (start != bars_.end() && start->is_blocked);

    return result;
}

BarIter BarSplittingHistBuilder::FindOrCreateBarFor(double value) {
    auto it  = search_map_.lower_bound(value);
  
    if (it == search_map_.begin()) {

        if (bars_.empty() || bars_.front().interval_min >= value) {
            bars_.push_front(Bar{EHSketch(eh_sketch_precision_, window_size_), 0, 0, false});
            search_map_[value] = bars_.begin();
            return bars_.begin();
        }

        return bars_.begin();
    }
    
    // return last bar
    if (it == search_map_.end()) {
        return std::prev(it)->second;
    }

    return it->second;
}

bool BarSplittingHistBuilder::FindAdjacentBothEmpty(BarIter& out1, BarIter& out2) const {
    for (auto it = search_map_.begin(); std::next(it) != search_map_.end(); it++) {
        out1 = it->second;
        out2 = std::next(it)->second;
        if (EmptyBar(out1) && EmptyBar(out2)) {
            return true;
        }
    }
    return false;
}

bool BarSplittingHistBuilder::FindAdjacentOneEmpty(BarIter& out1, BarIter& out2) const {
    for (auto curr_it = search_map_.begin(); curr_it != search_map_.end(); curr_it++) {
        BarIter curr = curr_it->second; 
        if (EmptyBar(curr)) {
            std::size_t l_neighbor_size = (curr_it == search_map_.begin() ? -1 : AggregateSize(std::prev(curr_it)->second));
            std::size_t r_neighbor_size = (std::next(curr_it) == search_map_.end() ? -1 : AggregateSize(std::next(curr_it)->second));

            if (l_neighbor_size <= r_neighbor_size) {
                out1 = std::prev(curr_it)->second;
                out2 = curr;
            } else {
                out1 = curr;
                out2 = std::next(curr_it)->second;
            }

            return true;
        }
    }
    return false;
}

bool BarSplittingHistBuilder::FindTwoAdjacentLowestAgSize(std::size_t max_bar_size, 
                                                          BarIter& out1, BarIter& out2) const 
{
    BarIter res1, res2;
    std::size_t min_ag_size = -1;

    for (auto curr = search_map_.begin(); std::next(curr) != search_map_.end(); curr++) {
        auto next = std::next(curr);

        out1 = curr->second;
        out2 = next->second;

        if (auto size = (AggregateSize(out1) + AggregateSize(out2)); size < min_ag_size) {
            res1 = out1;
            res2 = out2;
            min_ag_size = size; 
        }
    }

    out1 = res1;
    out2 = res2;
    return min_ag_size < max_bar_size;
}

// Ищем 2 пустые
// Ищем 1 пустую и мерджим с наименьшим соседом
// Ищем 2 соседние с наименьшим агрегатным размером
// Если агрегатный размер больше max_size то ничего не делаем
std::pair<BarIter, BarIter> BarSplittingHistBuilder::FindAdjacentBarsToMerge(std::size_t max_bar_size) {
    BarIter res1, res2;

    // .clang-format off
    bool found = FindAdjacentBothEmpty(res1, res2) 
                 || FindAdjacentOneEmpty(res1, res2)
                 || FindTwoAdjacentLowestAgSize(max_bar_size, res1, res2)
                 ; 
    // .clang-format on 

    if (found) {
        return std::make_pair(res1, res2);
    }
    return std::make_pair(bars_.end(), bars_.end());
}



bool BarSplittingHistBuilder::FindAndMergeBars(std::size_t max_bar_size) {
    auto [left_bar_it, right_bar_it] = FindAdjacentBarsToMerge(max_bar_size);    
    if (left_bar_it == bars_.end() || right_bar_it == bars_.end()) {
        return false;
    }
    
    Bar& X_l = *left_bar_it;
    Bar& X_r = *right_bar_it;

    EHSketch result_eh;
    double   result_min;
    double   result_max;
    BarIter  result_pos;

    bool erase_X_l = false;
    bool erase_X_r = false;
    
    if (X_l.eh.NumberOfBoxes() >= X_r.eh.NumberOfBoxes()) {
        result_eh = std::move(X_r.eh);
        X_l.is_blocked = true;
        erase_X_r = true;
    } else {
        result_eh = std::move(X_l.eh);
        X_r.is_blocked = true;
        erase_X_l = true;
    }
    
    result_min = X_l.interval_min;
    result_max = X_r.interval_max;

    search_map_.erase(X_l.interval_min);
    search_map_.erase(X_r.interval_min);

    if (erase_X_l) {
        bars_.erase(left_bar_it); // right bar is blocked
                                   // insert new_bar before right_bar_it 
        result_pos = right_bar_it;
    }

    if (erase_X_r) {
        bars_.erase(right_bar_it); // left bar is blocked, 
                                   // insert new_bar before left_bar_it 
        result_pos = left_bar_it;
    }
   
    
    Bar result_bar {
        .eh = std::move(result_eh),
        .interval_min = result_min,
        .interval_max = result_max,
        .is_blocked = false
    };

    if (result_pos == bars_.begin()) {
        bars_.push_front(std::move(result_bar));
        search_map_[result_min] = bars_.begin();
    } else if (result_pos == bars_.end()) {
        bars_.push_back(std::move(result_bar));
        search_map_[result_min] = std::prev(bars_.end());
    } else {
        search_map_[result_min] = bars_.insert(result_pos, std::move(result_bar));
    }
    return true;
}

void BarSplittingHistBuilder::SplitBar(BarIter it, double Sm, std::size_t max_bar_size) {
    if (it->eh.Count() == 1) {
        return;
    }
    if (NonBlockedBarsCount() >= Sm && !FindAndMergeBars(max_bar_size)) {
        return;
    } 

    Bar& split_bar = *it;
    std::size_t size = split_bar.eh.Count();
    std::size_t aggregated_size = AggregateSize(it) - size;

    std::list<Bar> X_l_blocked; 
    std::list<Bar> X_r_blocked;
    std::size_t cnt = 0;

    // Distribute blocked bars between X_l and X_r
    for (auto curr = std::next(it); curr != bars_.end() && curr->is_blocked;) {
        std::size_t curr_size = curr->eh.Count();
        auto next = std::next(curr);
        if (cnt + curr_size < (aggregated_size/2)) {
            X_l_blocked.splice(std::prev(X_l_blocked.end()), bars_, curr);
            cnt += curr_size;
        } else {
            X_r_blocked.splice(std::prev(X_r_blocked.end()), bars_, curr);
        }

        curr = next;
    }

    double interval_mid = (split_bar.interval_min + split_bar.interval_max)/2;

    // Init split bars
    Bar l{
        .eh = EHSketch(eh_sketch_precision_, window_size_),
        .interval_min = split_bar.interval_min,
        .interval_max = interval_mid,
        .is_blocked = false
    };

    Bar r{
        .eh = EHSketch(eh_sketch_precision_, window_size_),
        .interval_min=interval_mid,
        .interval_max=split_bar.interval_max,
        .is_blocked=false,
    };

    // Distributing EHSketch::Box between l and r EH sketches
    double size_x = static_cast<double>(size);
    double block_bars_size = static_cast<double>(aggregated_size);
    double ratio = (size_x / 2.f - static_cast<double>(cnt)) / (size_x - block_bars_size);

    bool add_to_l = true;
    for (EHSketch::Box& b : split_bar.eh) {
        if (b.count == 1) {
            if (add_to_l) {
                l.eh.Boxes().push_back(b);
                l.eh.Compress();
            } else {
                r.eh.Boxes().push_back(b);
                r.eh.Compress();
            }
            add_to_l = !add_to_l;
            continue;
        }

        b.count = b.count / 2;
        for (int i = 0; i < 2; i++) {
            double rSize = r.eh.Count();
            double lSize = l.eh.Count();

            if ((rSize / (lSize + rSize)) < ratio) {
                r.eh.Boxes().push_back(b);
                r.eh.Compress();
            } else {
                l.eh.Boxes().push_back(b);
                l.eh.Compress();
            }
        }
    }  
    

    auto l_iter = bars_.insert(it, std::move(l));
    auto r_iter = bars_.insert(it, std::move(r));

    bars_.splice(l_iter, X_l_blocked);
    bars_.splice(r_iter, X_r_blocked);

    search_map_[split_bar.interval_min] = l_iter;
    search_map_[interval_mid] = r_iter;
    bars_.erase(it);
}

void BarSplittingHistBuilder::InsertIntoBar(double value) {
    elements_visited_++;
    auto bar_map_it = FindOrCreateBarFor(value);
    Bar& related_bar = *bar_map_it;
    AppendBar(related_bar, value);

    double Sm = static_cast<double>(buckets_num_) * scaling_factor_;
    std::uint64_t max_bucket_size = std::round(kMaxCoef * (static_cast<double>(elements_visited_) / Sm));

    if (max_bucket_size && NeedSplit(bar_map_it, max_bucket_size)) {
        SplitBar(bar_map_it, Sm, max_bucket_size);      
    }
}

void BarSplittingHistBuilder::HandleIteration(KeyIterator& iter, TypeAdapter& conv) {
//    Tick();
    InsertIntoBar(conv.AsDouble(iter.Value()));
} 

HistType BarSplittingHistBuilder::Build() {
    HistType result;
    result.reserve(buckets_num_);
    
    std::uint64_t count = 0; 
    BarIter bar_iter = bars_.begin();

    std::uint64_t ideal_bucket_size = elements_visited_ / buckets_num_;

    for (std::size_t i = 0; i < buckets_num_; i++) {
        auto next = bar_iter;

        while (next != bars_.end()) {
            Bar& b = *next;
            count+=b.eh.Count();
            if (count <= ideal_bucket_size) {
                break;
            }
            next++;
        } 

        if (next == bars_.end()) {
            result.push_back(bar_iter->interval_min);
            break;
        }
        bar_iter = next;
        Bar& b = *bar_iter;

        std::uint64_t surplus = count - ideal_bucket_size;
        double len = b.interval_max - b.interval_min;

        result.push_back(b.interval_min + len * ((double)surplus/(double)b.eh.Count()));
        count = surplus;
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

void BarSplittingHistBuilder::Tick() {
    for (Bar& bar : bars_) {
        bar.eh.Tick();
    }
}

} // namespace vsh
