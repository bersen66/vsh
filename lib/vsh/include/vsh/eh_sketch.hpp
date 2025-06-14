#pragma once

#include <cstdint>
#include <list>
#include <memory_resource>

namespace vsh {

class EHSketch {
public:
    
    struct Box {
        std::size_t count;
        std::uint64_t interval_start;
        std::uint64_t interval_finish;
    };

    using BoxList = std::pmr::list<Box>;

    bool IsExpiredBox(const Box& b) const noexcept;

    void MergeBoxes(Box& to, const Box& from) noexcept;

public:
    
    explicit EHSketch(std::pmr::memory_resource* resource,
                      std::uint64_t precision = 100, 
                      std::uint64_t window_size = -1);

    EHSketch(const EHSketch&) = delete;
    EHSketch& operator=(const EHSketch&) = delete;
    EHSketch(EHSketch&&) noexcept = default;
    EHSketch& operator=(EHSketch&&) noexcept = default;
    
    void Increment();

    inline void TickIncrement() {
        Tick();
        Increment();
    }

    void Compress();
    void ExcludeExpiredBoxes();

    std::uint64_t Count() noexcept;

    void Tick();

    std::size_t NumberOfBoxes() const noexcept {
        return boxes_.size();
    }

    auto begin() {
        return boxes_.begin();
    }

    auto end() {
        return boxes_.end();
    }

    BoxList& Boxes() {
        return boxes_;
    }

    const BoxList& Boxes() const {
        return boxes_;
    }

    std::uint64_t Precision() const noexcept {
        return precision_;
    }

    std::uint64_t WindowSize() const noexcept {
        return window_size_;
    }
protected:
    
    // Functionality for reusing already allocated boxes;
    Box& GetBox(BoxList& dst, BoxList& src);

    BoxList::iterator
    EraseBox(BoxList& dst, BoxList& src, BoxList::iterator it);

protected:
    BoxList boxes_;
    BoxList deleted_boxes_; // optimization for reusing allocated structs
    std::uint64_t current_time_;
    std::uint64_t window_size_;
    std::uint64_t precision_;
    std::uint64_t curr_size_;
    double box_threshold_;
};

} // namespace vsh



