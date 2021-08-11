#pragma once

#include "src/processor/include/physical_plan/operator/read_list/read_list.h"

namespace graphflow {
namespace processor {

class ReadRelPropertyList : public ReadList {

public:
    ReadRelPropertyList(uint64_t inDataChunkPos, uint64_t inValueVectorPos,
        uint64_t outDataChunkPos, Lists* lists, unique_ptr<PhysicalOperator> prevOperator,
        ExecutionContext& context, uint32_t id);

    void getNextTuples() override;

    unique_ptr<PhysicalOperator> clone() override {
        return make_unique<ReadRelPropertyList>(inDataChunkPos, inValueVectorPos, outDataChunkPos,
            lists, prevOperator->clone(), context, id);
    }

private:
    uint64_t outDataChunkPos;
};

} // namespace processor
} // namespace graphflow
