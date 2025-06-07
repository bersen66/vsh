#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <random>
#include <vsh/bar_splitting_hist.hpp>
#include <vsh/eh_sketch.hpp>

struct BASHBuilderTestAdapter : public vsh::BarSplittingHistBuilder {

    using vsh::BarSplittingHistBuilder::BarSplittingHistBuilder;

    const std::list<Bar>& GetBars() const {
        return bars_;
    }

    void InsertValue(double value) {
        InsertIntoBar(value);
    }

    const std::map<double, BarIter>& GetSearchMap() const {
        return search_map_;
    }


    void Tick() {
        vsh::BarSplittingHistBuilder::Tick();
    }
};

namespace vsh {

static std::ostream& operator<<(std::ostream& out, const std::map<double, BASHBuilderTestAdapter::BarIter>& bars) {
    for (const auto& [key, iter] : bars) {
        out << "\t\t" << "Key = " << key << " [" << iter->interval_min << " -> " << iter->interval_max << ")\n";
    }
    return out;
}

} // namespace vsh


testing::AssertionResult HasConsistentIndex(const BASHBuilderTestAdapter& bash) {
    bool firstIter = true;
    double prev_inerval_min, prev_inerval_max;
    double curr_inerval_min, curr_inerval_max;
    for (const auto& [key, iter] : bash.GetSearchMap()) {
        if (firstIter) {
            prev_inerval_min = iter->interval_min;
            prev_inerval_max = iter->interval_max;
            
            if (key != prev_inerval_min) {
                return testing::AssertionFailure() << "prev_inerval_min = " << prev_inerval_min
                                                   << " key = " << key << "\n"
                                                   << bash.GetSearchMap(); 
            }

            firstIter = false;
            continue;
        } 
           
        
        curr_inerval_min = iter->interval_min;
        curr_inerval_max = iter->interval_max;


        if (key != curr_inerval_min) {
            return testing::AssertionFailure() << "key != curr_inerval_min: key = " << key
                                               << " curr_inerval_min = " << curr_inerval_min << "\n"
                                               << bash.GetSearchMap(); 
        }

        if (key != prev_inerval_max) {
            return testing::AssertionFailure() << "key != prev_inerval_max: key = " << key
                                               << " prev_inerval_max = " << prev_inerval_max << "\n"
                                               << bash.GetSearchMap(); 
        }
       
        prev_inerval_min = curr_inerval_min;
        prev_inerval_max = curr_inerval_max;
    }
    return testing::AssertionSuccess(); 
}

testing::AssertionResult HasConsistentEhSketches(const BASHBuilderTestAdapter& bash)
{
    for (const auto& [eh, interval_min, interval_max, is_blocked] : bash.GetBars()) {
        auto& boxes = eh.Boxes();

        if (boxes.empty() && eh.NumberOfBoxes() != 0) {
            return testing::AssertionFailure() << "eh.Boxes() is empty but eh.NumberOfBoxes != 0";
        }

        std::size_t iterations_made = 0;
        std::size_t prev_start = 0;
        std::size_t prev_finish = 0;

        std::uint64_t precision   = eh.Precision();
        std::uint64_t window_size = eh.WindowSize();

        bool first_iter = true;

        for (const auto& [count, interval_start, interval_finish] : boxes) {
            if (first_iter) {
                prev_start  = interval_start;
                prev_finish = interval_finish;
                first_iter = false;
                continue;
            } 
         
        }
    }

    return testing::AssertionSuccess();
}

TEST(BASH, InsertSequential) {
    
    BASHBuilderTestAdapter bash(/*buckets_num*/30, 
                                /*scaling_factor=*/2,
                                /*eh_sketch_prec=*/5,
                                /*window_size=*/10000);

    for (int i = 0; i < 10'000'000; i++) {
        ASSERT_NO_FATAL_FAILURE(bash.Tick())         << " fail at iter " << i;
        ASSERT_NO_FATAL_FAILURE(bash.InsertValue(i)) << " fail at iter " << i;
        ASSERT_TRUE(HasConsistentIndex(bash))        << " fail at iter " << i;
        // ASSERT_NO_FATAL_FAILURE(
        //     ASSERT_TRUE(HasConsistentEhSketches(bash))   << " fail at iter " << i
        // );
    }
}

TEST(BASH, InsertUniformDistribution) {
    
    BASHBuilderTestAdapter bash(/*buckets_num*/30, 
                                /*scaling_factor=*/2,
                                /*eh_sketch_prec=*/50,
                                /*window_size=*/100);

    std::random_device device;
    std::mt19937 gen(device());
    std::uniform_int_distribution<> distrib(1, 10000);

    for (int i = 0; i < 10'000'000; i++) {
        auto val = distrib(gen);
        ASSERT_NO_FATAL_FAILURE(bash.Tick()) << " fail at iter " << i;
        ASSERT_NO_FATAL_FAILURE(bash.InsertValue(val)) << " fail at iter " << i;
        ASSERT_TRUE(HasConsistentIndex(bash)) << " fail at iter " << i << " after adding " << val;
        // ASSERT_NO_FATAL_FAILURE(
        //     ASSERT_TRUE(HasConsistentEhSketches(bash))   << " fail at iter " << i
        // );
    }
}
