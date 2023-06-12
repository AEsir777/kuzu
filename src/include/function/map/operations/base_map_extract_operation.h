#pragma once

#include "common/vector/value_vector.h"
#include "common/vector/value_vector_utils.h"

namespace kuzu {
namespace function {
namespace operation {

struct BaseMapExtract {
    static void operation(common::list_entry_t& resultEntry, common::ValueVector& resultVector,
        uint8_t* srcValues, common::ValueVector* srcVector, uint64_t numValuesToCopy) {
        resultEntry = common::ListVector::addList(&resultVector, numValuesToCopy);
        auto dstValues = common::ListVector::getListValues(&resultVector, resultEntry);
        auto dstDataVector = common::ListVector::getDataVector(&resultVector);
        for (auto i = 0u; i < numValuesToCopy; i++) {
            common::ValueVectorUtils::copyValue(dstValues, *dstDataVector, srcValues, *srcVector);
            dstValues += dstDataVector->getNumBytesPerValue();
            srcValues += srcVector->getNumBytesPerValue();
        }
    }
};

} // namespace operation
} // namespace function
} // namespace kuzu