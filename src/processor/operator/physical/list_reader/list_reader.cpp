#include "src/processor/include/operator/physical/list_reader/list_reader.h"

namespace graphflow {
namespace processor {

ListReader::ListReader(const uint64_t& dataChunkPos, const uint64_t& valueVectorPos,
    BaseLists* lists, shared_ptr<ListSyncer> listSyncer, unique_ptr<Operator> prevOperator)
    : Operator{move(prevOperator)}, inDataChunkPos{dataChunkPos},
      inValueVectorPos{valueVectorPos}, lists{lists} {
    dataChunks = this->prevOperator->getDataChunks();
    inDataChunk = dataChunks->getDataChunk(dataChunkPos);
    inNodeIDVector = static_pointer_cast<NodeIDVector>(inDataChunk->getValueVector(valueVectorPos));
    handle = make_unique<ColumnOrListsHandle>();
    handle->setListSyncer(listSyncer);
}

void ListReader::readValuesFromList() {
    nodeID_t nodeID;
    inNodeIDVector->readValue(inDataChunk->curr_idx, nodeID);
    lists->readValues(nodeID, outValueVector, outDataChunk->size, handle, MAX_TO_READ);
}

} // namespace processor
} // namespace graphflow
