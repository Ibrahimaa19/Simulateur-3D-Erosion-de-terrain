#include "Benchmark.hpp"

#include "BenchmarkStats.hpp"
#include "FaultFormationTerrain.hpp"
#include "MidpointDisplacement.hpp"
#include "PerlinNoiseTerrain.hpp"
#include "TerrainConstants.hpp"
#include "ThermalErosion.hpp"

#if EROSION_ENABLE_MPI
#include "Mpi.hpp"
#endif

#if EROSION_ENABLE_RENDERING
#include "RendererManager.hpp"
#include <glm/gtc/matrix_transform.hpp>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifndef EROSION_ENABLE_RENDERING
#define EROSION_ENABLE_RENDERING 0
#endif

#ifndef EROSION_ENABLE_MPI
#define EROSION_ENABLE_MPI 0
#endif

namespace
{
using Clock = std::chrono::steady_clock;

struct CliArgs
{
    std::map<std::string, std::string> values;

    explicit CliArgs(int argc, char* argv[], int startIndex)
    {
        for (int i = startIndex; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg.rfind("--", 0) != 0)
            {
                continue;
            }

            const std::size_t eq = arg.find('=');
            if (eq != std::string::npos)
            {
                values[arg.substr(0, eq)] = arg.substr(eq + 1);
            }
            else if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0)
            {
                values[arg] = argv[++i];
            }
            else
            {
                values[arg] = "on";
            }
        }
    }

    std::string get(const std::string& key, const std::string& fallback) const
    {
        const auto it = values.find(key);
        return it == values.end() ? fallback : it->second;
    }

    int getInt(const std::string& key, int fallback) const
    {
        const auto value = get(key, "");
        if (value.empty())
        {
            return fallback;
        }
        return std::atoi(value.c_str());
    }

    double getDouble(const std::string& key, double fallback) const
    {
        const auto value = get(key, "");
        if (value.empty())
        {
            return fallback;
        }
        return std::atof(value.c_str());
    }
};

struct CommonConfig
{
    std::string mode;
    std::string terrain = "perlinNoise";
    int width = 1024;
    int height = 1024;
    int steps = 100;
    int warmup = 3;
    int runs = 10;
    std::filesystem::path outDir;
};

struct ErosionConfig
{
    CommonConfig common;
    std::vector<std::string> variants;
    std::vector<int> neighborhoods;
    std::vector<int> threads;
    float talusAngle = 25.0f;
    float transferRate = 0.1f;
};

std::vector<std::string> splitList(const std::string& value)
{
    std::vector<std::string> result;
    std::stringstream ss(value);
    std::string item;

    while (std::getline(ss, item, ','))
    {
        item.erase(std::remove_if(item.begin(), item.end(), [](unsigned char c) { return std::isspace(c); }),
                   item.end());
        if (!item.empty())
        {
            result.push_back(item);
        }
    }

    return result;
}

std::vector<int> parseIntList(const std::string& value, int fallback)
{
    if (value.empty())
    {
        return {fallback};
    }

    std::vector<int> result;
    for (const std::string& part : splitList(value))
    {
        result.push_back(std::atoi(part.c_str()));
    }

    return result.empty() ? std::vector<int>{fallback} : result;
}

bool parseOnOff(const std::string& value, bool fallback)
{
    if (value.empty())
    {
        return fallback;
    }

    return value == "on" || value == "true" || value == "1" || value == "yes";
}

CommonConfig parseCommon(const std::string& mode, const CliArgs& args)
{
    CommonConfig config;
    config.mode = mode;
    config.terrain = args.get("--terrain", config.terrain);
    config.width = args.getInt("--width", config.width);
    config.height = args.getInt("--height", config.height);
    config.steps = args.getInt("--steps", config.steps);
    config.warmup = args.getInt("--warmup", config.warmup);
    config.runs = args.getInt("--runs", config.runs);

    const std::string defaultOut = (std::filesystem::path("benchmarks") / "results" / mode).string();
    config.outDir = args.get("--out", defaultOut);

    return config;
}

bool isPowerOfTwoLocal(int value)
{
    return value > 0 && (value & (value - 1)) == 0;
}

int nearestMidpointSize(int requested)
{
    if (requested >= 3 && isPowerOfTwoLocal(requested - 1))
    {
        return requested;
    }

    int power = 2;
    while (power + 1 < requested)
    {
        power *= 2;
    }
    return power + 1;
}

