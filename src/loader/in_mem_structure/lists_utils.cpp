#include "src/loader/include/in_mem_structure/lists_utils.h"

#include <unordered_map>

#include "spdlog/spdlog.h"

#include "src/common/include/configs.h"
#include "src/storage/include/data_structure/lists/lists.h"

namespace graphflow {
namespace loader {

// Lists headers are created for only AdjLists and UnstrPropertyLists, both of which store data
// in the page without NULL bits.
void ListsUtils::calculateListHeadersTask(node_offset_t numNodeOffsets, uint32_t elementSize,
    listSizes_t* listSizes, ListHeaders* listHeaders, const shared_ptr<spdlog::logger>& logger) {
    logger->trace("Start: ListHeaders={0:p}", (void*)listHeaders);
    auto numElementsPerPage = PageUtils::getNumElementsInAPageWithoutNULLBytes(elementSize);
    auto numChunks = numNodeOffsets >> Lists::LISTS_CHUNK_SIZE_LOG_2;
    if (0 != (numNodeOffsets & (Lists::LISTS_CHUNK_SIZE - 1))) {
        numChunks++;
    }
    node_offset_t nodeOffset = 0u;
    uint64_t lAdjListsIdx = 0u;
    for (auto chunkId = 0u; chunkId < numChunks; chunkId++) {
        auto csrOffset = 0u;
        auto lastNodeOffsetInChunk = min(nodeOffset + Lists::LISTS_CHUNK_SIZE, numNodeOffsets);
        for (auto i = nodeOffset; i < lastNodeOffsetInChunk; i++) {
            auto numElementsInList = (*listSizes)[nodeOffset].load(memory_order_relaxed);
            uint32_t header;
            if (numElementsInList >= numElementsPerPage) {
                header = 0x80000000 + (lAdjListsIdx++ & 0x7fffffff);
            } else {
                header = ((csrOffset & 0xfffff) << 11) + (numElementsInList & 0x7ff);
                csrOffset += numElementsInList;
            }
            (*listHeaders).headers[nodeOffset] = header;
            nodeOffset++;
        }
    }
    logger->trace("End: adjListHeaders={0:p}", (void*)listHeaders);
}

void ListsUtils::calculateListsMetadataTask(uint64_t numNodeOffsets, uint32_t elementSize,
    listSizes_t* listSizes, ListHeaders* listHeaders, ListsMetadata* listsMetadata,
    bool hasNULLBytes, const shared_ptr<spdlog::logger>& logger) {
    logger->trace("Start: listsMetadata={0:p} adjListHeaders={1:p}", (void*)listsMetadata,
        (void*)listHeaders);
    auto globalPageId = 0u;
    auto numChunks = numNodeOffsets >> Lists::LISTS_CHUNK_SIZE_LOG_2;
    if (0 != (numNodeOffsets & (Lists::LISTS_CHUNK_SIZE - 1))) {
        numChunks++;
    }
    (*listsMetadata).initChunkPageLists(numChunks);
    node_offset_t nodeOffset = 0u;
    auto largeListIdx = 0u;
    for (auto chunkId = 0u; chunkId < numChunks; chunkId++) {
        auto lastNodeOffsetInChunk = min(nodeOffset + Lists::LISTS_CHUNK_SIZE, numNodeOffsets);
        for (auto i = nodeOffset; i < lastNodeOffsetInChunk; i++) {
            if (ListHeaders::isALargeList(listHeaders->headers[nodeOffset])) {
                largeListIdx++;
            }
            nodeOffset++;
        }
    }
    (*listsMetadata).initLargeListPageLists(largeListIdx);
    nodeOffset = 0u;
    largeListIdx = 0u;
    auto numPerPage = hasNULLBytes ? PageUtils::getNumElementsInAPageWithNULLBytes(elementSize) :
                                     PageUtils::getNumElementsInAPageWithoutNULLBytes(elementSize);
    for (auto chunkId = 0u; chunkId < numChunks; chunkId++) {
        auto numPages = 0u, offsetInPage = 0u;
        auto lastNodeOffsetInChunk = min(nodeOffset + Lists::LISTS_CHUNK_SIZE, numNodeOffsets);
        while (nodeOffset < lastNodeOffsetInChunk) {
            auto numElementsInList = (*listSizes)[nodeOffset].load(memory_order_relaxed);
            if (ListHeaders::isALargeList(listHeaders->headers[nodeOffset])) {
                auto numPagesForLargeList = numElementsInList / numPerPage;
                if (0 != numElementsInList % numPerPage) {
                    numPagesForLargeList++;
                }
                (*listsMetadata)
                    .populateLargeListPageList(
                        largeListIdx, numPagesForLargeList, numElementsInList, globalPageId);
                globalPageId += numPagesForLargeList;
                largeListIdx++;
            } else {
                while (numElementsInList + offsetInPage > numPerPage) {
                    numElementsInList -= (numPerPage - offsetInPage);
                    numPages++;
                    offsetInPage = 0;
                }
                offsetInPage += numElementsInList;
            }
            nodeOffset++;
        }
        if (0 == offsetInPage) {
            (*listsMetadata).populateChunkPageList(chunkId, numPages, globalPageId);
            globalPageId += numPages;
        } else {
            (*listsMetadata).populateChunkPageList(chunkId, numPages + 1, globalPageId);
            globalPageId += numPages + 1;
        }
    }
    listsMetadata->numPages = globalPageId;
    logger->trace(
        "End: listsMetadata={0:p} listHeaders={1:p}", (void*)listsMetadata, (void*)listHeaders);
}

void ListsUtils::calcPageElementCursor(uint32_t header, uint64_t reversePos,
    uint8_t numBytesPerElement, node_offset_t nodeOffset, PageElementCursor& cursor,
    ListsMetadata& metadata, bool hasNULLBytes) {
    auto numElementsInAPage =
        hasNULLBytes ? PageUtils::getNumElementsInAPageWithNULLBytes(numBytesPerElement) :
                       PageUtils::getNumElementsInAPageWithoutNULLBytes(numBytesPerElement);
    if (ListHeaders::isALargeList(header)) {
        auto lAdjListIdx = ListHeaders::getLargeListIdx(header);
        auto pos = metadata.getNumElementsInLargeLists(lAdjListIdx) - reversePos;
        cursor = PageUtils::getPageElementCursorForOffset(pos, numElementsInAPage);
        cursor.idx = metadata.getPageMapperForLargeListIdx(lAdjListIdx)(cursor.idx);
    } else {
        auto chunkId = nodeOffset >> Lists::LISTS_CHUNK_SIZE_LOG_2;
        auto csrOffset = ListHeaders::getSmallListCSROffset(header);
        auto pos = ListHeaders::getSmallListLen(header) - reversePos;
        cursor = PageUtils::getPageElementCursorForOffset(csrOffset + pos, numElementsInAPage);
        cursor.idx =
            metadata.getPageMapperForChunkIdx(chunkId)((csrOffset + pos) / numElementsInAPage);
    }
}

} // namespace loader
} // namespace graphflow
