#include "gtest/gtest.h"
#include "test/mock/mock_graph.h"

#include "src/binder/include/query_binder.h"
#include "src/parser/include/parser.h"
#include "src/planner/include/enumerator.h"
#include "src/planner/include/logical_plan/operator/extend/logical_extend.h"
#include "src/planner/include/logical_plan/operator/scan_node_id/logical_scan_node_id.h"

using ::testing::NiceMock;
using ::testing::Test;

using namespace graphflow::planner;

class OptimizerTest : public Test {

public:
    static unique_ptr<LogicalPlan> getBestPlan(const string& query, const Graph& graph) {
        auto parsedQuery = Parser::parseQuery(query);
        auto boundQuery = QueryBinder(graph.getCatalog()).bind(*parsedQuery);
        return Enumerator(graph).getBestPlan(*boundQuery);
    }

    static bool containSubstr(const string& str, const string& substr) {
        return str.find(substr) != string::npos;
    }
};

TEST_F(OptimizerTest, OneHopSingleFilter) {
    NiceMock<TinySnbGraph> graph;
    graph.setUp();

    auto query = "MATCH (a:person)-[:knows]->(b:person) WHERE a.age = 1 RETURN COUNT(*)";
    auto plan = OptimizerTest::getBestPlan(query, graph);
    auto& op1 = *plan->lastOperator;
    ASSERT_EQ(LOGICAL_EXTEND, op1.getLogicalOperatorType());
    ASSERT_TRUE(containSubstr(((LogicalExtend&)op1).nbrNodeID, "_b." + INTERNAL_ID_SUFFIX));
    auto& op2 = *op1.prevOperator->prevOperator->prevOperator->prevOperator;
    ASSERT_EQ(LOGICAL_SCAN_NODE_ID, op2.getLogicalOperatorType());
    ASSERT_TRUE(containSubstr(((LogicalScanNodeID&)op2).nodeID, "_a." + INTERNAL_ID_SUFFIX));
}

TEST_F(OptimizerTest, OneHopMultiFilters) {
    NiceMock<TinySnbGraph> graph;
    graph.setUp();

    auto query = "MATCH (a:person)-[:knows]->(b:person) WHERE a.age > 10 AND a.age < 20 AND b.age "
                 "= 45 RETURN COUNT(*)";
    auto plan = OptimizerTest::getBestPlan(query, graph);
    auto& op1 = *plan->lastOperator->prevOperator->prevOperator;
    ASSERT_EQ(LOGICAL_EXTEND, op1.getLogicalOperatorType());
    ASSERT_TRUE(containSubstr(((LogicalExtend&)op1).nbrNodeID, "_b." + INTERNAL_ID_SUFFIX));
    auto& op2 = *op1.prevOperator->prevOperator->prevOperator->prevOperator->prevOperator;
    ASSERT_EQ(LOGICAL_SCAN_NODE_ID, op2.getLogicalOperatorType());
    ASSERT_TRUE(containSubstr(((LogicalScanNodeID&)op2).nodeID, "_a." + INTERNAL_ID_SUFFIX));
}

TEST_F(OptimizerTest, TwoHop) {
    NiceMock<TinySnbGraph> graph;
    graph.setUp();

    auto query = "MATCH (a:person)-[:knows]->(b:person)-[:knows]->(c:person) RETURN COUNT(*)";
    auto plan = OptimizerTest::getBestPlan(query, graph);
    auto& op1 = *plan->lastOperator->prevOperator->prevOperator->prevOperator;
    ASSERT_EQ(LOGICAL_SCAN_NODE_ID, op1.getLogicalOperatorType());
    ASSERT_TRUE(containSubstr(((LogicalScanNodeID&)op1).nodeID, "_b." + INTERNAL_ID_SUFFIX));
}

TEST_F(OptimizerTest, TwoHopMultiFilters) {
    NiceMock<TinySnbGraph> graph;
    graph.setUp();

    auto query = "MATCH (a:person)-[:knows]->(b:person)-[:knows]->(c:person) WHERE a.age = 20 AND "
                 "b.age = 35 RETURN COUNT(*)";
    auto plan = OptimizerTest::getBestPlan(query, graph);
    auto& op1 = *plan->lastOperator->prevOperator->prevOperator->prevOperator->prevOperator
                     ->prevOperator->prevOperator->prevOperator;
    ASSERT_EQ(LOGICAL_SCAN_NODE_ID, op1.getLogicalOperatorType());
    ASSERT_TRUE(containSubstr(((LogicalScanNodeID&)op1).nodeID, "_b." + INTERNAL_ID_SUFFIX));
}
