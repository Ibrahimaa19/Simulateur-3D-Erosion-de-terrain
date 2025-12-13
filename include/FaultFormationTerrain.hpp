#pragma once 

#include "Terrain.hpp"

/**
 * @class FaultFormationTerrain
 * @brief Generates terrain using the fault formation algorithm.
 *
 * This class produces a heightmap by repeatedly applying random
 * fault lines on a terrain grid. Each iteration displaces points
 * based on their position relative to the fault line. An optional
 * FIR (Finite Impulse Response) filter can be applied to smooth
 * the terrain after generation.
 */
class FaultFormationTerrain : public Terrain
{
public: 
    /**
     * @brief Default constructor.
     *
     * Initializes the terrain object without setting width, height, or data.
     */
    FaultFormationTerrain();
    
    /**
     * @brief Generates a terrain using the fault formation algorithm.
     *
     * The terrain heights are iteratively modified by randomly generated
     * fault lines. Optionally, a FIR filter can smooth the terrain.
     *
     * @param width Width of the terrain grid in number of points.
     * @param height Height of the terrain grid in number of points.
     * @param iterations Number of fault iterations to apply (>=1).
     * @param minHeight Minimum allowed height value of the terrain.
     * @param maxHeight Maximum allowed height value of the terrain.
     * @param scale Scale factor applied to the XZ plane (default = 1.0f, interval >0).
     * @param applyFilter Whether to apply the FIR smoothing filter (default = false).
     * @param filter FIR filter coefficient (default = 0.5f, interval [0.0 ; 1.0]).
     */
    void CreateFaultFormation(int width, int height, int iterations, float minHeight, float maxHeight, float scale = 1.0f, bool applyFilter = false, float filter = 0.5f);

private:
    /**
     * @struct TerrainPoint
     * @brief Represents a single point in the terrain grid.
     */
    struct TerrainPoint
    {
        int x = 0; ///< X coordinate in the terrain grid
        int z = 0; ///< Z coordinate in the terrain grid
        
        /**
         * @brief Compares this point to another point.
         * @param p The point to compare with.
         * @return True if both points have identical X and Z coordinates.
         */
        bool IsEqual(TerrainPoint& p) const;
    };

    /**
     * @brief Internal function implementing the fault formation algorithm.
     *
     * Iteratively displaces terrain heights based on randomly generated
     * fault lines. Optionally applies FIR filtering after iterations.
     *
     * @param iterations Number of iterations to apply (>=1).
     * @param minHeight Minimum allowed height value.
     * @param maxHeight Maximum allowed height value.
     * @param applyFilter Whether to apply FIR smoothing after generation.
     * @param filter FIR filter coefficient (interval [0.0 ; 1.0]).
     */
    void CreateFaultFormationInternal(int iterations, float minHeight, float maxHeight, bool applyFilter, float filter);

    /**
     * @brief Generates two random points defining a fault line.
     *
     * Ensures that the two points are distinct.
     *
     * @param p1 Output parameter for the first random point.
     * @param p2 Output parameter for the second random point.
     */
    void GenRandomTerrainPoints(TerrainPoint& p1, TerrainPoint& p2);

    /**
     * @brief Normalizes terrain heights to fit within minHeight and maxHeight.
     *
     * Adjusts all values in the terrain grid proportionally to match the
     * defined minimum and maximum height interval.
     */
    void Normalize();

    /**
     * @brief Applies a FIR filter to smooth the terrain.
     *
     * This method applies the filter in all four directions: left-to-right,
     * right-to-left, top-to-bottom, and bottom-to-top.
     *
     * @param filter FIR filter coefficient (interval [0.0 ; 1.0]).
     */
    void ApplyFIRFilter(float filter);

    /**
     * @brief Applies the FIR filter to a single terrain point.
     *
     * Updates the height of the point using the previous filtered value
     * and the given filter coefficient.
     *
     * @param x X coordinate of the terrain point.
     * @param z Z coordinate of the terrain point.
     * @param PrevFractalVal Height value from the previous iteration.
     * @param filter FIR filter coefficient (interval [0.0 ; 1.0]).
     * @return The new filtered height value at the specified point.
     */
    float FIRFilterSinglePoint(int x, int z, float PrevFractalVal, float filter);
};
