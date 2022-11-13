#include "include/intersect_hash_table.h"

namespace graphflow {
namespace processor {

static void sortSelectedPos(const shared_ptr<ValueVector>& nodeIDVector) {
    auto selVector = nodeIDVector->state->selVector.get();
    auto size = selVector->selectedSize;
    auto selectedPos = selVector->getSelectedPositionsBuffer();
    if (selVector->isUnfiltered()) {
        memcpy(selectedPos, &SelectionVector::INCREMENTAL_SELECTED_POS, size * sizeof(sel_t));
        selVector->resetSelectorToValuePosBuffer();
    }
    sort(selectedPos, selectedPos + size, [nodeIDVector](sel_t left, sel_t right) {
        return ((nodeID_t*)nodeIDVector->values)[left] < ((nodeID_t*)nodeIDVector->values)[right];
    });
}

void IntersectHashTable::append(const vector<shared_ptr<ValueVector>>& vectorsToAppend) {
    auto numTuplesToAppend = 1;
    // Based on the way we are planning, we assume that the first and second vectors are both
    // nodeIDs from extending, while the first one is key, and the second one is payload.
    auto keyState = vectorsToAppend[0]->state.get();
    auto payloadNodeIDVector = vectorsToAppend[1];
    auto payloadsState = payloadNodeIDVector->state.get();
    assert(keyState->isFlat());
    if (!payloadsState->isFlat()) {
        sortSelectedPos(payloadNodeIDVector);
    }
    // A single appendInfo will return from `allocateFlatTupleBlocks` when numTuplesToAppend is 1.
    auto appendInfo = factorizedTable->allocateFlatTupleBlocks(numTuplesToAppend)[0];
    for (auto i = 0u; i < vectorsToAppend.size(); i++) {
        factorizedTable->copyVectorToColumn(*vectorsToAppend[i], appendInfo, numTuplesToAppend, i);
    }
    if (!payloadsState->isFlat()) {
        payloadsState->selVector->resetSelectorToUnselected();
    }
    factorizedTable->numTuples += numTuplesToAppend;
}

} // namespace processor
} // namespace graphflow