std::unique_ptr<Terrain> buildTerrain(const std::string& terrainType, int width, int height)
{
    if (terrainType == "loadHeightmap")
    {
        auto terrain = std::make_unique<Terrain>();
        terrain->loadTerrain("../src/heightmap/heightmap.png", 1.0f, 100.0f);
        return terrain;
    }

    if (terrainType == "faultFormation")
    {
        auto terrain = std::make_unique<FaultFormationTerrain>();
        terrain->CreateFaultFormation(width, height, 1000, 0.0f, 255.0f, 1.0f, true, 0.5f);
        return terrain;
    }

    if (terrainType == "midpointDisplacement")
    {
        const int size = nearestMidpointSize(width);
        auto terrain = std::make_unique<MidpointDisplacement>();
        terrain->CreateMidpointDisplacement(size, 0.0f, 255.0f, 1.0f, 0.5f);
        return terrain;
    }

    auto terrain = std::make_unique<PerlinNoiseTerrain>();
    terrain->CreatePerlinNoise(width, height, 0.0f, 255.0f, 1.0f, 0.005f);
    return terrain;
}

double totalMass(const std::vector<float>& data)
{
    return std::accumulate(data.begin(), data.end(), 0.0);
}

double relativeMassError(const std::vector<float>& before, const std::vector<float>& after)
{
    const double initialMass = totalMass(before);
    if (initialMass == 0.0)
    {
        return 0.0;
    }

    return std::abs(totalMass(after) - initialMass) / initialMass;
}

void ensureOutputDir(const std::filesystem::path& path)
{
    std::filesystem::create_directories(path);
}

void writeSummaryHeader(std::ofstream& out, const std::string& prefixColumns)
{
    out << prefixColumns << "metric,n,mean,median,stddev,min,max,ci95_low,ci95_high,p05,p95,speedup,efficiency\n";
}

void writeSummaryRow(std::ofstream& out, const std::string& prefix, const std::string& metric,
                     const BenchmarkSummaryStats& stats, double speedup = 1.0, double efficiency = 1.0)
{
    out << prefix << metric << "," << stats.n << "," << stats.mean << "," << stats.median << "," << stats.stddev << ","
        << stats.min << "," << stats.max << "," << stats.ci95Low << "," << stats.ci95High << "," << stats.p05 << ","
        << stats.p95 << "," << speedup << "," << efficiency << "\n";
}

std::vector<std::string> allErosionVariants()
{
    return {"pure", "blocked", "blockedParallel", "checkerboard", "blockedCheckerboard", "inPlace", "inPlaceParallel"};
}

std::vector<std::string> parseVariantList(const CliArgs& args)
{
    const std::string value = args.get("--variant-list", args.get("--variant", "blocked"));
    if (value == "all")
    {
        return allErosionVariants();
    }

    return splitList(value);
}

std::vector<int> parseNeighborhoodList(const CliArgs& args)
{
    const std::string value = args.get("--neighbors-list", args.get("--neighbors", "8"));
    if (value == "all" || value == "both")
    {
        return {4, 8};
    }

    return parseIntList(value, 8);
}

std::vector<int> parseThreadList(const CliArgs& args)
{
#ifdef _OPENMP
    const int fallback = omp_get_max_threads();
#else
    const int fallback = 1;
#endif
    return parseIntList(args.get("--threads-list", args.get("--threads", "")), fallback);
}

int runErosionStep(ThermalErosion& erosion, const std::string& variant)
{
    if (variant == "pure")
    {
        return erosion.stepPureTwoPhase();
    }
    if (variant == "blocked")
    {
        return erosion.stepBlockedPureTwoPhase();
    }
    if (variant == "blockedParallel")
    {
        return erosion.stepBlockedParallelPureTwoPhase();
    }
    if (variant == "checkerboard")
    {
        return erosion.stepCheckerboardPureTwoPhase();
    }
    if (variant == "blockedCheckerboard")
    {
        return erosion.stepBlockedCheckerboardPureTwoPhase();
    }
    if (variant == "inPlace")
    {
        return erosion.stepCheckerboardInPlace();
    }
    if (variant == "inPlaceParallel")
    {
        return erosion.stepCheckerboardInPlaceParallel();
    }

    std::cerr << "Unknown erosion variant: " << variant << "\n";
    return 0;
}

struct ErosionRunRow
{
    int runId = 0;
    bool warmup = false;
    std::string terrain;
    int width = 0;
    int height = 0;
    int steps = 0;
    std::string variant;
    int neighbors = 0;
    int threads = 1;
    double totalMs = 0.0;
    double avgStepMs = 0.0;
    int lastCellsModified = 0;
    double finalMassError = 0.0;
};

struct ErosionGroup
{
    std::string variant;
    int neighbors = 0;
    int threads = 1;
    BenchmarkSummaryStats total;
    BenchmarkSummaryStats avgStep;
    BenchmarkSummaryStats massError;
    BenchmarkSummaryStats lastCells;
};

