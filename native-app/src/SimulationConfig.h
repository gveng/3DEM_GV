#pragma once

#include <QString>

enum class FrequencySweepMode {
    Linear,
    Adaptive
};

struct FrequencySweepConfig {
    double fMinHz = 1.0e9;
    double fMaxHz = 8.0e9;
    FrequencySweepMode mode = FrequencySweepMode::Linear;

    // Linear settings
    double stepHz = 1.0e8;

    // Adaptive settings
    double tolerance = 0.02;
    int maxPoints = 120;
    double minIntervalHz = 1.0e6;
};

struct PortConfig {
    QString name = "P1";
    double impedanceOhm = 50.0;
};
