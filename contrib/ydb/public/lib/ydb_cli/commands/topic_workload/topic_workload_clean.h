#pragma once

#include <contrib/ydb/public/lib/ydb_cli/commands/ydb_workload.h>
#include <contrib/ydb/public/lib/ydb_cli/commands/topic_operations_scenario.h>

namespace NYdb::NConsoleClient {

class TCommandWorkloadTopicClean: public TWorkloadCommand {
public:
    TCommandWorkloadTopicClean();

    void Config(TConfig& config) override;
    void Parse(TConfig& config) override;
    int Run(TConfig& config) override;

private:
    class TScenario : public TTopicOperationsScenario {
        int DoRun(const TConfig& config) override;
    };

    TScenario Scenario;
};

class TCommandWorkloadTopicRun: public TClientCommandTree {
public:
    TCommandWorkloadTopicRun();
};

}