int runErosionBenchmark(const CliArgs& args)
{
    ErosionConfig config;
    config.common = parseCommon("erosion", args);
    config.variants = parseVariantList(args);
    config.neighborhoods = parseNeighborhoodList(args);
    config.threads = parseThreadList(args);
    config.talusAngle = static_cast<float>(args.getDouble("--talus", config.talusAngle));
    config.transferRate = static_cast<float>(args.getDouble("--transfer", config.transferRate));

    ensureOutputDir(config.common.outDir);

    auto terrain = buildTerrain(config.common.terrain, config.common.width, config.common.height);
    if (!terrain || !terrain->getData() || terrain->getData()->empty())
    {
        std::cerr << "Unable to build benchmark terrain.\n";
        return 1;
    }

    const std::vector<float> referenceData = *terrain->getData();
    const int width = terrain->getTerrainWidth();
    const int height = terrain->getTerrainHeight();

    std::vector<ErosionRunRow> rawRows;
    std::vector<ErosionGroup> groups;

    for (const std::string& variant : config.variants)
    {
        for (int neighbors : config.neighborhoods)
        {
            std::map<int, BenchmarkSummaryStats> baselineByThreads;

            for (int threads : config.threads)
            {
#ifdef _OPENMP
                if (threads > 0)
                {
                    omp_set_num_threads(threads);
                }
#endif

                std::vector<double> totalTimes;
                std::vector<double> avgTimes;
                std::vector<double> massErrors;
                std::vector<double> lastCells;

                const int totalRuns = config.common.warmup + config.common.runs;
                for (int run = 0; run < totalRuns; ++run)
                {
                    *terrain->getData() = referenceData;

                    ThermalErosion erosion;
                    erosion.loadTerrainInfo(*terrain);
                    erosion.setTalusAngle(config.talusAngle);
                    erosion.setTransferRate(config.transferRate);
                    if (neighbors == 4)
                    {
                        erosion.useFourNeighbors();
                    }
                    else
                    {
                        erosion.useEightNeighbors();
                    }

                    int lastModified = 0;
                    const auto t0 = Clock::now();
                    for (int step = 0; step < config.common.steps; ++step)
                    {
                        lastModified = runErosionStep(erosion, variant);
                    }
                    const auto t1 = Clock::now();

                    const double totalMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
                    const double avgMs = totalMs / static_cast<double>(config.common.steps);
                    const double massError = relativeMassError(referenceData, *terrain->getData());
                    const bool warmup = run < config.common.warmup;

                    rawRows.push_back({run + 1, warmup, config.common.terrain, width, height, config.common.steps,
                                       variant, neighbors, threads, totalMs, avgMs, lastModified, massError});

                    if (!warmup)
                    {
                        totalTimes.push_back(totalMs);
                        avgTimes.push_back(avgMs);
                        massErrors.push_back(massError);
                        lastCells.push_back(static_cast<double>(lastModified));
                    }
                }

                groups.push_back({variant, neighbors, threads, computeBenchmarkSummaryStats(totalTimes),
                                  computeBenchmarkSummaryStats(avgTimes), computeBenchmarkSummaryStats(massErrors),
                                  computeBenchmarkSummaryStats(lastCells)});
            }
        }
    }

    const auto rawPath = config.common.outDir / "raw_runs.csv";
    std::ofstream raw(rawPath);
    raw << "run_id,is_warmup,terrain,width,height,steps,variant,neighbors,threads,total_time_ms,"
           "avg_time_per_step_ms,last_cells_modified,final_mass_error\n";
    for (const ErosionRunRow& row : rawRows)
    {
        raw << row.runId << "," << (row.warmup ? 1 : 0) << "," << row.terrain << "," << row.width << "," << row.height
            << "," << row.steps << "," << row.variant << "," << row.neighbors << "," << row.threads << ","
            << row.totalMs << "," << row.avgStepMs << "," << row.lastCellsModified << "," << row.finalMassError << "\n";
    }

    const auto summaryPath = config.common.outDir / "summary_stats.csv";
    std::ofstream summary(summaryPath);
    writeSummaryHeader(summary, "terrain,width,height,steps,variant,neighbors,threads,");

    for (const ErosionGroup& group : groups)
    {
        double baselineMean = group.total.mean;
        int baselineThreads = group.threads;
        for (const ErosionGroup& candidate : groups)
        {
            if (candidate.variant == group.variant && candidate.neighbors == group.neighbors && candidate.threads == 1)
            {
                baselineMean = candidate.total.mean;
                baselineThreads = 1;
                break;
            }
        }

        const double speedup = group.total.mean > 0.0 ? baselineMean / group.total.mean : 1.0;
        const double efficiency = group.threads > 0 ? speedup / static_cast<double>(group.threads) : speedup;

        std::ostringstream prefix;
        prefix << config.common.terrain << "," << width << "," << height << "," << config.common.steps << ","
               << group.variant << "," << group.neighbors << "," << group.threads << ",";

        writeSummaryRow(summary, prefix.str(), "total_time_ms", group.total, speedup, efficiency);
        writeSummaryRow(summary, prefix.str(), "avg_time_per_step_ms", group.avgStep, speedup, efficiency);
        writeSummaryRow(summary, prefix.str(), "final_mass_error", group.massError, 1.0, 1.0);
        writeSummaryRow(summary, prefix.str(), "last_cells_modified", group.lastCells, 1.0, 1.0);

        (void)baselineThreads;
    }

    std::cout << "Erosion benchmark written to " << config.common.outDir << "\n";
    return 0;
}

