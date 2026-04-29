#include "BenchmarkStats.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
double quantile(const std::vector<double>& sorted, double q)
{
    if (sorted.empty())
    {
        return 0.0;
    }

    if (sorted.size() == 1)
    {
        return sorted.front();
    }

    const double pos = q * static_cast<double>(sorted.size() - 1);
    const auto lower = static_cast<std::size_t>(std::floor(pos));
    const auto upper = static_cast<std::size_t>(std::ceil(pos));
    const double weight = pos - static_cast<double>(lower);

    return sorted[lower] * (1.0 - weight) + sorted[upper] * weight;
}
} // namespace

BenchmarkSummaryStats computeBenchmarkSummaryStats(const std::vector<double>& values)
{
    BenchmarkSummaryStats stats;

    std::vector<double> filtered;
    filtered.reserve(values.size());
    for (double value : values)
    {
        if (std::isfinite(value))
        {
            filtered.push_back(value);
        }
    }

    if (filtered.empty())
    {
        return stats;
    }

    std::sort(filtered.begin(), filtered.end());

    stats.n = filtered.size();
    stats.min = filtered.front();
    stats.max = filtered.back();
    stats.mean = std::accumulate(filtered.begin(), filtered.end(), 0.0) / static_cast<double>(filtered.size());
    stats.median = quantile(filtered, 0.5);
    stats.p05 = quantile(filtered, 0.05);
    stats.p95 = quantile(filtered, 0.95);

    if (filtered.size() > 1)
    {
        double variance = 0.0;
        for (double value : filtered)
        {
            const double delta = value - stats.mean;
            variance += delta * delta;
        }

        variance /= static_cast<double>(filtered.size() - 1);
        stats.stddev = std::sqrt(variance);
    }

    const double margin = stats.n > 0 ? 1.96 * stats.stddev / std::sqrt(static_cast<double>(stats.n)) : 0.0;
    stats.ci95Low = stats.mean - margin;
    stats.ci95High = stats.mean + margin;

    return stats;
}
