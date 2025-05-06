#include "vsh/key_iterator.hpp"
#include <arrow/result.h>
#include <arrow/type_fwd.h>
#include <gtest/gtest.h>
#include <parquet/arrow/reader.h>
#include <parquet/exception.h>
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
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <iostream>
#include <parquet/exception.h>
#include <parquet/file_reader.h>
#include <parquet/properties.h>
#include <parquet/type_fwd.h>
#include <arrow/csv/api.h>
#include <parquet/column_reader.h>


class ParquetKeyIteratorTest : public testing::Test {
    void SetUp() override {
        std::cerr << "SETUP";
    }
protected:
    std::string temp_parquet_file_;
};

static arrow::Status CreateParquetFile(const std::string& file) {
    auto schema = arrow::schema({
        arrow::field("key", arrow::int64()),
        arrow::field("value", arrow::utf8())
    });

    arrow::Int64Builder idBuilder;
    ARROW_RETURN_NOT_OK(idBuilder.AppendValues({1, 2, 3, 4}));
    std::shared_ptr<arrow::Array> idArray;
    ARROW_RETURN_NOT_OK(idBuilder.Finish(&idArray));

    arrow::StringBuilder valBuilder;
    ARROW_RETURN_NOT_OK(valBuilder.AppendValues({"BOBA", "AKULA", "ARTUR", "SABINA"}));
    std::shared_ptr<arrow::Array> valArray;
    ARROW_RETURN_NOT_OK(valBuilder.Finish(&valArray));

    
    std::shared_ptr<arrow::Table> table = arrow::Table::Make(
        schema,
        {idArray, valArray},
        4
    );

    std::shared_ptr<arrow::io::FileOutputStream> outputFile;
    ARROW_ASSIGN_OR_RAISE(outputFile, arrow::io::FileOutputStream::Open(file));
    
    PARQUET_THROW_NOT_OK(
        parquet::arrow::WriteTable(
            *table,
            arrow::default_memory_pool(),
            outputFile
        )
    );
    
    return arrow::Status::OK();
}

TEST_F(ParquetKeyIteratorTest, TestCreationAndIteration) {
    PARQUET_ASSIGN_OR_THROW(auto iter, vsh::ParquetKeyIterator::OverFile(temp_parquet_file_));
}


TEST_F(ParquetKeyIteratorTest, TestCreationAndIteration2) {
    PARQUET_ASSIGN_OR_THROW(auto iter, vsh::ParquetKeyIterator::OverFile(temp_parquet_file_));
}
