#include "vsh/bar_splitting_hist.hpp"
#include <cstdlib>
#include <iostream>

#include <iterator>
#include <vsh/key_iterator.hpp>
#include <vsh/histogram.hpp>
#include <vsh/quantile_hist.hpp>

// #define ALLOW_INDICATORS
#ifdef ALLOW_INDICATORS
#include "indicators.hpp"
#endif

#include "vsh/consumer_node.hpp"



arrow::Status Creation(const std::string& file, vsh::ConsumerList& consumers) {
    ARROW_ASSIGN_OR_RAISE(auto iter, vsh::ParquetKeyIterator::OverFile(file, 0));
    
    auto h = vsh::BarSplittingHistBuilder(9);
    auto hist = vsh::MakeEquiDepthHistogram(h, iter);
    
    for (const auto& v : hist) {
        std::cout << v << " ";
    }
    std::cout << "\n";

    {
        ARROW_ASSIGN_OR_RAISE(auto itr, vsh::ParquetKeyIterator::OverFile(file, 0));
        auto conv = iter.ValuesAdapter();
        for (int i = 0;itr.HasNext(); itr.StepForward()) {
            auto v = conv->AsDouble(itr.Value());
            auto hist_pos = std::distance(hist.begin(), std::lower_bound(hist.begin(), hist.end(), v));
            consumers[hist_pos]->Consume(iter.Value());
        }
    }

    return arrow::Status::OK();
}

int main(int argc, char** argv) {
    std::string outputParquerPath = argv[1];

    auto consumers = vsh::ConstructConsumers<vsh::LoadStatisticsAccumulator>(10); 
    arrow::Status res = Creation(outputParquerPath, consumers);
    if (!res.ok()) {
        std::cerr << res << std::endl;
        return EXIT_FAILURE;
    }

    for (const auto& consumer_ptr : consumers) {
        auto c = std::dynamic_pointer_cast<vsh::LoadStatisticsAccumulator>(consumer_ptr);
        std::cout << c->rows_processed_ << "\n";
    }

    return EXIT_SUCCESS;
}


