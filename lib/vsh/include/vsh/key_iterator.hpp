#pragma  once

#include <cstdint>
#include <memory>
#include <parquet/types.h>
#include <string>
#include <any>

#include <arrow/type_fwd.h>
#include <arrow/datum.h>

namespace vsh {

struct TypeAdapter {
    virtual bool IsInt64(const std::any&) const = 0;
    virtual bool IsDouble(const std::any&) const = 0;
    virtual int64_t AsInt64(const std::any&) const = 0;
    virtual double AsDouble(const std::any&) const = 0;
    virtual ~TypeAdapter() = default;
};

using TypeAdapterPtr = std::shared_ptr<TypeAdapter>;

struct KeyIterator {
    virtual std::any Type() const = 0;
    virtual std::uint64_t RowPosition() const = 0;
    virtual const std::any& Value() const = 0;
    virtual bool HasNext() const = 0;
    virtual void StepForward() = 0;
    virtual std::optional<std::uint64_t> StreamSize() const = 0;
    virtual TypeAdapterPtr ValuesAdapter() const = 0;
    virtual ~KeyIterator() = default;
};

namespace internal {
struct ParquetKeyIterator;
} // namespace internal

class ParquetKeyIterator : public KeyIterator {
public:

    using FieldType = parquet::Type::type;

    class ParquetTypesAdaptor final : public TypeAdapter {
    public:
        explicit ParquetTypesAdaptor(const ParquetKeyIterator& it)
            : data_(it)
        {}

        bool IsInt64(const std::any&) const override; 
        bool IsDouble(const std::any&) const override;
        int64_t AsInt64(const std::any&) const override;
        double AsDouble(const std::any&) const override;
    private:
        const ParquetKeyIterator& data_;
    };

public:
    ParquetKeyIterator(ParquetKeyIterator&& other);
    ParquetKeyIterator& operator=(ParquetKeyIterator&&) noexcept;
    
    // TODO: Enable copying of ParquetKeyIterator
    ParquetKeyIterator(const ParquetKeyIterator&) = delete;
    ParquetKeyIterator& operator=(const ParquetKeyIterator& other) = delete;

    virtual ~ParquetKeyIterator();

    static arrow::Result<ParquetKeyIterator> OverFile(const std::string& path, int64_t key_column_idx = 0);

    /// @brief: Extracts library specific type of key-field
    /// 
    /// Usage example:
    ///         ParquetKeyIterator iter = ParquetKeyIterator::OverFile("example.parquet", 13)
    ///         FieldType type = std::any_cast<ParquetKeyIterator::FieldType>(iter.Type())
    /// @returns: underlying library-specific type of field
    std::any Type() const override;

    std::uint64_t RowPosition() const override;
    const std::any& Value() const override;
    bool HasNext() const override;
    void StepForward() override;
    std::optional<std::uint64_t> StreamSize() const override;

    [[nodiscard]]TypeAdapterPtr ValuesAdapter() const override {
        return std::make_shared<ParquetTypesAdaptor>(*this);
    }

protected:
    ParquetKeyIterator();
private:
    internal::ParquetKeyIterator* pimpl_;
};

} // namespace vsh

