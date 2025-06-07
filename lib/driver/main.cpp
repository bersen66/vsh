#include <limits>
#include <memory>
#include <stdexcept>
#include <vsh/bar_splitting_hist.hpp>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <iterator>
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

arrow::Status ProcessUsingBASH(vsh::ParquetKeyIterator& iter, vsh::ConsumerList& consumers) {
    vsh::BarSplittingHistBuilder histBuilder(flagNumberOfPartitions, 4.f, 500, 500);

    for(auto conv = iter.ValuesAdapter(); iter.HasNext(); iter.StepForward()) {
        histBuilder.HandleIteration(iter, *conv);
    }

    auto hist = vsh::MakeEquiDepthHistogram(histBuilder, iter);

    for (const auto& v : hist) {
        std::printf("%f ", v);
    }
    std::printf("\n");
    hist.push_back(std::numeric_limits<double>::max());

    ARROW_ASSIGN_OR_RAISE(auto itr, vsh::ParquetKeyIterator::OverFile(flagPathToDataset, flagColumnIdx));
    auto conv = itr.ValuesAdapter();
    for (int i = 0; itr.HasNext(); itr.StepForward()) {
        auto v = conv->AsDouble(itr.Value());

        std::size_t consumer_idx = 0;

        for (int i = 0; i+1 < hist.size(); i++) {
            if (hist[i] <= v && hist[i+1] > v) {
                consumer_idx = i;
                break;
            }
            
        }

        consumers[consumer_idx]->Consume(iter.Value());
    }
    
    return arrow::Status::OK();
}

arrow::Status ProcessUsingQuantiles(vsh::ParquetKeyIterator& iter, vsh::ConsumerList& consumers) {
    vsh::QuantileHistBuilder histBuilder(flagNumberOfPartitions);

    for(auto conv = iter.ValuesAdapter(); iter.HasNext(); iter.StepForward()) {
        histBuilder.HandleIteration(iter, *conv);
    }

    auto hist = vsh::MakeEquiDepthHistogram(histBuilder, iter);

    for (const auto& v : hist) {
        std::printf("%f ", v);
    }
    std::printf("\n");
    hist.push_back(std::numeric_limits<double>::max());

    ARROW_ASSIGN_OR_RAISE(auto itr, vsh::ParquetKeyIterator::OverFile(flagPathToDataset, flagColumnIdx));
    auto conv = itr.ValuesAdapter();
    for (int i = 0; itr.HasNext(); itr.StepForward()) {
        auto v = conv->AsDouble(itr.Value());

        std::size_t consumer_idx = 0;

        for (int i = 0; i+1 < hist.size(); i++) {
            if (hist[i] <= v && hist[i+1] > v) {
                consumer_idx = i;
                break;
            }
            
        }

        consumers[consumer_idx]->Consume(iter.Value());
    }
    
    return arrow::Status::OK();
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

    switch(flagAlgorithm) {
        case SupportedAlgo::kBASH:      return ProcessUsingBASH(iter, consumers);
        case SupportedAlgo::kQuantiles: return ProcessUsingQuantiles(iter, consumers);
        case SupportedAlgo::kHash:      return ProcessUsingHash(iter, consumers);
    }

    return arrow::Status::OK();
}

inline void PrintHelpMessage() {
    std::cerr << R"help_message(
Usage: ./driver <algo> <file> <number_of_partitions>
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
    arrow::Status res = Creation(flagPathToDataset, consumers);
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


