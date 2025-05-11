#pragma once

#include <cstdint>
#include <ostream>
#include <list>

namespace vsh {

class EHSketch {
public:
    
    struct Box {
        std::size_t count;
        std::uint64_t interval_start;
        std::uint64_t interval_finish;
    };

    bool IsExpiredBox(const Box& b) const noexcept;

    void MergeBoxes(Box& to, const Box& from) noexcept;

public:
    
    explicit EHSketch(std::uint64_t window_size, std::uint64_t precision = 100);
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

    std::uint64_t Count() const noexcept;

    void Tick();

protected:
    
    // Functionality for reusing already allocated boxes;
    Box& GetBox(std::list<Box>& dst, std::list<Box>& src);

    std::list<EHSketch::Box>::iterator
    EraseBox(std::list<Box>& dst, std::list<Box>& src, std::list<Box>::iterator it);

protected:
    std::list<Box> boxes_;
    std::list<Box> deleted_boxes_; // optimization for reusing allocated structs
    std::uint64_t current_time_;
    std::uint64_t window_size_;
    std::uint64_t precision_;
    long double box_threshold_;
};

bool operator==(const vsh::EHSketch::Box& lhs, const vsh::EHSketch::Box& rhs);

std::ostream& operator<<(std::ostream& out, const vsh::EHSketch::Box& box);

std::ostream& operator<<(std::ostream& out, const std::list<vsh::EHSketch::Box>& boxes); 
} // namespace vsh



