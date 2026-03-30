// #define STB_IMAGE_IMPLEMENTATION

// #include "TerrainApp.hpp"
// #include "ValidationTest.hpp"
// #include "ThermalErosion.hpp"
// #include <map>
// #include <string>
// #include <memory>

// enum class State{
//     Render,
//     Test,
// };

// enum class Heightmap{
//     LoadHeightmap, 
//     FaultFormation, 
//     MidpointDisplacement, 
//     PerlinNoise
// };

// std::map<std::string, State> dicState{{"render", State::Render}, {"test", State::Test}};
// std::map<std::string, Heightmap> dicHeightmap{{"loadHeightmap", Heightmap::LoadHeightmap}, {"faultFormation", Heightmap::FaultFormation}
//                                                 ,{"midpointDisplacement", Heightmap::MidpointDisplacement}, {"perlinNoise", Heightmap::PerlinNoise} };

// int main(int argc, char const *argv[])
// {

//     if(argc == 1 || State::Render == dicState[argv[1]]){
//         TerrainApp app;
//         app.Init();
//         app.Run();
//     }
//     else if (State::Test == dicState[argv[1]]) {

//         std::unique_ptr<Terrain> terrain;

//         if (argc < 4) {
//             std::cerr << "Usage: " << argv[0]
//                     << " test <typeTerrain> <steps>\n";
//             std::cerr << "<typeTerrain> : loadHeightmap | faultFormation | midpointDisplacement | perlinNoise\n";
//             exit(1);
//         }

//         std::string terrainType = argv[2];
//         int steps = std::atoi(argv[3]);

//         if (steps <= 0) {
//             std::cerr << "Erreur: steps doit être strictement positif\n";
//             exit(1);
//         }

//         switch (dicHeightmap[terrainType])
//         {
//         case Heightmap::LoadHeightmap:
//             terrain = std::make_unique<Terrain>();
//             terrain->loadTerrain("../src/heightmap/iceland_heightmap.png", 1.0f, 100.0f);
//             break;
//         case Heightmap::FaultFormation:
//             {
//                 auto generator = std::make_unique<FaultFormationTerrain>();
//                 generator->CreateFaultFormation(2048, 2048, 1000, 0, 255, 1);
//                 terrain = std::move(generator);
//                 break;
//             } // 2048 , 4096 , 8192
            
//         case Heightmap::MidpointDisplacement:
//             {
//                 auto generator = std::make_unique<MidpointDisplacement>();
//                 generator->CreateMidpointDisplacement(std::pow(2, 11) + 1, 0, 255, 1, 0.5);
//                 terrain = std::move(generator);
//                 break;
//             }
//         case Heightmap::PerlinNoise:
//             {
//                 auto generator = std::make_unique<PerlinNoiseTerrain>();
//                 generator->CreatePerlinNoise(2048, 2048, 0, 255, 1, 0.005);
//                 terrain = std::move(generator);
//                 break;
//             }
//         default:
//             std::cerr << "Heightmap non prise en charge" << std::endl;
//             exit(1);
//             break;
//         }
//         ValidationTest::run_all_tests(terrain, terrainType, steps);
//     }
//     else{
//         fprintf(stdout, "Usage: %s render\n", argv[0]);
//         fprintf(stdout, "Usage: %s test <typeTerrain>\n", argv[0]);
//         fprintf(stdout, "<typeTerrain> : loadHeightmap | faultFormation | midpointDisplacement | perlinNoise");
//     }
//     return 0;
// }



#define STB_IMAGE_IMPLEMENTATION

#include "ValidationTest.hpp"
#include "ThermalErosion.hpp"
#include "FaultFormationTerrain.hpp"
#include <mpi.h>


