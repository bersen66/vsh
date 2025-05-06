#include <any>
#include <stdexcept>
#include <vsh/histogram.hpp>
#include <vsh/key_iterator.hpp>
#include <DataSketches/quantiles_sketch.hpp>

namespace vsh {

Histogram MakeEquiDepthHistogram(KeyIterator& key_range) {
    if (!key_range.StreamSize().has_value()) {
        throw std::runtime_error("dataset size is unkown, can't detect heavy hitters");
    }


    Histogram hist;
    
    for(auto conv = key_range.ValuesAdapter(); key_range.HasNext(); key_range.StepForward()) {
        double curr = conv->AsDouble(key_range.Value());
         
    } 


    return {};
}

} // namespace vsh
