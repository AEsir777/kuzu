#pragma once

#include "logical_operator_visitor.h"
#include "planner/logical_plan/logical_plan.h"

namespace kuzu {
namespace optimizer {

class TopKOptimizer : public LogicalOperatorVisitor {
public:
    void rewrite(planner::LogicalPlan* plan);

    std::shared_ptr<planner::LogicalOperator> visitOperator(
        std::shared_ptr<planner::LogicalOperator> op);

private:
    std::shared_ptr<planner::LogicalOperator> visitLimitReplace(
        std::shared_ptr<planner::LogicalOperator> op) override;
};

} // namespace optimizer
} // namespace kuzu
