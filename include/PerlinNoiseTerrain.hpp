#pragma once

#include "Terrain.hpp"
#include <vector>

/**
 * @class PerlinNoiseTerrain
 * @brief Generates terrain using classic 2D Perlin noise.
 *
 * This class produces a heightmap by evaluating a Perlin noise function
 * over a 2D grid. Supports customizable frequency, octaves, persistence
 * and lacunarity.
 */
class PerlinNoiseTerrain : public Terrain
{
public:
    /**
     * @brief Default constructor.
     */
    PerlinNoiseTerrain();

    /**
     * @brief Generates terrain using 2D Perlin noise.
     *
     * @param width Width of the grid.
     * @param height Height of the grid.
     * @param minHeight Minimum terrain height.
     * @param maxHeight Maximum terrain height.
     * @param scale Global scale multiplier on the terrain.
     * @param frequency Base frequency of the Perlin noise.
     * @param octaves Number of noise octaves.
     * @param persistence Amplitude damping factor for each octave.
     * @param lacunarity Frequency multiplier for each octave.
     */
    void CreatePerlinNoise(int width, int height,
                           float minHeight, float maxHeight,
                           float scale = 1.0f,
                           float frequency = 0.005f,
                           int octaves = 4,
                           float persistence = 0.5f,
                           float lacunarity = 2.0f);

private:
    float mBaseFrequency;
    int mNumOctaves;
    float mPersistenceCoef;
    float mLacunarityCoef;

    std::vector<int> mPermutation;

    /// Initialize the permutation table (size 512).
    void InitPermutation();

    /// Computes 2D Perlin noise at coordinates (x, y).
    float Noise2D(float x, float y) const;

    /// Compute gradient vector from hash value.
    void Gradient(int hash, float& gx, float& gy) const;

    /// Fade function (6t^5 - 15t^4 + 10t^3).
    inline float Fade(float t) const;

    /// Linear interpolation.
    inline float Lerp(float a, float b, float t) const;

    /// Internal generation function.
    void CreatePerlinNoiseInternal(float minH, float maxH);
    
    /**
     * @brief Normalizes terrain heights to fit within minHeight and maxHeight.
     *
     * Adjusts all values in the terrain grid proportionally to match the
     * defined minimum and maximum height interval.
     */
    void Normalize();
};
