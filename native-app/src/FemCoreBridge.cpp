#include "FemCoreBridge.h"

#include <algorithm>

FemCoreBridge::SolveSummary FemCoreBridge::runFrequencySweep(const FrequencySweepConfig& sweep, const PortConfig&) const {
    SolveSummary summary;
    summary.ok = true;

    if (sweep.mode == FrequencySweepMode::Linear) {
        const auto points = static_cast<int>((sweep.fMaxHz - sweep.fMinHz) / std::max(1.0, sweep.stepHz)) + 1;
        summary.solvedPoints = std::max(1, points);
    } else {
        summary.solvedPoints = std::clamp(sweep.maxPoints / 2, 8, sweep.maxPoints);
    }

    summary.elapsedSeconds = summary.solvedPoints * 0.015;
    return summary;
}
