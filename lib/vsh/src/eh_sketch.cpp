#include <iterator>
#include <memory_resource>
#include <tuple>
#include <algorithm>
#include <vsh/eh_sketch.hpp>

namespace vsh {

bool EHSketch::IsExpiredBox(const EHSketch::Box &b) const noexcept {
    return (current_time_ - b.interval_start) > window_size_;
}

void EHSketch::MergeBoxes(Box& to, const Box& from) noexcept {
    to.count += from.count;
    to.interval_start = std::min(to.interval_start, from.interval_start);
    to.interval_finish = std::max(to.interval_finish, from.interval_finish);
}

EHSketch::EHSketch(std::pmr::memory_resource* resource, std::uint64_t precision, std::uint64_t window_size)
    : boxes_(resource)
    , deleted_boxes_(resource)
    , current_time_(0)
    , window_size_(window_size)
    , precision_(precision)
    , curr_size_(0)
    , box_threshold_((static_cast<double>(precision_) / 2.f) + 2)
{}

void EHSketch::Compress() {
    using IterType = std::list<Box>::iterator;
    
    IterType curr = boxes_.begin();
    while (curr != boxes_.end()) {
        std::size_t range_len = 0;
        IterType next = curr;

        while  (next != boxes_.end()) {
            if (next->count > curr->count) {
                break;
            }
            range_len++;
            next++;
        }
        

        while (range_len > box_threshold_) {
            IterType oldest = std::prev(next);
            IterType oldest_prev = std::prev(oldest);

            MergeBoxes(*oldest, *oldest_prev);

            next = EraseBox(deleted_boxes_, boxes_, oldest_prev);
            range_len-=2;
        }

        curr = next;
    }
}

void EHSketch::ExcludeExpiredBoxes() {
    while (!boxes_.empty() && IsExpiredBox(boxes_.back())) {
        std::ignore = EraseBox(deleted_boxes_, boxes_, std::prev(boxes_.end()));
    }
}

EHSketch::Box& EHSketch::GetBox(BoxList& dst, BoxList& src) {
    if (src.empty()) {
        dst.push_front(Box{});
    } else {
        dst.splice(dst.begin(), src, src.begin());
    }
    return dst.front();
}

EHSketch::BoxList::iterator
EHSketch::EraseBox(BoxList& dst, BoxList& src, BoxList::iterator it) {
    auto next = std::next(it);
    dst.splice(dst.begin(), src, it);
    return next;
}

void EHSketch::Increment() {
    Box& box = GetBox(boxes_, deleted_boxes_);

    box.count = 1;
    box.interval_start = current_time_;
    box.interval_finish = current_time_;
    curr_size_++;

    ExcludeExpiredBoxes();
    Compress();

}

std::uint64_t EHSketch::Count() noexcept {
    return curr_size_;
}

void EHSketch::Tick() {
    current_time_++;
    Compress();
    ExcludeExpiredBoxes();
}

} // namespace vsh
