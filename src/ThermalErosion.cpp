#include "ThermalErosion.hpp"
#include <iostream>

int ThermalErosion::step()
{
    if (!m_data) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    return runThermalErosionStep(*m_data,
                                 m_width,
                                 m_height,
                                 talusAngle,
                                 transferRate,
                                 ThermalNeighborMode::Eight).modifiedCells;
}
