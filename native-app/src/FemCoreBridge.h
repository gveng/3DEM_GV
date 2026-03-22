#pragma once

#include "SimulationConfig.h"

class FemCoreBridge {
public:
    struct SolveSummary {
        bool ok = false;
        double elapsedSeconds = 0.0;
        int solvedPoints = 0;
    };

    SolveSummary runFrequencySweep(const FrequencySweepConfig& sweep, const PortConfig& port) const;
};
