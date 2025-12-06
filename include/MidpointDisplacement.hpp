#pragma once
#include "Terrain.hpp"

/**
 * @class MidpointDisplacement
 * @brief Generates terrain using the midpoint displacement algorithm.
 *
 * This class produces a heightmap by recursively subdividing a grid
 * and displacing midpoints to create realistic fractal terrain. The
 * algorithm consists of alternating "diamond" and "square" steps.
 */
class MidpointDisplacement : public Terrain
{
public:
    /**
     * @brief Default constructor.
     *
     * Initializes the terrain object without setting width, height, or data.
     */
    MidpointDisplacement();

    /**
     * @brief Generates a terrain using the midpoint displacement algorithm.
     *
     * The terrain heights are recursively modified by subdividing the grid
     * and displacing midpoint values. The roughness factor controls the
     * variation at each subdivision.
     *
     * @param size Size of the terrain grid (should be 2^n + 1 for proper subdivision).
     * @param minHeight Minimum allowed height value of the terrain.
     * @param maxHeight Maximum allowed height value of the terrain.
     * @param scale Scale factor applied to the XZ plane (default = 1.0f, interval >0).
     * @param roughness Controls the roughness of the terrain (default = 1, interval >0).
     */
    void CreateMidpointDisplacement(int size, float minHeight, float maxHeight, float scale = 1.0f, float roughness = 1);

private:
    /**
     * @brief Internal recursive implementation of the midpoint displacement algorithm.
     *
     * @param roughness Roughness factor controlling terrain variation.
     */
    void CreateMidpointDisplacementInterne(float roughness);

    /**
     * @brief Normalizes terrain heights to fit within minHeight and maxHeight.
     *
     * Adjusts all values in the terrain grid proportionally to match the
     * defined minimum and maximum height interval.
     */
    void Normalize();

    /**
     * @brief Performs the diamond step of the midpoint displacement.
     *
     * @param rectSize Current size of the sub-square being processed.
     * @param curHeight Current height displacement for this iteration.
     */
    void DiamondStep(int rectSize, float curHeight);

    /**
     * @brief Performs the square step of the midpoint displacement.
     *
     * @param rectSize Current size of the sub-square being processed.
     * @param curHeight Current height displacement for this iteration.
     */
    void SquareStep(int rectSize, float curHeight);
};