struct FrameRow
{
    int frameId = 0;
    double cpuFrameMs = 0.0;
    double gpuFrameMs = std::numeric_limits<double>::quiet_NaN();
    double totalFrameMs = 0.0;
    double syncMs = 0.0;
    double drawMs = 0.0;
    std::size_t triangles = 0;
    std::size_t visiblePatches = 0;
    std::size_t totalPatches = 0;
    std::size_t dirtyPatches = 0;
    double fps = 0.0;
};

std::string csvDouble(double value)
{
    if (!std::isfinite(value))
    {
        return "nan";
    }

    std::ostringstream out;
    out << value;
    return out.str();
}

std::vector<int> syntheticDirtyPatches(std::size_t totalPatches, int frameId)
{
    std::vector<int> dirty;
    if (totalPatches == 0)
    {
        return dirty;
    }

    const std::size_t count = std::max<std::size_t>(1, totalPatches / 16);
    dirty.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
    {
        dirty.push_back(static_cast<int>((static_cast<std::size_t>(frameId) * count + i) % totalPatches));
    }

    return dirty;
}

void writeFrameCsv(const std::filesystem::path& path, const std::vector<FrameRow>& rows)
{
    std::ofstream out(path);
    out << "frame_id,cpu_frame_ms,gpu_frame_ms,total_frame_ms,sync_ms,draw_ms,triangles,visible_patches,"
           "total_patches,dirty_patches,fps\n";

    for (const FrameRow& row : rows)
    {
        out << row.frameId << "," << csvDouble(row.cpuFrameMs) << "," << csvDouble(row.gpuFrameMs) << ","
            << csvDouble(row.totalFrameMs) << "," << csvDouble(row.syncMs) << "," << csvDouble(row.drawMs) << ","
            << row.triangles << "," << row.visiblePatches << "," << row.totalPatches << "," << row.dirtyPatches << ","
            << csvDouble(row.fps) << "\n";
    }
}

void writeFrameSummaryCsv(const std::filesystem::path& path, const std::vector<FrameRow>& rows)
{
    std::ofstream out(path);
    out << "metric,n,mean,median,stddev,min,max,ci95_low,ci95_high,p05,p95\n";

    auto writeMetric = [&out](const std::string& metric, const std::vector<double>& values)
    {
        const BenchmarkSummaryStats stats = computeBenchmarkSummaryStats(values);
        out << metric << "," << stats.n << "," << stats.mean << "," << stats.median << "," << stats.stddev << ","
            << stats.min << "," << stats.max << "," << stats.ci95Low << "," << stats.ci95High << "," << stats.p05 << ","
            << stats.p95 << "\n";
    };

    std::vector<double> cpuFrame;
    std::vector<double> gpuFrame;
    std::vector<double> totalFrame;
    std::vector<double> sync;
    std::vector<double> draw;
    std::vector<double> triangles;
    std::vector<double> visiblePatches;
    std::vector<double> fps;

    for (const FrameRow& row : rows)
    {
        cpuFrame.push_back(row.cpuFrameMs);
        gpuFrame.push_back(row.gpuFrameMs);
        totalFrame.push_back(row.totalFrameMs);
        sync.push_back(row.syncMs);
        draw.push_back(row.drawMs);
        triangles.push_back(static_cast<double>(row.triangles));
        visiblePatches.push_back(static_cast<double>(row.visiblePatches));
        fps.push_back(row.fps);
    }

    writeMetric("cpu_frame_ms", cpuFrame);
    writeMetric("gpu_frame_ms", gpuFrame);
    writeMetric("total_frame_ms", totalFrame);
    writeMetric("sync_ms", sync);
    writeMetric("draw_ms", draw);
    writeMetric("triangles", triangles);
    writeMetric("visible_patches", visiblePatches);
    writeMetric("fps", fps);
}

