#include "vsh/histogram.hpp"
#include <any>
#include <arrow/array/builder_binary.h>
#include <arrow/csv/options.h>
#include <arrow/csv/reader.h>
#include <arrow/io/api.h>
#include <arrow/api.h>
#include <arrow/io/file.h>
#include <arrow/io/interfaces.h>
#include <arrow/io/type_fwd.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/table.h>
#include <arrow/table_builder.h>
#include <arrow/type.h>
#include <arrow/type_fwd.h>
#include <arrow/util/thread_pool.h>
#include <arrow/util/type_fwd.h>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <iostream>
#include <parquet/exception.h>
#include <parquet/file_reader.h>
#include <parquet/properties.h>
#include <parquet/type_fwd.h>
#include <arrow/csv/api.h>
#include <parquet/column_reader.h>

#include <vsh/key_iterator.hpp>

// #define ALLOW_INDICATORS
#ifdef ALLOW_INDICATORS
#include "indicators.hpp"
#endif

#include "vsh/consumer_node.hpp"

arrow::Status Creation(const std::string& file, vsh::ConsumerList& consumers) {
    ARROW_ASSIGN_OR_RAISE(auto iter, vsh::ParquetKeyIterator::OverFile(file, 0));
#ifdef ALLOW_INDICATORS
    std::size_t rows_count = 0;
    using namespace indicators;
    show_console_cursor(false);
    BlockProgressBar bar{
        option::BarWidth{80},
        option::Start{"["},
        option::End{"]"},
        option::ForegroundColor{Color::white},
        option::FontStyles{std::vector<FontStyle>{FontStyle::bold},},
        option::PrefixText{"Scanning " +  std::filesystem::path(file).filename().string()},
        option::ShowElapsedTime{true},
    };
    std::size_t elements_num = iter.StreamSize().value();
    std::size_t prev_update = 0;
    bar.set_progress(0.f);
#endif // ALLOW_INDICATORS

    auto hasher = std::hash<int64_t>{};
    auto hist = vsh::MakeEquiDepthHistogram(iter);
    /*
    while (iter.HasNext()) {
        iter.StepForward();
        consumers[hasher(std::any_cast<int64_t>(iter.Value())) % consumers.size()]->Consume(iter.Value());

#ifdef  ALLOW_INDICATORS
        rows_count++;
        if (rows_count - prev_update >= (iter.StreamSize().value()/100)) [[unlikely]] {
            bar.set_progress(100.f * (static_cast<double>(rows_count) / static_cast<double>(elements_num)));
            prev_update = rows_count;
        }
#endif // ALLOW_INDICATORS
    }

#ifdef ALLOW_INDICATORS
    show_console_cursor(true);
    bar.set_progress(100);
    bar.mark_as_completed();
    std::cout << "Stream size: " << iter.StreamSize().value() << " - " << rows_count << '\n';
#endif // ALLOW_INDICATORS
    */

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
    // arrow::Status res = CreateParquetFromCsv(inputCsvPath, outputParquerPath);
    // arrow::Status res = ReadLargeParquetTable(outputParquerPath); 


    return EXIT_SUCCESS;
}


