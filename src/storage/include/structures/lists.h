#pragma once

#include "src/common/include/vector/node_vector.h"
#include "src/storage/include/structures/common.h"
#include "src/storage/include/structures/lists_aux_structures.h"

namespace graphflow {
namespace storage {

// BaseLists is the top-level structure that holds a set of lists {of adjacent edges or rel
// properties}.
class BaseLists : public BaseColumnOrLists {

public:
    void readValues(const nodeID_t& nodeID, const shared_ptr<ValueVector>& valueVector,
        uint64_t& listLen, const unique_ptr<ColumnOrListsHandle>& handle,
        uint32_t maxElementsToRead);

protected:
    BaseLists(const string& fname, size_t elementSize, shared_ptr<AdjListHeaders> headers,
        BufferManager& bufferManager)
        : BaseColumnOrLists{fname, elementSize, bufferManager}, metadata{fname}, headers(headers){};

    virtual void readFromLargeList(const nodeID_t& nodeID,
        const shared_ptr<ValueVector>& valueVector, uint64_t& listLen,
        const unique_ptr<ColumnOrListsHandle>& handle, uint32_t header, uint32_t maxElementsToRead);

    void readSmallList(const nodeID_t& nodeID, const shared_ptr<ValueVector>& valueVector,
        uint64_t& listLen, const unique_ptr<ColumnOrListsHandle>& handle, uint32_t header);

public:
    constexpr static uint16_t LISTS_CHUNK_SIZE = 512;

protected:
    ListsMetadata metadata;
    shared_ptr<AdjListHeaders> headers;
};

// Lists<T> is the implementation of BaseLists for Lists of a specific datatype T.
template<typename T>
class Lists : public BaseLists {

public:
    Lists(const string& fname, shared_ptr<AdjListHeaders> headers, BufferManager& bufferManager)
        : BaseLists{fname, sizeof(T), headers, bufferManager} {};
};

// Lists<nodeID_t> is the specialization of Lists<T> for lists of nodeIDs.
template<>
class Lists<nodeID_t> : public BaseLists {

public:
    Lists(const string& fname, BufferManager& bufferManager,
        NodeIDCompressionScheme nodeIDCompressionScheme)
        : BaseLists{fname, nodeIDCompressionScheme.getNumTotalBytes(),
              make_shared<AdjListHeaders>(fname), bufferManager},
          nodeIDCompressionScheme{nodeIDCompressionScheme} {};

    void readFromLargeList(const nodeID_t& nodeID, const shared_ptr<ValueVector>& valueVector,
        uint64_t& listLen, const unique_ptr<ColumnOrListsHandle>& handle, uint32_t header,
        uint32_t maxElementsToRead) override;

    NodeIDCompressionScheme& getNodeIDCompressionScheme() { return nodeIDCompressionScheme; }

    shared_ptr<AdjListHeaders> getHeaders() { return headers; };

private:
    NodeIDCompressionScheme nodeIDCompressionScheme;
};

typedef Lists<int32_t> RelPropertyListsInt;
typedef Lists<double_t> RelPropertyListsDouble;
typedef Lists<uint8_t> RelPropertyListsBool;
typedef Lists<gf_string_t> RelPropertyListsString;
typedef Lists<nodeID_t> AdjLists;

} // namespace storage
} // namespace graphflow