#if EROSION_ENABLE_RENDERING
FrameRow measureRenderFrame(RendererManager& renderer, Terrain& terrain, int frameId, bool lodEnabled,
                            bool cullingEnabled, const std::string& syncMode, bool useSyntheticDirty)
{
    const std::size_t totalPatches = renderer.getPatches().size();
    std::vector<int> dirtyPatches =
        useSyntheticDirty ? syntheticDirtyPatches(totalPatches, frameId) : std::vector<int>{};

    if (syncMode == "full")
    {
        dirtyPatches.clear();
        dirtyPatches.reserve(totalPatches);
        for (std::size_t i = 0; i < totalPatches; ++i)
        {
            dirtyPatches.push_back(static_cast<int>(i));
        }
    }

    const auto sync0 = Clock::now();
    if (syncMode == "full")
    {
        renderer.updateVerticesCpuLod();
    }
    else if (syncMode == "dirty")
    {
        renderer.updateVerticesCpuLod(dirtyPatches);
    }
    const auto sync1 = Clock::now();

    const float width = static_cast<float>(terrain.getTerrainWidth());
    const float height = static_cast<float>(terrain.getTerrainHeight());
    const float phase = static_cast<float>(frameId % 180) / 180.0f;
    const glm::vec3 cameraPos(width * 0.5f + std::sin(phase * 6.28318f) * width * 0.25f, 220.0f,
                              -250.0f + std::cos(phase * 6.28318f) * height * 0.25f);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(width * 0.5f, 0.0f, height * 0.5f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.01f, 5000.0f);

    const auto draw0 = Clock::now();
    const RenderBenchmarkStats stats =
        renderer.collectRenderStats(cameraPos, projection, view, lodEnabled, cullingEnabled);
    const auto draw1 = Clock::now();

    FrameRow row;
    row.frameId = frameId;
    row.syncMs = std::chrono::duration<double, std::milli>(sync1 - sync0).count();
    row.drawMs = std::chrono::duration<double, std::milli>(draw1 - draw0).count();
    row.cpuFrameMs = row.syncMs + row.drawMs;
    row.totalFrameMs = row.cpuFrameMs;
    row.triangles = stats.triangles;
    row.visiblePatches = stats.visiblePatches;
    row.totalPatches = stats.totalPatches;
    row.dirtyPatches = dirtyPatches.size();
    row.fps = row.totalFrameMs > 0.0 ? 1000.0 / row.totalFrameMs : 0.0;

    return row;
}

int runRenderBenchmark(const CliArgs& args)
{
    CommonConfig config = parseCommon("render", args);
    const int frames = args.getInt("--frames", 300);
    const bool lodEnabled = parseOnOff(args.get("--lod", "on"), true);
    const bool cullingEnabled = parseOnOff(args.get("--culling", "on"), true);
    const std::string syncMode = args.get("--sync", "dirty");
    const bool gpuTimerRequested = parseOnOff(args.get("--gpu-timer", "off"), false);

    ensureOutputDir(config.outDir);

    auto terrain = buildTerrain(config.terrain, config.width, config.height);
    if (!terrain || !terrain->getData() || terrain->getData()->empty())
    {
        std::cerr << "Unable to build render benchmark terrain.\n";
        return 1;
    }

    RendererManager renderer(terrain.get());
    renderer.setupTerrainLodCpuOnly();

    if (gpuTimerRequested)
    {
        std::cerr << "GPU timer requested, but headless render benchmarks do not create an OpenGL timing context; "
                     "gpu_frame_ms will be nan.\n";
    }

    std::vector<FrameRow> measuredRows;
    measuredRows.reserve(frames);

    const int totalFrames = config.warmup + frames;
    for (int frame = 0; frame < totalFrames; ++frame)
    {
        FrameRow row = measureRenderFrame(renderer, *terrain, frame, lodEnabled, cullingEnabled, syncMode, true);
        if (frame >= config.warmup)
        {
            row.frameId = frame - config.warmup;
            measuredRows.push_back(row);
        }
    }

    writeFrameCsv(config.outDir / "render_frames.csv", measuredRows);
    writeFrameSummaryCsv(config.outDir / "render_summary.csv", measuredRows);

    std::ofstream meta(config.outDir / "render_config.csv");
    meta << "terrain,width,height,frames,warmup,lod,culling,sync,gpu_timer_requested\n";
    meta << config.terrain << "," << terrain->getTerrainWidth() << "," << terrain->getTerrainHeight() << "," << frames
         << "," << config.warmup << "," << (lodEnabled ? "on" : "off") << "," << (cullingEnabled ? "on" : "off") << ","
         << syncMode << "," << (gpuTimerRequested ? "on" : "off") << "\n";

    std::cout << "Render benchmark written to " << config.outDir << "\n";
    return 0;
}

