#include <cmath>
#include <gtest/gtest.h>
#include <vsh/eh_sketch.hpp>

namespace vsh {

bool operator==(const vsh::EHSketch::Box& lhs, const vsh::EHSketch::Box& rhs) {
    return std::tie(lhs.count, lhs.interval_start, lhs.interval_finish) ==
           std::tie(rhs.count, rhs.interval_start, rhs.interval_finish);
}

std::ostream& operator<<(std::ostream& out, const vsh::EHSketch::Box& box) {
    out << "{ count:" << box.count << ", begin:" << box.interval_start << ", end:" << box.interval_finish << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const std::list<vsh::EHSketch::Box>& boxes) {
    out << "[";
    for (const auto& box : boxes) {
        out << box;
    }
    out << "]";
    return out;
}


} // namespace vsh

struct EHSketchTest : public vsh::EHSketch {

    explicit EHSketchTest(std::uint64_t precision = 100, std::uint64_t window_size = -1)
        : EHSketch(precision, window_size)
    {}

    using vsh::EHSketch::EHSketch;

    std::uint64_t CurrentTime() const noexcept {
        return current_time_;
    } 

    std::uint64_t WindowSize() const noexcept {
        return window_size_;
    }

    std::uint64_t Precision() const noexcept {
        return precision_;
    }

    const std::list<Box>& BoxesList() const noexcept {
        return boxes_;
    }

    void Reset() {
        boxes_.clear();
        current_time_ = 0;
    }
};

TEST(TestEHSketch, Basic) {
    EHSketchTest eh(/*precision=*/2, /*window_size=*/35);

    ASSERT_TRUE(eh.BoxesList().empty());
    ASSERT_EQ(eh.CurrentTime(), 0);
    ASSERT_EQ(eh.WindowSize(), 35);
    ASSERT_EQ(eh.Precision(), 2);


    for (int i = 1; i <= 1000; i++) {
        ASSERT_NO_FATAL_FAILURE(eh.Tick());
        ASSERT_EQ(eh.CurrentTime(), i);
    }

    eh.Reset();

    ASSERT_NO_FATAL_FAILURE(eh.TickIncrement());
    ASSERT_EQ(eh.BoxesList().size(), 1);
    ASSERT_EQ(eh.BoxesList().back().count, 1);
    ASSERT_EQ(eh.BoxesList().back().interval_start, 1);
    ASSERT_EQ(eh.BoxesList().back().interval_finish, 1);

    ASSERT_NO_FATAL_FAILURE(eh.TickIncrement());
    ASSERT_EQ(eh.BoxesList().size(), 2);
    ASSERT_EQ(eh.BoxesList().front().count, 1);
    ASSERT_EQ(eh.BoxesList().front().interval_start, 2);
    ASSERT_EQ(eh.BoxesList().front().interval_finish, 2);
}

TEST(TestEHSketch, Compression) {
    EHSketchTest eh(/*precision=*/2, /*window_size=*/35);

    using BoxList = std::list<vsh::EHSketch::Box>;
    std::vector<BoxList> expectation = {
        {{1, 1, 1}},
        {{1, 2, 2}, {1, 1, 1}},
        {{1, 3, 3}, {1, 2, 2}, {1, 1, 1}},
        {{1, 4, 4}, {1, 3, 3}, {2, 1, 2}},
        {{1, 5, 5}, {1, 4, 4}, {1, 3, 3}, {2, 1, 2}}, 
        {{1, 6, 6}, {1, 5, 5}, {2, 3, 4}, {2, 1, 2}},
        {{1, 7, 7}, {1, 6, 6}, {1, 5, 5}, {2, 3, 4}, {2, 1, 2}},
        {{1, 8, 8}, {1, 7, 7}, {2, 5, 6}, {2, 3, 4}, {2, 1, 2}},
        {{1, 9, 9}, {1, 8, 8}, {1, 7, 7}, {2, 5, 6}, {2, 3, 4}, {2, 1, 2}},
        {{1, 10, 10}, {1, 9, 9}, {2, 7, 8}, {2, 5, 6}, {4, 1, 4}},
        {{1, 11, 11}, {1, 10, 10}, {1, 9, 9}, {2, 7, 8}, {2, 5, 6}, {4, 1, 4}},
        {{1, 12, 12}, {1, 11, 11}, {2, 9, 10}, {2, 7, 8}, {2, 5, 6}, {4, 1, 4}},
        {{1, 13, 13}, {1, 12, 12}, {1, 11, 11}, {2, 9, 10}, {2, 7, 8}, {2, 5, 6}, {4, 1, 4}},
        {{1, 14, 14}, {1, 13, 13}, {2, 11, 12},  {2, 9, 10}, {4, 5, 8}, {4, 1, 4}},
    };

    for (std::size_t i = 0; i < expectation.size(); i++) {
        ASSERT_NO_FATAL_FAILURE(eh.TickIncrement());
        ASSERT_TRUE(
            std::equal(expectation[i].begin(), expectation[i].end(), 
                       eh.BoxesList().begin(), eh.BoxesList().end())
        ) << "Failed at iteration: " << i << "\n"
          << "Etalon: " << expectation[i] << "\n"
          << "Actual: " << eh.BoxesList() << "\n";  
    }
}

TEST(TestEHSketch, ExcludingExpired) {
    EHSketchTest eh(/*precision=*/2, /*window_size=*/4);
    using BoxList = std::list<vsh::EHSketch::Box>;
    std::vector<BoxList> expectation = {
        {{1, 1, 1}},
        {{1, 2, 2}, {1, 1, 1}},
        {{1, 3, 3}, {1, 2, 2}, {1, 1, 1}},
        {{1, 4, 4}, {1, 3, 3}, {2, 1, 2}},
        {{1, 5, 5}, {1, 4, 4}, {1, 3, 3}, {2, 1, 2}}, 
        {{1, 6, 6}, {1, 5, 5}, {2, 3, 4}},
        {{1, 7, 7}, {1, 6, 6}, {1, 5, 5}, {2, 3, 4}},
        {{1, 8, 8}, {1, 7, 7}, {2, 5, 6}},
        {{1, 9, 9}, {1, 8, 8}, {1, 7, 7}, {2, 5, 6},},
        {{1, 10, 10}, {1, 9, 9}, {2, 7, 8}},
        {{1, 11, 11}, {1, 10, 10}, {1, 9, 9}, {2, 7, 8}},
        {{1, 12, 12}, {1, 11, 11}, {2, 9, 10}},
        {{1, 13, 13}, {1, 12, 12}, {1, 11, 11}, {2, 9, 10}},
        {{1, 14, 14}, {1, 13, 13}, {2, 11, 12}},
    };

    for (std::size_t i = 0; i < expectation.size(); i++) {
        ASSERT_NO_FATAL_FAILURE(eh.TickIncrement());
        ASSERT_TRUE(
            std::equal(expectation[i].begin(), expectation[i].end(), 
                       eh.BoxesList().begin(), eh.BoxesList().end())
        ) << "Failed at iteration: " << i << "\n"
          << "Etalon: " << expectation[i] << "\n"
          << "Actual: " << eh.BoxesList() << "\n"; 
    } 
}
