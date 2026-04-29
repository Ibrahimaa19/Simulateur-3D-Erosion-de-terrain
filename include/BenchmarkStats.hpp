#pragma once

#include <cstddef>
#include <vector>

struct BenchmarkSummaryStats
{
    std::size_t n = 0;
    double mean = 0.0;
    double median = 0.0;
    double stddev = 0.0;
    double min = 0.0;
    double max = 0.0;
    double ci95Low = 0.0;
    double ci95High = 0.0;
    double p05 = 0.0;
    double p95 = 0.0;
};

BenchmarkSummaryStats computeBenchmarkSummaryStats(const std::vector<double>& values);