int stepChunkMPI(float* inData,float* outData,const int width,const int height)
{
    float transferRate = 0.2;
    const int talusAngle = 30;
    const int W = width;
    const int H = height;

    if (!inData) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    float* data = inData;// Copie du pointeur
        
    int changes = 0;

    // Boucle sur le terrain
    for (int i = 1; i < H - 1; i++) {
        for (int j = 1; j < W - 1; j++) {

            //printf("test, indata[%d * %d + %d]: %d \n",i,W,j,i * W + j);
            float currentHeight = inData[i * W + j];

            // Hauteurs des 8 voisins (Moore neighborhood)
            float diffUp         = currentHeight - inData[(i - 1) * W + j];
            float diffDown       = currentHeight - inData[(i + 1) * W + j];
            float diffLeft       = currentHeight - inData[i * W + (j - 1)];
            float diffRight      = currentHeight - inData[i * W + (j + 1)];
            float diffUpLeft     = currentHeight - inData[(i - 1) * W + (j - 1)];
            float diffUpRight    = currentHeight - inData[(i - 1) * W + (j + 1)];
            float diffDownLeft   = currentHeight - inData[(i + 1) * W + (j - 1)];
            float diffDownRight  = currentHeight - inData[(i + 1) * W + (j + 1)];

            // Stockage des différences et indices des voisins
            float dist[8] = { diffUp, diffDown, diffLeft, diffRight,
                              diffUpLeft, diffUpRight, diffDownLeft, diffDownRight };
            
            int neighbors[8][2] = { 
                {-1, 0}, {1, 0}, {0, -1}, {0, 1},     // 4 voisins directs
                {-1, -1}, {-1, 1}, {1, -1}, {1, 1}    // 4 voisins diagonaux
            };

            float totalDiff = 0.0f;
            int validNeighbors = 0;

            // Accumulation des différences valides
            for (int k = 0; k < 8; k++) {
                if (dist[k] > talusAngle) {
                    totalDiff += dist[k];
                    validNeighbors++;
                }
            }

            // Érosion
            if (totalDiff > 0 && validNeighbors > 0) {
                
                float materialToMove = transferRate * (totalDiff / validNeighbors);
                materialToMove = std::min(materialToMove, currentHeight * transferRate);

                // On retire la matière de la cellule actuelle
                outData[i * W + j] -= materialToMove;

                // Redistribution aux voisins
                for (int k = 0; k < 8; k++) {
                    if (dist[k] > talusAngle) {
                        float proportion = dist[k] / totalDiff;
                        float moveAmount = materialToMove * proportion;

                        int ni = i + neighbors[k][0];
                        int nj = j + neighbors[k][1];

                        outData[ni * W + nj] += moveAmount;
                    }
                }

                changes++;
            }
        }
    }

    return changes;
}

void generateTerrain(std::unique_ptr<Terrain>& terrain)
{
    auto generator = std::make_unique<FaultFormationTerrain>();
    generator->CreateFaultFormation(512, 512, 1000, 0, 255, 1);
    terrain = std::move(generator);
}

int main(int argc, char **argv)
{

    int rang,sizeRecvBuf,nbChanges;
    int step = 1;
    double ma_var;

    float* data;
    float* recvBuf;
    float* sendBuf;

    ThermalErosion erosion;
    std::unique_ptr<Terrain> terrain;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rang);

    if (rang == 0){
        
        generateTerrain(terrain);
        printf("data: %d\n",terrain->getData()->size());
        sizeRecvBuf = terrain->getData()->size()/2;
        data = terrain->getData()->data();

    }

    MPI_Bcast(&sizeRecvBuf,1,MPI_INT,0,MPI_COMM_WORLD);

    recvBuf = (float*)malloc(sizeof(float)*sizeRecvBuf);
    sendBuf = (float*)malloc(sizeof(float)*sizeRecvBuf);

    for(int i=0;i< step ; ++i)
    {
        
        MPI_Scatter(data,sizeRecvBuf,MPI_FLOAT,recvBuf,sizeRecvBuf,MPI_FLOAT,0,MPI_COMM_WORLD);

        nbChanges = stepChunkMPI(recvBuf,sendBuf,512/2,512/2);
        
        printf("changes : %d \n",nbChanges);

    }
    

    free(recvBuf);
    free(sendBuf);

    MPI_Finalize();

}