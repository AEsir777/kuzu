#pragma once

#include <unordered_map>

#include "common/types/types_include.h"

namespace kuzu {
namespace processor {

/*
 * A Frontier can stores dst node offsets, its multiplicity and its bwd edges. Note that we don't
 * need to track all information in BFS computation.
 *
 * Computation                   |  Information tracked
 * Shortest path track path      |  nodeOffsets & bwdEdges
 * Shortest path NOT track path  |  nodeOffsets
 * Var length track path         |  nodeOffsets & bwdEdges
 * Var length NOT track path     |  nodeOffsets & offsetToMultiplicity
 */
struct Frontier {
    using node_rel_offset_t = std::pair<common::offset_t, common::offset_t>;
    using node_rel_offsets_t = std::vector<node_rel_offset_t>;

    std::vector<common::offset_t> nodeOffsets;
    std::unordered_map<common::offset_t, node_rel_offsets_t> bwdEdges;
    std::unordered_map<common::offset_t, uint64_t> offsetToMultiplicity;

    inline void resetState() {
        nodeOffsets.clear();
        bwdEdges.clear();
        offsetToMultiplicity.clear();
    }

    inline void addNode(common::offset_t offset) { nodeOffsets.push_back(offset); }

    inline void addEdge(
        common::offset_t boundOffset, common::offset_t nbrOffset, common::offset_t relOffset) {
        if (!bwdEdges.contains(nbrOffset)) {
            nodeOffsets.push_back(nbrOffset);
            bwdEdges.insert({nbrOffset, node_rel_offsets_t{}});
        }
        bwdEdges.at(nbrOffset).emplace_back(boundOffset, relOffset);
    }

    inline void addNodeWithMultiplicity(common::offset_t offset, uint64_t multiplicity) {
        if (offsetToMultiplicity.contains(offset)) {
            offsetToMultiplicity.at(offset) += multiplicity;
        } else {
            offsetToMultiplicity.insert({offset, multiplicity});
            nodeOffsets.push_back(offset);
        }
    }

    inline uint64_t getMultiplicity(common::offset_t offset) const {
        return offsetToMultiplicity.empty() ? 1 : offsetToMultiplicity.at(offset);
    }
};

} // namespace processor
} // namespace kuzu