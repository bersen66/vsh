#include <arrow/chunked_array.h>
#include <arrow/io/file.h>
#include <arrow/memory_pool.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/type.h>
#include <arrow/type_fwd.h>
#include <arrow/util/macros.h>
#include <parquet/column_reader.h>
#include <parquet/file_reader.h>
#include <parquet/arrow/schema.h>
#include <parquet/arrow/reader.h>
#include <parquet/properties.h>

#include "parquet_key_iterator.hpp"

namespace vsh {

namespace internal {

arrow::Status ParquetKeyIterator::Init(const std::string& path, int64_t key_column_idx) {
    key_column_idx_ = key_column_idx;

    arrow::MemoryPool* memoryPool = arrow::default_memory_pool();
    auto reader_properties = parquet::ReaderProperties(memoryPool);
    reader_properties.set_buffer_size(64 * 1024); // 64 KB
    reader_properties.enable_buffered_stream();

    auto arrow_reader_props = parquet::ArrowReaderProperties(/*use_threads=*/true);
    arrow_reader_props.set_batch_size(kReadBatchSize);

    parquet::arrow::FileReaderBuilder reader_builder;
    ARROW_RETURN_NOT_OK(reader_builder.OpenFile(path, /*memory_map=*/false, reader_properties));
    reader_builder.memory_pool(memoryPool);
    reader_builder.properties(arrow_reader_props);
    ARROW_ASSIGN_OR_RAISE(file_reader_, reader_builder.Build());

    current_row_group_ = 0;

    rg_reader_ = file_reader_->parquet_reader()->RowGroup(current_row_group_);
    col_reader_ = rg_reader_->Column(key_column_idx_);

    return arrow::Status::OK();
}

void ParquetKeyIterator::Construct(const std::string& path, int64_t key_column_idx) {
    PARQUET_THROW_NOT_OK(Init(path, key_column_idx)); 
}


std::any ParquetKeyIterator::ColumnType() const {
    return file_reader_->parquet_reader()->metadata()->schema()->Column(key_column_idx_)->physical_type();
}

const std::any& ParquetKeyIterator::ExtractDatum() const {
    return current_key_;
}

std::size_t ParquetKeyIterator::CurrentRowPosition() const {
    return current_pos_;
}

arrow::Status ParquetKeyIterator::Destruct() {
    file_reader_->parquet_reader()->Close();
    file_reader_.reset();
    rg_reader_.reset();
    col_reader_.reset();
    current_row_group_ = 0;
    key_column_idx_ = 0;
    current_pos_ = 0;
    return arrow::Status::OK();
}

bool ParquetKeyIterator::HasNext() const {
    return CurrentRowPosition() < StreamSize();
}

bool ParquetKeyIterator::StepForward() {

    parquet::Int64Reader* concrete_reader = static_cast<parquet::Int64Reader*>(col_reader_.get());
    while (!concrete_reader->HasNext()) {
        current_row_group_++;
        rg_reader_ = file_reader_->parquet_reader()->RowGroup(current_row_group_);
        col_reader_ = rg_reader_->Column(key_column_idx_);
        concrete_reader = static_cast<parquet::Int64Reader*>(col_reader_.get());
    }

    int64_t data;
    int64_t values_read;
    ARROW_UNUSED(values_read);
    int64_t rows_readed = concrete_reader->ReadBatch(1, nullptr, nullptr, &data, &values_read);

    if (!rows_readed) {
        return false;
    }

    current_key_ = data;
    current_pos_++; 
    return true;
}

std::optional<std::uint64_t> ParquetKeyIterator::StreamSize() const {
    return file_reader_->parquet_reader()->metadata()->num_rows();
}

} // namespace internal

} // namespace vsh