int runInteractionBenchmark(const CliArgs& args)
{
    CommonConfig config = parseCommon("interaction", args);
    const int frames = args.getInt("--frames", 120);
    const bool lodEnabled = parseOnOff(args.get("--lod", "on"), true);
    const bool cullingEnabled = parseOnOff(args.get("--culling", "on"), true);
    const std::string syncMode = args.get("--sync", "dirty");

    ensureOutputDir(config.outDir);

    auto terrain = buildTerrain(config.terrain, config.width, config.height);
    if (!terrain || !terrain->getData() || terrain->getData()->empty())
    {
        std::cerr << "Unable to build interaction benchmark terrain.\n";
        return 1;
    }

    RendererManager renderer(terrain.get());
    renderer.setupTerrainLodCpuOnly();

    ThermalErosion erosion;
    erosion.loadTerrainInfo(*terrain);
    erosion.setTalusAngle(static_cast<float>(args.getDouble("--talus", 25.0)));
    erosion.setTransferRate(static_cast<float>(args.getDouble("--transfer", 0.1)));

    std::ofstream out(config.outDir / "interaction_frames.csv");
    out << "frame_id,erosion_ms,sync_ms,draw_ms,total_frame_ms,cells_modified,triangles,visible_patches,total_patches,"
           "dirty_patches,fps\n";

    for (int frame = 0; frame < config.warmup + frames; ++frame)
    {
        const auto erosion0 = Clock::now();
        const int cellsModified = erosion.stepBlockedPureTwoPhase();
        const auto erosion1 = Clock::now();

        std::vector<int> dirtyPatches = erosion.getDirtyPatchIndices();
        const auto sync0 = Clock::now();
        if (syncMode == "full")
        {
            renderer.updateVerticesCpuLod();
            dirtyPatches = syntheticDirtyPatches(renderer.getPatches().size(), 0);
            dirtyPatches.resize(renderer.getPatches().size());
            for (std::size_t i = 0; i < dirtyPatches.size(); ++i)
            {
                dirtyPatches[i] = static_cast<int>(i);
            }
        }
        else if (syncMode == "dirty")
        {
            renderer.updateVerticesCpuLod(dirtyPatches);
        }
        const auto sync1 = Clock::now();

        const float width = static_cast<float>(terrain->getTerrainWidth());
        const float height = static_cast<float>(terrain->getTerrainHeight());
        const glm::vec3 cameraPos(width * 0.5f, 220.0f, -250.0f + static_cast<float>(frame % 120) * 2.0f);
        glm::mat4 view =
            glm::lookAt(cameraPos, glm::vec3(width * 0.5f, 0.0f, height * 0.5f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.01f, 5000.0f);

        const auto draw0 = Clock::now();
        const RenderBenchmarkStats stats =
            renderer.collectRenderStats(cameraPos, projection, view, lodEnabled, cullingEnabled);
        const auto draw1 = Clock::now();

        erosion.clearDirtyPatchIndices();

        if (frame < config.warmup)
        {
            continue;
        }

        const double erosionMs = std::chrono::duration<double, std::milli>(erosion1 - erosion0).count();
        const double syncMs = std::chrono::duration<double, std::milli>(sync1 - sync0).count();
        const double drawMs = std::chrono::duration<double, std::milli>(draw1 - draw0).count();
        const double totalMs = erosionMs + syncMs + drawMs;

        out << (frame - config.warmup) << "," << erosionMs << "," << syncMs << "," << drawMs << "," << totalMs << ","
            << cellsModified << "," << stats.triangles << "," << stats.visiblePatches << "," << stats.totalPatches
            << "," << dirtyPatches.size() << "," << (totalMs > 0.0 ? 1000.0 / totalMs : 0.0) << "\n";
    }

    std::cout << "Interaction benchmark written to " << config.outDir << "\n";
    return 0;
}
#else
int runRenderBenchmark(const CliArgs&)
{
    std::cerr << "Render benchmark requires -DEROSION_ENABLE_RENDERING=ON.\n";
    return 1;
}

int runInteractionBenchmark(const CliArgs&)
{
    std::cerr << "Interaction benchmark requires -DEROSION_ENABLE_RENDERING=ON.\n";
    return 1;
}
#endif

#if EROSION_ENABLE_MPI
int runMpiBenchmark(const CliArgs& args)
{
    CommonConfig config = parseCommon("mpi", args);

    MPI_Init(nullptr, nullptr);

    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2)
    {
        if (rank == 0)
        {
            std::cerr << "MPI benchmark requires at least 2 ranks.\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (config.terrain == "midpointDisplacement" && (config.width != config.height || !isPowerOfTwo(config.width - 1)))
    {
        if (rank == 0)
        {
            std::cerr << "midpointDisplacement MPI benchmark requires size 2^n + 1.\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0)
    {
        ensureOutputDir(config.outDir);
    }

    std::unique_ptr<Terrain> terrain;
    std::vector<float> referenceData;
    float* rootData = nullptr;
    float* initialData = nullptr;
    const int terrainSize = config.width * config.height;

    if (rank == 0)
    {
        generateTerrain(terrain, config.width, config.height, config.terrain);
        referenceData = *terrain->getData();
        rootData = terrain->getData()->data();
        initialData = static_cast<float*>(std::malloc(sizeof(float) * terrainSize));
        std::memcpy(initialData, rootData, terrainSize * sizeof(float));
    }

    int* scatterOffset = static_cast<int*>(std::malloc(sizeof(int) * size));
    int* scatterSize = static_cast<int*>(std::malloc(sizeof(int) * size));

    Mesh myTerrain;
    initSplitMesh(rank, size, myTerrain, config.width, config.height, scatterSize, scatterOffset);

    std::ofstream raw;
    if (rank == 0)
    {
        raw.open(config.outDir / "mpi_raw_runs.csv");
        raw << "run_id,is_warmup,terrain,width,height,steps,ranks,total_time_ms,compute_ms,communication_ms,"
               "final_mass_error\n";
    }

    std::vector<double> measuredTotal;
    std::vector<double> measuredCompute;
    std::vector<double> measuredCommunication;
    std::vector<double> measuredMassError;

    const int totalRuns = config.warmup + config.runs;
    for (int run = 0; run < totalRuns; ++run)
    {
        if (rank == 0)
        {
            *terrain->getData() = referenceData;
            rootData = terrain->getData()->data();
            std::memcpy(initialData, rootData, terrainSize * sizeof(float));
        }

        std::memset(myTerrain.meshData, 0, myTerrain.meshBufferSize * sizeof(float));
        std::memset(myTerrain.meshFluxData, 0, myTerrain.meshBufferSize * sizeof(float));
        std::memset(myTerrain.bottomFlux, 0, myTerrain.meshWidth * sizeof(float));
        std::memset(myTerrain.topFlux, 0, myTerrain.meshWidth * sizeof(float));
        std::memset(myTerrain.tempFlux, 0, myTerrain.meshWidth * 2 * sizeof(float));

        MPI_Barrier(MPI_COMM_WORLD);

        double communicationSeconds = 0.0;
        double computeSeconds = 0.0;
        double t0 = MPI_Wtime();
        double phase0 = MPI_Wtime();
        MPI_Scatterv(rootData, scatterSize, scatterOffset, MPI_FLOAT, myTerrain.meshData + myTerrain.meshWidth,
                     scatterSize[rank], MPI_FLOAT, 0, MPI_COMM_WORLD);
        communicationSeconds += MPI_Wtime() - phase0;

        int ghostStartIndex = myTerrain.meshBufferSize - myTerrain.meshWidth;
        int lastLineIndex = myTerrain.meshBufferSize - (myTerrain.meshWidth * 2);
        int tagCpt = run * 100000;
        int nbChanges = 0;

        for (int step = 1; step <= config.steps; ++step)
        {
            phase0 = MPI_Wtime();
            MPI_Sendrecv(myTerrain.meshData, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshTopId, tagCpt,
                         myTerrain.meshData + ghostStartIndex, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshBottomId,
                         tagCpt, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            ++tagCpt;

            MPI_Sendrecv(myTerrain.meshData + lastLineIndex, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshBottomId,
                         tagCpt, myTerrain.meshData, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshTopId, tagCpt,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            ++tagCpt;
            communicationSeconds += MPI_Wtime() - phase0;

            phase0 = MPI_Wtime();
            nbChanges = stepChunkMPIBlockVect2(myTerrain.meshData, myTerrain.meshFluxData, myTerrain.bottomFlux,
                                               myTerrain.topFlux, myTerrain.meshWidth, myTerrain.meshHeight);
            computeSeconds += MPI_Wtime() - phase0;

            std::memset(myTerrain.tempFlux, 0, myTerrain.meshWidth * 2 * sizeof(float));

            phase0 = MPI_Wtime();
            MPI_Sendrecv(myTerrain.topFlux, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshTopId, tagCpt,
                         myTerrain.tempFlux, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshBottomId, tagCpt,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            ++tagCpt;

            MPI_Sendrecv(myTerrain.bottomFlux, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshBottomId, tagCpt,
                         myTerrain.tempFlux + myTerrain.meshWidth, myTerrain.meshWidth, MPI_FLOAT, myTerrain.meshTopId,
                         tagCpt, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            ++tagCpt;
            communicationSeconds += MPI_Wtime() - phase0;

            phase0 = MPI_Wtime();
            transferFluxTopBot(myTerrain.meshData + myTerrain.meshWidth, myTerrain.meshData + lastLineIndex,
                               myTerrain.tempFlux, myTerrain.meshWidth);

            if (myTerrain.meshTopId == MPI_PROC_NULL)
            {
                for (int i = 0; i < myTerrain.meshWidth; ++i)
                {
                    myTerrain.meshData[myTerrain.meshWidth + i] += myTerrain.topFlux[i];
                }
            }

            if (myTerrain.meshBottomId == MPI_PROC_NULL)
            {
                for (int i = 0; i < myTerrain.meshWidth; ++i)
                {
                    myTerrain.meshData[lastLineIndex + i] += myTerrain.bottomFlux[i];
                }
            }

            std::memset(myTerrain.topFlux, 0, myTerrain.meshWidth * sizeof(float));
            std::memset(myTerrain.bottomFlux, 0, myTerrain.meshWidth * sizeof(float));
            computeSeconds += MPI_Wtime() - phase0;
        }

        (void)nbChanges;

        phase0 = MPI_Wtime();
        MPI_Gatherv(myTerrain.meshData + myTerrain.meshWidth, scatterSize[rank], MPI_FLOAT, rootData, scatterSize,
                    scatterOffset, MPI_FLOAT, 0, MPI_COMM_WORLD);
        communicationSeconds += MPI_Wtime() - phase0;

        const double totalSeconds = MPI_Wtime() - t0;

        double maxTotalSeconds = 0.0;
        double maxComputeSeconds = 0.0;
        double maxCommunicationSeconds = 0.0;
        MPI_Reduce(&totalSeconds, &maxTotalSeconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&computeSeconds, &maxComputeSeconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&communicationSeconds, &maxCommunicationSeconds, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

        if (rank == 0)
        {
            const bool warmup = run < config.warmup;
            const double massError = testConservation(initialData, rootData, terrainSize);
            const double totalMs = maxTotalSeconds * 1000.0;
            const double computeMs = maxComputeSeconds * 1000.0;
            const double communicationMs = maxCommunicationSeconds * 1000.0;

            raw << (run + 1) << "," << (warmup ? 1 : 0) << "," << config.terrain << "," << config.width << ","
                << config.height << "," << config.steps << "," << size << "," << totalMs << "," << computeMs << ","
                << communicationMs << "," << massError << "\n";

            if (!warmup)
            {
                measuredTotal.push_back(totalMs);
                measuredCompute.push_back(computeMs);
                measuredCommunication.push_back(communicationMs);
                measuredMassError.push_back(massError);
            }
        }
    }

    if (rank == 0)
    {
        std::ofstream summary(config.outDir / "mpi_summary_stats.csv");
        summary << "terrain,width,height,steps,ranks,metric,n,mean,median,stddev,min,max,ci95_low,ci95_high,p05,p95\n";

        auto writeMetric = [&summary, &config, size](const std::string& metric, const std::vector<double>& values)
        {
            const BenchmarkSummaryStats stats = computeBenchmarkSummaryStats(values);
            summary << config.terrain << "," << config.width << "," << config.height << "," << config.steps << ","
                    << size << "," << metric << "," << stats.n << "," << stats.mean << "," << stats.median << ","
                    << stats.stddev << "," << stats.min << "," << stats.max << "," << stats.ci95Low << ","
                    << stats.ci95High << "," << stats.p05 << "," << stats.p95 << "\n";
        };

        writeMetric("total_time_ms", measuredTotal);
        writeMetric("compute_ms", measuredCompute);
        writeMetric("communication_ms", measuredCommunication);
        writeMetric("final_mass_error", measuredMassError);

        std::cout << "MPI benchmark written to " << config.outDir << "\n";
    }

    std::free(scatterOffset);
    std::free(scatterSize);
    std::free(initialData);

    MPI_Finalize();
    return 0;
}
#else
int runMpiBenchmark(const CliArgs&)
{
    std::cerr << "MPI benchmark requires -DEROSION_ENABLE_MPI=ON.\n";
    return 1;
}
#endif

void printUsage(const char* program)
{
    std::cerr << "Usage: " << program << " bench <erosion|render|interaction|mpi> [options]\n";
    std::cerr << "Common options: --terrain perlinNoise --width 1024 --height 1024 --steps 100 --warmup 3 --runs 10 "
                 "--out DIR\n";
    std::cerr << "Erosion options: --variant blocked --neighbors 8 --threads 4\n";
    std::cerr << "Render options: --lod on --culling on --sync dirty --frames 300 --gpu-timer off\n";
}
} // namespace

namespace Benchmark
{
int run(int argc, char* argv[])
{
    if (argc < 3)
    {
        printUsage(argv[0]);
        return 1;
    }

    const std::string mode = argv[2];
    const CliArgs args(argc, argv, 3);

    if (mode == "erosion")
    {
        return runErosionBenchmark(args);
    }

    if (mode == "render")
    {
        return runRenderBenchmark(args);
    }

    if (mode == "interaction")
    {
        return runInteractionBenchmark(args);
    }

    if (mode == "mpi")
    {
        return runMpiBenchmark(args);
    }

    printUsage(argv[0]);
    return 1;
}
} // namespace Benchmark
