#include <any>
#include <arrow/type_fwd.h>
#include <cstdint>
#include <optional>
#include <parquet/exception.h>
#include <parquet/types.h>
#include <stdexcept>
#include <string>
#include <utility>

#include <vsh/key_iterator.hpp>
#include "internal/parquet_key_iterator.hpp"

namespace vsh {

ParquetKeyIterator::ParquetKeyIterator()
    : pimpl_(new internal::ParquetKeyIterator())
{}


ParquetKeyIterator::ParquetKeyIterator(ParquetKeyIterator&& other)
    : pimpl_(std::exchange(other.pimpl_, nullptr))
{}

ParquetKeyIterator& ParquetKeyIterator::operator=(ParquetKeyIterator&& other) noexcept {
    if (&other != this) {
        PARQUET_THROW_NOT_OK(pimpl_->Destruct());
        pimpl_ = std::exchange(other.pimpl_, nullptr);
    }
    return *this;
}

ParquetKeyIterator::~ParquetKeyIterator() {
    delete pimpl_;
}

arrow::Result<ParquetKeyIterator> ParquetKeyIterator::OverFile(const std::string& path, int64_t key_column_idx) {
    ParquetKeyIterator result;
    PARQUET_CATCH_NOT_OK(result.pimpl_->Construct(path, key_column_idx));
    result.StepForward();
    return result;
}

std::any ParquetKeyIterator::Type() const {
    return pimpl_->ColumnType();
}

std::size_t ParquetKeyIterator::RowPosition() const {
    return pimpl_->CurrentRowPosition();
}

const std::any& ParquetKeyIterator::Value() const {
    return pimpl_->ExtractDatum();
}

bool ParquetKeyIterator::HasNext() const {
    return pimpl_->HasNext();
}

void ParquetKeyIterator::StepForward() {
    pimpl_->StepForward();
}

std::optional<std::uint64_t> ParquetKeyIterator::StreamSize() const {
    return pimpl_->StreamSize();
}

template<typename EtalonType>
void ThrowIfTypeMismatch(const std::any& v) {
    if (v.type() != typeid(EtalonType)) {
        throw std::runtime_error("Invalid type: " + std::string(v.type().name()));
    }
}

bool ParquetKeyIterator::ParquetTypesAdaptor::IsInt64(const std::any& v) const {
    ThrowIfTypeMismatch<FieldType>(v);
    auto pqType = std::any_cast<FieldType>(v);
    return pqType == parquet::Type::INT64;
}

bool ParquetKeyIterator::ParquetTypesAdaptor::IsDouble(const std::any& v) const {
    ThrowIfTypeMismatch<FieldType>(v);
    auto pqType = std::any_cast<FieldType>(v);
    return pqType == parquet::Type::DOUBLE || pqType == parquet::Type::FLOAT;
}

int64_t ParquetKeyIterator::ParquetTypesAdaptor::AsInt64(const std::any& v) const {
    if (IsInt64(data_.Type())) {
        return std::any_cast<int64_t>(v);
    }

    if (IsDouble(data_.Type())) {
        return static_cast<int64_t>(std::any_cast<double>(v));
    }

    throw std::runtime_error("invalid conversion from " + std::string(v.type().name()) + "to int64_t");
}

double ParquetKeyIterator::ParquetTypesAdaptor::AsDouble(const std::any& v) const {
    if (IsDouble(data_.Type())) {
        return std::any_cast<double>(v);
    }


    if (IsInt64(data_.Type())) {
        return std::any_cast<int64_t>(v);
    }

    throw std::runtime_error("invalid conversion from " + std::string(v.type().name()) + "to double");
}

} // namespace vsh
