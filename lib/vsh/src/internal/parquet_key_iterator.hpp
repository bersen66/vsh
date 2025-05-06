#include <memory>
#include <any>

#include <arrow/io/file.h>
#include <parquet/file_reader.h>
#include <parquet/arrow/reader.h>

namespace vsh {

namespace internal {

// TODO: iterating by column
class ParquetKeyIterator final {
public:
    static constexpr std::size_t kReadBatchSize = 128 * 1024; // 128 KB 
public:
    void Construct(const std::string& path, int64_t key_column_idx); 

    std::any ColumnType() const;

    std::size_t CurrentRowPosition() const;

    const std::any& ExtractDatum() const;

    bool HasNext() const; 

    bool StepForward(); 

    arrow::Status Destruct();

    std::optional<std::uint64_t> StreamSize() const;

private:

    [[nodiscard]]arrow::Status Init(const std::string& path, int64_t key_column_idx);

private:
    std::unique_ptr<parquet::arrow::FileReader> file_reader_;
    std::shared_ptr<parquet::RowGroupReader> rg_reader_;
    std::shared_ptr<parquet::ColumnReader> col_reader_;
    std::size_t current_row_group_;
    int64_t key_column_idx_;
    std::size_t current_pos_;
    std::any current_key_;
};

} // namespace internal

} // namespace vsh


