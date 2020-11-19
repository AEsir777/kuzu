#include "src/common/loader/include/nodes_loader.h"

#include "src/common/loader/include/csv_reader.h"
#include "src/storage/include/node_property_store.h"

namespace graphflow {
namespace common {

void NodesLoader::load(vector<string> &filenames, vector<uint64_t> &numNodesPerLabel,
    vector<uint64_t> &numBlocksPerLabel, vector<vector<uint64_t>> &numLinesPerBlock,
    vector<shared_ptr<NodeIDMap>> &nodeIDMappings) {
    logger->info("Starting to read and store node properties.");
    vector<shared_ptr<pair<unique_ptr<mutex>, vector<uint32_t>>>> files(
        catalog.getNodeLabelsCount());
    vector<gfNodeOffset_t> frontierOffsets(catalog.getNodeLabelsCount());
    for (gfLabel_t label = 0; label < catalog.getNodeLabelsCount(); label++) {
        files[label] =
            createFilesForNodeProperties(label, catalog.getPropertyMapForNodeLabel(label));
        frontierOffsets[label] = 0;
    }
    auto maxNumBlocks = *max_element(begin(numBlocksPerLabel), end(numBlocksPerLabel));
    for (auto blockId = 0u; blockId < maxNumBlocks; blockId++) {
        for (gfLabel_t label = 0; label < catalog.getNodeLabelsCount(); label++) {
            if (blockId < numBlocksPerLabel[label]) {
                threadPool.execute(populateNodePropertyColumnTask, filenames[label],
                    metadata.at("tokenSeparator").get<string>()[0],
                    catalog.getPropertyMapForNodeLabel(label), blockId,
                    numLinesPerBlock[label][blockId], frontierOffsets[label], files[label],
                    nodeIDMappings[label], logger);
                frontierOffsets[label] += numLinesPerBlock[label][blockId];
            }
        }
    }
    threadPool.wait();
}

shared_ptr<pair<unique_ptr<mutex>, vector<uint32_t>>> NodesLoader::createFilesForNodeProperties(
    gfLabel_t label, const vector<Property> &propertyMap) {
    auto files = make_shared<pair<unique_ptr<mutex>, vector<uint32_t>>>();
    files->first = make_unique<mutex>();
    files->second.resize(propertyMap.size());
    for (auto i = 0u; i < propertyMap.size(); i++) {
        if (NODE == propertyMap[i].dataType) {
            continue;
        } else if (STRING == propertyMap[i].dataType) {
            logger->warn("Ignoring string properties.");
        } else {
            auto fname = NodePropertyStore::getColumnFname(
                outputDirectory, label, propertyMap[i].propertyName);
            files->second[i] = open(fname.c_str(), O_WRONLY | O_CREAT, 0666);
            if (-1u == files->second[i]) {
                invalid_argument("cannot create file: " + fname);
            }
        }
    }
    return files;
}

void NodesLoader::populateNodePropertyColumnTask(string fname, char tokenSeparator,
    const vector<Property> &propertyMap, uint64_t blockId, uint64_t numElements,
    gfNodeOffset_t beginOffset, shared_ptr<pair<unique_ptr<mutex>, vector<uint32_t>>> pageFiles,
    shared_ptr<NodeIDMap> nodeIDMap, shared_ptr<spdlog::logger> logger) {
    logger->debug("start {0} {1}", fname, blockId);
    auto buffers = getBuffersForWritingNodeProperties(propertyMap, numElements, logger);
    CSVReader reader(fname, tokenSeparator, blockId);
    NodeIDMap map(numElements);
    if (0 == blockId) {
        reader.skipLine(); // skip header line
    }
    auto bufferOffset = 0u;
    while (reader.hasMore()) {
        map.setOffset(*reader.getNodeID().get(), beginOffset + bufferOffset);
        for (auto i = 1u; i < propertyMap.size(); i++) {
            switch (propertyMap[i].dataType) {
            case INT: {
                auto intVal = reader.skipTokenIfNULL() ? NULL_GFINT : reader.getInteger();
                memcpy((*buffers)[i] + (bufferOffset * getDataTypeSize(INT)), &intVal,
                    getDataTypeSize(INT));
                break;
            }
            case DOUBLE: {
                auto doubleVal = reader.skipTokenIfNULL() ? NULL_GFDOUBLE : reader.getDouble();
                memcpy((*buffers)[i] + (bufferOffset * getDataTypeSize(DOUBLE)), &doubleVal,
                    getDataTypeSize(DOUBLE));
                break;
            }
            case BOOLEAN: {
                auto boolVal = reader.skipTokenIfNULL() ? NULL_GFBOOL : reader.getBoolean();
                memcpy((*buffers)[i] + (bufferOffset * getDataTypeSize(BOOLEAN)), &boolVal,
                    getDataTypeSize(BOOLEAN));
                break;
            }
            default:
                if (!reader.skipTokenIfNULL()) {
                    reader.skipToken();
                }
            }
        }
        bufferOffset++;
    }
    lock_guard lck(*pageFiles->first);
    nodeIDMap->merge(map);
    for (auto i = 1u; i < propertyMap.size(); i++) {
        auto dataType = propertyMap[i].dataType;
        if (STRING == dataType) {
            continue;
        }
        auto offsetInFile = beginOffset * getDataTypeSize(dataType);
        if (-1 == lseek(pageFiles->second[i], offsetInFile, SEEK_SET)) {
            throw invalid_argument("Cannot seek to the required offset in file.");
        }
        auto bytesToWrite = numElements * getDataTypeSize(dataType);
        uint64_t bytesWritten = write(pageFiles->second[i], (*buffers)[i], bytesToWrite);
        if (bytesWritten != bytesToWrite) {
            throw invalid_argument("Cannot write in file.");
        }
    }
    logger->debug("end   {0} {1}", fname, blockId);
}

unique_ptr<vector<uint8_t *>> NodesLoader::getBuffersForWritingNodeProperties(
    const vector<Property> &propertyMap, uint64_t numElements, shared_ptr<spdlog::logger> logger) {
    logger->debug("creating buffers for elements: {0}", numElements);
    auto buffers = make_unique<vector<uint8_t *>>();
    for (auto &property : propertyMap) {
        if (STRING == property.dataType) {
            (*buffers).push_back(nullptr);
        } else {
            (*buffers).push_back(new uint8_t[numElements * getDataTypeSize(property.dataType)]);
        }
    }
    return buffers;
}

} // namespace common
} // namespace graphflow