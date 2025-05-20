#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <vsh/bar_splitting_hist.hpp>

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
                                                   << " key = " << key; 
            }

            firstIter = false;
            continue;
        } 
           
        
        curr_inerval_min = iter->interval_min;
        curr_inerval_max = iter->interval_max;


        if (key != curr_inerval_min) {
            return testing::AssertionFailure() << "key != curr_inerval_min: key = " << key
                                               << " curr_inerval_min = " << curr_inerval_min; 
        }

        if (key != prev_inerval_max) {
            return testing::AssertionFailure() << "key != prev_inerval_max: key = " << key
                                               << " prev_inerval_max = " << prev_inerval_max;
        }
       
        prev_inerval_min = curr_inerval_min;
        prev_inerval_max = curr_inerval_max;
    }
    return testing::AssertionSuccess(); 
}


TEST(BASH, Adding) {
    BASHBuilderTestAdapter bash(5);
    for (int i = 0; i < 1'000'000; i++) {
        ASSERT_NO_FATAL_FAILURE(bash.Tick()) << " fail at iter " << i;
        ASSERT_NO_FATAL_FAILURE(bash.InsertValue(i)) << " fail at iter " << i;
        EXPECT_TRUE(HasConsistentIndex(bash)) << " fail at iter " << i;
    }

}
