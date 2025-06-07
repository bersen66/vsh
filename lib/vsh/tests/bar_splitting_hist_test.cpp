#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <memory_resource>
#include <random>
#include <vsh/bar_splitting_hist.hpp>
#include <vsh/eh_sketch.hpp>

struct BASHBuilderTestAdapter : public vsh::BarSplittingHistBuilder {

    using vsh::BarSplittingHistBuilder::BarSplittingHistBuilder;

    const BarList& GetBars() const {
        return bars_;
    }

    void InsertValue(double value) {
        InsertIntoBar(value);
    }

    const BarSearchMap& GetSearchMap() const {
        return search_map_;
    }


    void Tick() {
        vsh::BarSplittingHistBuilder::Tick();
    }
};

namespace vsh {

static std::ostream& operator<<(std::ostream& out, const BASHBuilderTestAdapter::BarSearchMap& bars) {
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

std::array<std::byte, 1 << 20> buffer;

TEST(BASH, InsertSequential) {
    std::pmr::monotonic_buffer_resource local_pool{
        buffer.data(), buffer.size()
    };
    BASHBuilderTestAdapter bash(&local_pool,
                                /*buckets_num*/30, 
                                /*scaling_factor=*/2,
                                /*eh_sketch_prec=*/5,
                                /*window_size=*/10000);

    for (int i = 0; i < 10'000'000; i++) {
        ASSERT_NO_FATAL_FAILURE(bash.Tick())         << " fail at iter " << i;
        ASSERT_NO_FATAL_FAILURE(bash.InsertValue(i)) << " fail at iter " << i;
        ASSERT_TRUE(HasConsistentIndex(bash))        << " fail at iter " << i;
    }
}

TEST(BASH, InsertUniformDistribution) {
    std::pmr::monotonic_buffer_resource local_pool{
        buffer.data(), buffer.size()
    };
    BASHBuilderTestAdapter bash(&local_pool,
                                /*buckets_num*/30, 
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
    }
}
