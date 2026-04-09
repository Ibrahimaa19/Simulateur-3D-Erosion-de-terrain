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


int stepChunkMPI(float* inData,float* bottomFlux,const int width,const int height)
{
    float transferRate = 0.1;

    const float PI = 3.14159265f;
    float talusAngle = std::tan(25.f * PI / 180.0f);

    const int W = width;
    const int H = height;

    if (!inData) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    //float* data = inData;// Copie du pointeur
        
    int changes = 0;

    // Boucle sur le terrain
    for (int i = 1; i < H ; i++) {
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
                {0, -1}, {-1, -1},{-1, 0}, {1, 0}, {0, 1},{1, -1},{-1, 1}, {1, 1}    // 4 voisins diagonaux
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
                inData[i * W + j] -= materialToMove;

                // Redistribution aux voisins
                for (int k = 0; k < 8; k++) {
                    if (dist[k] > talusAngle) {
                        float proportion = dist[k] / totalDiff;
                        float moveAmount = materialToMove * proportion;

                        int ni = i + neighbors[k][0];
                        int nj = j + neighbors[k][1];

                        if (i == H-1 && k < 3)
                            bottomFlux[j -1 +k] += moveAmount;
                        else
                            inData[ni * W + nj] += moveAmount;
                    }
                }

                changes++;
            }
        }
    }

    return changes;
}

void generateTerrain(std::unique_ptr<Terrain>& terrain,int width,int height)
{
    auto generator = std::make_unique<FaultFormationTerrain>();
    generator->CreateFaultFormation(width, height, 1000, 0, 255, 1);
    terrain = std::move(generator);
}

float checksum(float* tab,int size)
{
    float sum = 0;
    for(int i =0;i<size;++i)
    {
        sum +=tab[i];
    }
    return sum;
}

struct Mesh
{
	float* meshData;
    float* meshDataTemp;
    float* bottomFlux;

	int meshWidth;
	int meshHeight;

    int meshSize;
    int meshBufferSize;

    int meshTopId;
    int meshBottomId;
	
    void initMesh(int width,int height,int topId, int botId)
    {
        meshWidth = width;
        meshHeight = height;

        meshSize = meshHeight*meshWidth;
        meshBufferSize = (meshHeight+2)*meshWidth;

        meshData = (float*)malloc(sizeof(float)*meshBufferSize);
        meshDataTemp = (float*)malloc(sizeof(float)*meshBufferSize);
        bottomFlux = (float*)malloc(sizeof(float)*meshWidth);

        meshTopId = topId;
        meshBottomId = botId;
    }

    void transferFlux()
    {
        for(int i=0;i<meshWidth;i++)
            meshData[i] += bottomFlux[i];
    }
    
    ~Mesh()
    {
        free(meshData);
        free(meshDataTemp);
        free(bottomFlux);
    }
};

void initSplitMesh(int rank,int size,Mesh& mesh,int terrainWidth,int terrainHeight)
{
    int sizeBlock = terrainHeight/size;
    int sizeBlockRest = terrainHeight%size;
    
    int meshHeight = sizeBlock;
    int meshWidth = terrainWidth;

    if (rank == 0)
    {
        mesh.initMesh(meshWidth,meshHeight,-1,rank+1);
    }
    else if (rank == size-1)
    {
        mesh.initMesh(meshWidth,meshHeight+sizeBlockRest,rank-1,-1);
    }
    else
    {
        mesh.initMesh(meshWidth,meshHeight,rank-1,rank+1);
    }
}

enum COMM
{
    SEND,
    RECV
};

void horizontaleComm(int targetRank,COMM comm,Mesh& mesh,int paddingSend,int paddingRecv,bool bottomFlux)
{

    if (targetRank == -1) {
    return;
    }

    MPI_Status status;
    switch (comm)
    {
        case SEND:
        {
            if (!bottomFlux)
                MPI_Send(mesh.meshData+paddingSend, mesh.meshWidth, MPI_FLOAT, targetRank, 0, MPI_COMM_WORLD);
            else
                MPI_Send(mesh.bottomFlux, mesh.meshWidth, MPI_FLOAT, targetRank, 0, MPI_COMM_WORLD);

            break;
        }
        case RECV:
        {

            MPI_Recv(mesh.meshData+paddingRecv, mesh.meshWidth, MPI_FLOAT, targetRank, 0, MPI_COMM_WORLD, &status);

            break;
        }
    
        default:
            break;
    }
}


int main(int argc, char **argv)
{

    int rank,size,sizeRecvBuf,nbChanges;
    
    double ma_var;

    float* data;

    ThermalErosion erosion;
    std::unique_ptr<Terrain> terrain;

    if (argc < 4)
    {
        printf("usage : erosion <width> <height> <step>\n");
        return 1;
    }

    int terrainWidth = atoi(argv[1]);
    int terrainHeight = atoi(argv[2]);
    int terrainStep = atoi(argv[3]);

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0){
        
        generateTerrain(terrain,terrainWidth,terrainHeight);
        erosion.loadTerrainInfo(terrain);

        printf("data: %d\n",terrain->getData()->size());
        data = terrain->getData()->data();

    }


    Mesh myTerrain;
    initSplitMesh(rank,size,myTerrain,terrainWidth,terrainHeight);    

    MPI_Scatter(data,
        myTerrain.meshSize,
        MPI_FLOAT,
        myTerrain.meshData+myTerrain.meshWidth,
        myTerrain.meshHeight*myTerrain.meshWidth,
        MPI_FLOAT,
        0,
        MPI_COMM_WORLD
    );

    memccpy(
        myTerrain.meshDataTemp,
        myTerrain.meshData,
        myTerrain.meshBufferSize,
        myTerrain.meshBufferSize
    );

    int lastLineIndex = myTerrain.meshBufferSize - myTerrain.meshWidth;


    horizontaleComm(myTerrain.meshTopId,COMM::SEND,myTerrain,0,lastLineIndex,false);
    horizontaleComm(myTerrain.meshBottomId,COMM::RECV,myTerrain,0,lastLineIndex,false);

    MPI_Barrier(MPI_COMM_WORLD);

    horizontaleComm(myTerrain.meshBottomId,COMM::SEND,myTerrain,0,lastLineIndex,false);
    horizontaleComm(myTerrain.meshTopId,COMM::RECV,myTerrain,0,lastLineIndex,false);

    MPI_Barrier(MPI_COMM_WORLD);

    for(int i=0;i< terrainStep ; ++i)
    {
        nbChanges = stepChunkMPI(myTerrain.meshData,myTerrain.bottomFlux,myTerrain.meshWidth,myTerrain.meshHeight);
        printf("[%d][%d/%d] changes : %d \n",rank,i,terrainStep,nbChanges);

        horizontaleComm(myTerrain.meshBottomId,COMM::SEND,myTerrain,0,lastLineIndex,true);
        horizontaleComm(myTerrain.meshTopId,COMM::RECV,myTerrain,0,0,true);
        MPI_Barrier(MPI_COMM_WORLD);

        myTerrain.transferFlux();

        horizontaleComm(myTerrain.meshTopId,COMM::SEND,myTerrain,0,lastLineIndex,false);
        horizontaleComm(myTerrain.meshBottomId,COMM::RECV,myTerrain,0,lastLineIndex,false);

    }


    MPI_Gather(myTerrain.meshData,myTerrain.meshSize,MPI_FLOAT,data,myTerrain.meshSize,MPI_FLOAT,0,MPI_COMM_WORLD);

    MPI_Finalize();


}
