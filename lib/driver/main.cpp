#include <algorithm>
#include <chrono>
#include <cstddef>
#include <limits>
#include <memory>
#include <memory_resource>
#include <stdexcept>
#include <vsh/bar_splitting_hist.hpp>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <vsh/key_iterator.hpp>
#include <vsh/histogram.hpp>
#include <vsh/quantile_hist.hpp>
#include <vsh/consumer_node.hpp>


enum class SupportedAlgo {
    kHash = 0,
    kQuantiles = 1,
    kBASH = 2,
};

SupportedAlgo flagAlgorithm;
const char*   flagPathToDataset;
std::size_t   flagNumberOfPartitions;
std::size_t   flagColumnIdx = 0;
std::size_t   gFileSize = 0;  

SupportedAlgo AlgoFromString(const char* arg) {
    if(std::strcmp(arg,"hash") == 0) {
        return SupportedAlgo::kHash;
    } else if (std::strcmp(arg, "quantile") == 0) {
        return SupportedAlgo::kQuantiles;
    } else if (std::strcmp(arg, "bash") == 0) {
        return SupportedAlgo::kBASH;
    }

    throw std::runtime_error("Invalid algo: " + std::string(arg));
}

const char* ValidatePath(const char* path) {
    if (access(path, F_OK) == 0) {
        return path;
    } 
    throw std::runtime_error("Invalid file: " + std::string(path));
}

arrow::Status DistributeValues(const vsh::HistType& hist, vsh::ConsumerList& consumers) {

    ARROW_ASSIGN_OR_RAISE(auto itr, vsh::ParquetKeyIterator::OverFile(flagPathToDataset, flagColumnIdx));
    auto conv = itr.ValuesAdapter();
    for (int i = 0; itr.HasNext(); itr.StepForward()) {
        auto v = conv->AsDouble(itr.Value());

        std::size_t consumer_idx = std::lower_bound(hist.begin(), hist.end(), v) - hist.begin();
        consumers[consumer_idx]->Consume(itr.Value());
    }


    return arrow::Status::OK();
}

arrow::Status ProcessUsingBASH(vsh::ParquetKeyIterator& iter, vsh::ConsumerList& consumers) {
    std::array<std::byte, 1 << 20> buffer;
    std::pmr::monotonic_buffer_resource local_pool{
        buffer.data(), buffer.size()
    };
    vsh::BarSplittingHistBuilder histBuilder(&local_pool, flagNumberOfPartitions, 5.f, 10, 5000);
    auto hist = vsh::MakeEquiDepthHistogram(histBuilder, iter);
    return DistributeValues(hist, consumers);
}

arrow::Status ProcessUsingQuantiles(vsh::ParquetKeyIterator& iter, vsh::ConsumerList& consumers) {
    vsh::QuantileHistBuilder histBuilder(flagNumberOfPartitions);
    auto hist = vsh::MakeEquiDepthHistogram(histBuilder, iter);
    return DistributeValues(hist, consumers);
}

arrow::Status ProcessUsingHash(vsh::KeyIterator& iter, vsh::ConsumerList& consumers) {

    auto conv = iter.ValuesAdapter();
    auto hasher = std::hash<double>{};

    for (int i = 0; iter.HasNext(); iter.StepForward()) {
        auto value = conv->AsDouble(iter.Value());
        auto v = hasher(value) % flagNumberOfPartitions;
        consumers[v]->Consume(value);
    }
    
    return arrow::Status::OK();
}

arrow::Status Creation(const std::string& file, vsh::ConsumerList& consumers) {
    ARROW_ASSIGN_OR_RAISE(auto iter, vsh::ParquetKeyIterator::OverFile(file, flagColumnIdx));
    gFileSize = iter.StreamSize().value();

    switch(flagAlgorithm) {
        case SupportedAlgo::kBASH:      return ProcessUsingBASH(iter, consumers);
        case SupportedAlgo::kQuantiles: return ProcessUsingQuantiles(iter, consumers);
        case SupportedAlgo::kHash:      return ProcessUsingHash(iter, consumers);
    }

    return arrow::Status::OK();
}

struct DistributionMetrics {
    long double rmse;
    long double max_deviation;
    long double max_relative_error;
    long double ideal_load;
};

DistributionMetrics EvaluateDistributionMetrics(const vsh::ConsumerList& consumers) {
    DistributionMetrics metrics{
        .rmse = 0,
        .max_deviation = (consumers.empty() ? 0 : std::numeric_limits<double>::min()),
        .max_relative_error = 0,
        .ideal_load = (long double)gFileSize/(long double)flagNumberOfPartitions,
    };

    for (std::size_t i = 0; i < consumers.size(); i++) {
        auto consumer_ptr = std::dynamic_pointer_cast<vsh::LoadStatisticsAccumulator>(consumers[i]);

        // std::printf("Consumer %ld): %ld\n", i, consumer_ptr->rows_processed_);

        long double deviation = std::abs(metrics.ideal_load - consumer_ptr->rows_processed_);

        metrics.max_deviation = std::max(metrics.max_deviation, deviation);
        metrics.max_relative_error = std::max(metrics.max_relative_error, deviation / metrics.ideal_load);
        metrics.rmse += deviation * deviation;
    }

    metrics.rmse = std::sqrt(metrics.rmse / consumers.size());

    return metrics;
}

inline void PrintHelpMessage() {
    std::cerr << R"help_message(
Usage: ./driver <algo> <file> <number_of_partitions> <dataset_column>
    * algo                 -- supported values: "hash", "quantiles", "bash"
    * file                 -- path to dataset
    * number_of_partitions -- must be positive integer
    * dataset_column       -- must be non negative integer
)help_message" << std::endl;
}

int main(int argc, char** argv) {
    if (std::strcmp(argv[1], "--help") == 0) {
        PrintHelpMessage();
        return EXIT_FAILURE;
    }

    if (!(argc >= 4 && argc <= 5)) {
        std::cerr << "Error: invalid usage" << std::endl;
        PrintHelpMessage();
        return EXIT_FAILURE;
    }
   
    try {
        flagAlgorithm           = AlgoFromString(argv[1]);
        flagPathToDataset       = ValidatePath(argv[2]);
        flagNumberOfPartitions  = std::atoi(argv[3]);
        if (argc >= 5) {
            flagColumnIdx           = std::atoi(argv[4]);
        }
    } catch (const std::exception& exc) {
        std::cerr << "Exception: " << exc.what() << std::endl; 
        PrintHelpMessage();
        return EXIT_FAILURE;
    }

    auto consumers = vsh::ConstructConsumers<vsh::LoadStatisticsAccumulator>(flagNumberOfPartitions); 
    
    auto start = std::chrono::high_resolution_clock::now();
    arrow::Status res = Creation(flagPathToDataset, consumers);
    auto end= std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (!res.ok()) {
        std::cerr << res << std::endl;
        return EXIT_FAILURE;
    }

    auto [rmse, max_deviation, max_relative_error, ideal_load] = EvaluateDistributionMetrics(consumers);
    std::printf("%Lf %Lf %Lf %ld", rmse, max_deviation, max_relative_error, duration); 
    return EXIT_SUCCESS;
}

