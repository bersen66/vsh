#pragma once 

#include <any>
#include <memory>
#include <vector>

namespace vsh {

struct IConsumerNode {
    virtual void Consume(std::any data) = 0;
    virtual ~IConsumerNode() = default;
};

using ConsumerPtr = std::shared_ptr<IConsumerNode>;
using ConsumerList = std::vector<ConsumerPtr>;

struct LoadStatisticsAccumulator : public IConsumerNode {
public:
    LoadStatisticsAccumulator()
        : rows_processed_(0)
    {}

    void Consume(std::any) override {
        rows_processed_++;
    }

public:
    std::size_t rows_processed_; 
};


template<typename ConsumerType>
ConsumerList ConstructConsumers(std::size_t len) {
    ConsumerList result;
    result.reserve(len);

    for (std::size_t i = 0; i < len; i++) {
        result.push_back(std::make_shared<ConsumerType>());
    }

    return result;
}




} // namespace vsh
