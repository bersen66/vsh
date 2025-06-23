#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <memory_resource>
#include <stdexcept>
#include <vsh/bar_splitting_hist.hpp>
#include <cstdlib>
#include <cstdio>
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
std::size_t   gFileSizeRows = 0;  
bool          gComputeMetrics        = false;
bool          gShowFinalDistribution = true;
bool          gShowExecutionTime     = false;

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
    // Получаем итератор по данным
    ARROW_ASSIGN_OR_RAISE(auto itr, vsh::ParquetKeyIterator::OverFile(flagPathToDataset, flagColumnIdx));
    auto conv = itr.ValuesAdapter();

    const std::size_t num_partitions = consumers.size();

    if (num_partitions == 0) {
        return arrow::Status::OK();
    }

    // Обработка случая, когда гистограмма пуста (например, нет данных)
    if (hist.empty()) {
        printf("Warning: Histogram boundaries are empty. All values will go to the first partition.\n");
        for (int i = 0; itr.HasNext(); itr.StepForward()) {
            consumers[0]->Consume(conv->AsDouble(itr.Value()));
        }
        return arrow::Status::OK();
    }

    // Проверяем, достаточно ли границ.
    // Если `hist.size()` равно `num_partitions + 1`, это ожидаемый формат:
    // {min_val, boundary_1, boundary_2, ..., boundary_N-1, max_val}
    // где boundary_i - это N-1 внутренних границ.
    if (hist.size() != num_partitions + 1) {
        printf("Warning: Unexpected number of histogram boundaries (%zu) for %zu partitions. Expected %zu. Distribution might be inaccurate.\n", 
               hist.size(), num_partitions, num_partitions + 1);
        // Продолжаем выполнение, но с предупреждением.
        // Логика ниже все равно попытается работать, но может быть неоптимальной.
    }


    for (int i = 0; itr.HasNext(); itr.StepForward()) {
        auto v = conv->AsDouble(itr.Value());

        // Используем std::lower_bound для нахождения индекса первой границы, которая не меньше v.
        // Возвращаемое значение: итератор на первый элемент, не меньший v.
        // Вычитание hist.begin() дает нам 0-индексированное смещение.
        std::size_t consumer_idx = std::lower_bound(hist.begin(), hist.end(), v) - hist.begin();

        // Нормализация индекса для N партиций:
        // Если hist содержит N+1 границ (0, b1, b2, ..., bN-1, max_val):
        //   - v < 0      -> lower_bound вернет 0 (min_val)
        //   - 0 <= v < b1 -> lower_bound вернет 1 (b1)
        //   - bi <= v < b(i+1) -> lower_bound вернет i+1
        //   - v >= max_val -> lower_bound вернет hist.size() (т.е. num_partitions + 1)

        // Чтобы получить 0-индексированный номер партиции, нам нужно вычесть 1.
        // Например, если lower_bound вернул 1 (для 0 <= v < b1), то это партиция 0.
        if (consumer_idx > 0) {
            consumer_idx--;
        }
        // Если consumer_idx был 0 (v < min_val), он останется 0.
        // Это корректно, так как все, что меньше минимального значения, должно попасть в первую партицию.

        // Важно: зажать consumer_idx в пределах [0, num_partitions - 1].
        // Это обрабатывает случай, когда v >= max_val (lower_bound вернул hist.size()),
        // или когда Build() вернул не N+1 границ, и lower_bound выдал слишком большой индекс.
        consumer_idx = std::min(consumer_idx, num_partitions - 1);

        // Отправляем значение в соответствующий потребитель
        consumers[consumer_idx]->Consume(conv->AsDouble(itr.Value()));
    }

    return arrow::Status::OK();
}

arrow::Status DistributeValues2(const vsh::HistType& hist, vsh::ConsumerList& consumers) {
    ARROW_ASSIGN_OR_RAISE(auto itr, vsh::ParquetKeyIterator::OverFile(flagPathToDataset, flagColumnIdx));
    auto conv = itr.ValuesAdapter();
     for (int i = 0; itr.HasNext(); itr.StepForward()) {
        auto v = conv->AsDouble(itr.Value());

        std::size_t consumer_idx = std::lower_bound(hist.begin(), hist.end(), v) - hist.begin();
        consumers[consumer_idx % consumers.size()]->Consume(itr.Value());
    }


    return arrow::Status::OK();
}

arrow::Status ProcessUsingBASH(vsh::ParquetKeyIterator& iter, vsh::ConsumerList& consumers) {
    vsh::BarSplittingHistBuilder histBuilder(std::pmr::new_delete_resource(),
                                             flagNumberOfPartitions,
                                             /*scaling_factor=*/7.f,
                                             /*eh_sketch_precision=*/100,
                                             /*window_size=*/iter.StreamSize().value());
    auto hist = vsh::MakeEquiDepthHistogram(histBuilder, iter);
    
    return DistributeValues2(hist, consumers);
}

arrow::Status ProcessUsingQuantiles(vsh::ParquetKeyIterator& iter, vsh::ConsumerList& consumers) {
    vsh::QuantileHistBuilder histBuilder(flagNumberOfPartitions);
    auto hist = vsh::MakeEquiDepthHistogram(histBuilder, iter);
    return DistributeValues2(hist, consumers);
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
    gFileSizeRows = iter.StreamSize().value();

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
        .ideal_load = (long double)gFileSizeRows/(long double)flagNumberOfPartitions,
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
    std::printf(R"help_message(
Usage: ./driver <algo> <file> <number_of_partitions> <dataset_column>
    * algo                 -- supported values: "hash", "quantiles", "bash"
    * file                 -- path to dataset
    * number_of_partitions -- must be positive integer
    * dataset_column       -- must be non negative integer
)help_message");
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
        flagAlgorithm = AlgoFromString(argv[1]);
        flagPathToDataset = ValidatePath(argv[2]);
        flagNumberOfPartitions = std::atoi(argv[3]);
        if (argc >= 5) {
            flagColumnIdx = std::atoi(argv[4]);
        }

        if (const char* showDistr = getenv("SHOW_DISTR")) {
            gShowFinalDistribution = strcmp(showDistr, "0");
        }

        if (const char* computeStats = getenv("SHOW_METRICS")) {
            gComputeMetrics = strcmp(computeStats, "0");
        }

        if (const char* printExecTime = getenv("PRINT_EXEC_TIME")) {
            gShowExecutionTime = strcmp(printExecTime, "0");
        }
    } catch (const std::exception& exc) {
        std::printf("Exception: %s\n", exc.what());
        PrintHelpMessage();
        return EXIT_FAILURE;
    }

    auto consumers = vsh::ConstructConsumers<vsh::LoadStatisticsAccumulator>(flagNumberOfPartitions); 
    
    auto start = std::chrono::high_resolution_clock::now();
        arrow::Status res = Creation(flagPathToDataset, consumers);
    auto end= std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (!res.ok()) {
        std::fprintf(stderr, "%s\n", res.CodeAsString().c_str());
        return EXIT_FAILURE;
    }

    if (gComputeMetrics) {
        auto [rmse, max_deviation, max_relative_error, ideal_load] = EvaluateDistributionMetrics(consumers);
        std::printf("%Lf %Lf %Lf %ld\n", rmse, max_deviation, max_relative_error, duration); 
    }

    if (gShowFinalDistribution) {
        for (const auto& consumer : consumers) {
            std::printf("%lu ", consumer->RowsProcessed());
        }
        std::printf("\n");
    }


    if (gShowExecutionTime) {
        std::printf("%lu ms\n", duration); 
    }
    return EXIT_SUCCESS;
}

