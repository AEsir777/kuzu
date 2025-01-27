#pragma once

#include "binder/copy/bound_copy_from.h"
#include "catalog/table_schema.h"
#include "common/copier_config/copier_config.h"
#include "common/table_type.h"
#include "planner/logical_plan/logical_operator.h"

namespace kuzu {
namespace planner {

class LogicalCopyFrom : public LogicalOperator {
public:
    LogicalCopyFrom(std::unique_ptr<binder::BoundCopyFromInfo> info,
        std::shared_ptr<binder::Expression> outputExpression)
        : LogicalOperator{LogicalOperatorType::COPY_FROM}, info{std::move(info)},
          outputExpression{std::move(outputExpression)} {}

    inline std::string getExpressionsForPrinting() const override {
        return info->tableSchema->tableName;
    }

    void computeFactorizedSchema() override;
    void computeFlatSchema() override;

    inline binder::BoundCopyFromInfo* getInfo() const { return info.get(); }
    inline binder::Expression* getOutputExpression() const { return outputExpression.get(); }

    inline std::unique_ptr<LogicalOperator> copy() override {
        return make_unique<LogicalCopyFrom>(info->copy(), outputExpression->copy());
    }

private:
    std::unique_ptr<binder::BoundCopyFromInfo> info;
    std::shared_ptr<binder::Expression> outputExpression;
};

} // namespace planner
} // namespace kuzu
