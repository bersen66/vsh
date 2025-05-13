#include <gtest/gtest.h>
#include <ostream>
#include <vsh/bar_splitting_hist.hpp>

struct BASHBuilderTestAdapter : public vsh::BarSplittingHistBuilder {

    using vsh::BarSplittingHistBuilder::BarSplittingHistBuilder;

    const std::list<Bar>& GetBars() const {
        return bars_;
    }

    void InsertValue(double value) {
        InsertIntoBar(value);
    }


    Bar& FindOrCreateBar(double value) {
        return FindOrCreateBarFor(value)->second;
    }
};

namespace vsh {

std::ostream& operator<<(std::ostream& out, const BASHBuilderTestAdapter::BarsMap& m) {
    out << "PIPIP";
    return out;
}

} // namespace vsh

TEST(BASH, Adding) {
    BASHBuilderTestAdapter bash(5);

    for (int i = 0; i < 10000; i++) {
        ASSERT_NO_FATAL_FAILURE(bash.InsertValue(i));
    }
}
