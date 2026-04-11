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
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"
#include "ValidationTest.hpp"
#include "ThermalErosion.hpp"
#include "PerlinNoiseTerrain.hpp"
#include <mpi.h>
#include <fstream>


void saveBinaryHeightmap(const char* nom_fichier, float* heightmap, int largeur, int hauteur) {
    std::ofstream fichier(nom_fichier, std::ios::binary);
    fichier.write(reinterpret_cast<char*>(heightmap), largeur * hauteur * sizeof(float));
}

void savePngHeightmap(const char* nom_fichier, float* heightmap, int largeur, int hauteur) {
    unsigned char* pixels = new unsigned char[largeur * hauteur];
    
    for (int i = 0; i < largeur * hauteur; i++) {
        pixels[i] = (unsigned char)(heightmap[i] * 255);
    }
    
    stbi_write_png(nom_fichier, largeur, hauteur, 1, pixels, largeur);
    delete[] pixels;
}

int stepChunkMPI(float* initialData,float* fluxData,float* bottomFlux,float* topFlux, const int width,const int height)
{
    float transferRate = 0.5;

    const float PI = 3.14159265f;
    float talusAngle = std::tan(30.f * PI / 180.0f);

    const int W = width;
    const int H = height;


    if (!initialData) {
        std::cerr << "Error: Terrain data not loaded in ThermalErosion.\n";
        return 0;
    }

    memset(fluxData,0,sizeof(float)*(H+2)*W);
    int changes = 0;

    // Boucle sur le terrain
    for (int i = 1; i <= H ; i++) {
        for (int j = 1; j < W - 1; j++) {
            float currentHeight = initialData[i * W + j];

            // Hauteurs des 8 voisins
            float diffBottomLeft  = currentHeight - (initialData[(i + 1) * W + (j - 1)]);
            float diffBottom      = currentHeight - (initialData[(i + 1) * W + j]);
            float diffBottomRight = currentHeight - (initialData[(i + 1) * W + (j + 1)]);
            float diffLeft        = currentHeight - (initialData[i * W + (j - 1)]);
            float diffRight       = currentHeight - (initialData[i * W + (j + 1)]);
            float diffTopLeft     = currentHeight - (initialData[(i - 1) * W + (j - 1)]);
            float diffTop         = currentHeight - (initialData[(i - 1) * W + j]);
            float diffTopRight    = currentHeight - (initialData[(i - 1) * W + (j + 1)]);

            // Stockage des différences et indices des voisins (dans le même ordre)
            float dist[8] = { diffBottomLeft, diffBottom, diffBottomRight,
                            diffLeft, diffRight,
                            diffTopLeft, diffTop, diffTopRight };

            int neighbors[8][2] = { 
                {1, -1},  // bas-gauche
                {1, 0},   // bas
                {1, 1},   // bas-droite
                {0, -1},  // gauche
                {0, 1},   // droite
                {-1, -1}, // haut-gauche
                {-1, 0},  // haut
                {-1, 1}   // haut-droite
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
                fluxData[i * W + j] -= materialToMove;

                // Redistribution aux voisins
                for (int k = 0; k < 8; k++) {
                    if (dist[k] > talusAngle) {
                        float proportion = dist[k] / totalDiff;
                        float moveAmount = materialToMove * proportion;

                        int ni = i + neighbors[k][0];
                        int nj = j + neighbors[k][1];

                        if (i == H && ni == H+1) {
                            if (nj >= 0 && nj < width) {
                                bottomFlux[nj] += moveAmount;
                            }else {
                                fluxData[i * W + j] += moveAmount;
                            }
                        } else if (i == 1 && ni == 0) {
                            if (nj >= 0 && nj < width) {
                                topFlux[nj] += moveAmount;
                            }else {
                                fluxData[i * W + j] += moveAmount;
                            }
                        }else if (nj == -1) {
                            fluxData[i * W + j] += moveAmount;
                        } else if (nj == W) {
                            fluxData[i * W + j] += moveAmount;
                        }else {
                            fluxData[ni * W + nj] += moveAmount;   
                        }
                    }
                }

                changes++;
            }
        }
    }

    for (int i = 1; i <= H; i++) {
        for (int j = 0; j < W; j++) {
            initialData[i * W + j] += fluxData[i * W + j];
        }
    }

    return changes;
}

void generateTerrain(std::unique_ptr<Terrain>& terrain,int width,int height)
{
    auto generator = std::make_unique<PerlinNoiseTerrain>();
    generator->CreatePerlinNoise(width, height, 0, 255, 1, 0.005);
    terrain = std::move(generator);
}

double checksum(float* tab,int size)
{
    double sum = 0.;
    for(int i =0;i<size;++i)
    {
        sum +=tab[i];
    }
    return sum;
}

struct Mesh
{
	float* meshData;
    float* meshFluxData;
    
    float* bottomFlux;
    float* topFlux;


    float* tempFlux;
    float* deltaFlux;

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
        meshBufferSize = (meshHeight+2)*(meshWidth);

        meshData = (float*)calloc(meshBufferSize,sizeof(float));
        meshFluxData = (float*)calloc(meshBufferSize,sizeof(float));
        deltaFlux = (float*)calloc(meshBufferSize, sizeof(float));

        bottomFlux = (float*)calloc(meshWidth,sizeof(float));
        topFlux = (float*)calloc(meshWidth,sizeof(float));

        tempFlux = (float*)calloc(meshWidth*2,sizeof(float));

        meshTopId = topId;
        meshBottomId = botId;
    }
    
    ~Mesh()
    {
        free(meshData);
        free(meshFluxData);

        free(bottomFlux);
        free(topFlux); 
           
        free(tempFlux);  
        free(deltaFlux);
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

void horizontal_Comm(int targetRank,int tag,COMM comm,float* src,int width,int paddingSend,int paddingRecv)
{
    if (targetRank == -1) {
    return;
    }

    MPI_Status status;
    switch (comm)
    {
        case SEND:
        {

            MPI_Send(src+paddingSend, width, MPI_FLOAT, targetRank, tag, MPI_COMM_WORLD);
            break;
        }
        case RECV:
        {

            MPI_Recv(src+paddingRecv, width, MPI_FLOAT, targetRank, tag, MPI_COMM_WORLD, &status);
            break;
        }
    
        default:
            break;
    }
}

double testConservation(float* initialData,float* finalData,int size) {
    if(!initialData || !finalData)
    {
        printf("intialData or finalData is null ! \n");
        return 1;
    }

    double mass_before = checksum(initialData, size);
    double mass_after = checksum(finalData, size);
    printf("Mass before: %.10f, Mass after: %.10f\n", mass_before, mass_after);
    if (mass_before == 0) return 0;
    return std::abs(mass_after - mass_before) / mass_before;
}


void transferFluxTopBot(float* top,float* bot,float* flux,int size)
{
    for(int i=0;i<size;i++)
    {
        top[i] += flux[i];
        bot[i] += flux[size+i];
    }

    memset(flux,0,sizeof(float)*size*2);
        
}

int main(int argc, char **argv)
{

    int rank,size,sizeRecvBuf,nbChanges;
    
    float* data = nullptr;

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
    int terrainSize = terrainHeight*terrainWidth;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(size <= 1)
    {
        printf("You must launch this code with atleast 2 processors \n");
        return 1;
    }

    float* initialData = nullptr;

    if (rank == 0){
        
        generateTerrain(terrain,terrainWidth,terrainHeight);
        erosion.loadTerrainInfo(terrain);

        data = terrain->getData()->data();

        initialData = (float*)malloc(sizeof(float)*terrainHeight*terrainWidth);
        memcpy(initialData,data,terrainSize*sizeof(float));

    }


    Mesh myTerrain;
    initSplitMesh(rank,size,myTerrain,terrainWidth,terrainHeight);    

    MPI_Scatter(data,
        myTerrain.meshSize,
        MPI_FLOAT,
        myTerrain.meshData+myTerrain.meshWidth,
        myTerrain.meshSize,
        MPI_FLOAT,
        0,
        MPI_COMM_WORLD
    );


    if (rank == 0)
    {
        savePngHeightmap("MPI_heightmap_before.png",data,terrainWidth,terrainHeight);
    }

    int ghostStartIndex = myTerrain.meshBufferSize - myTerrain.meshWidth;
    int lastLineIndex = myTerrain.meshBufferSize - (myTerrain.meshWidth*2);
    int tagCpt = 0;

    for(int i=1; i <= terrainStep ; ++i)
    {

        horizontal_Comm(myTerrain.meshTopId,tagCpt,COMM::SEND,myTerrain.meshData,myTerrain.meshWidth,myTerrain.meshWidth,0);
        horizontal_Comm(myTerrain.meshBottomId,tagCpt,COMM::RECV,myTerrain.meshData,myTerrain.meshWidth,0,ghostStartIndex);
        tagCpt++;
 

        horizontal_Comm(myTerrain.meshBottomId,tagCpt,COMM::SEND,myTerrain.meshData,myTerrain.meshWidth,lastLineIndex,0);
        horizontal_Comm(myTerrain.meshTopId,tagCpt,COMM::RECV,myTerrain.meshData,myTerrain.meshWidth,0,0);
        tagCpt++;
        
        MPI_Barrier(MPI_COMM_WORLD);

        nbChanges = stepChunkMPI(myTerrain.meshData,myTerrain.meshFluxData,myTerrain.bottomFlux,myTerrain.topFlux,myTerrain.meshWidth,myTerrain.meshHeight);

        memset(myTerrain.tempFlux, 0, myTerrain.meshWidth * 2 * sizeof(float));
        // On transfert les changement à faire sur la dernière ligne du terrain
        
        horizontal_Comm(myTerrain.meshTopId,tagCpt,COMM::SEND,myTerrain.topFlux,myTerrain.meshWidth,0,0);
        horizontal_Comm(myTerrain.meshBottomId,tagCpt,COMM::RECV,myTerrain.tempFlux,myTerrain.meshWidth,0,0);
        tagCpt++;

        // On transfert les changements à faire sur le première ligne du terrain
        horizontal_Comm(myTerrain.meshBottomId,tagCpt,COMM::SEND,myTerrain.bottomFlux,myTerrain.meshWidth,0,0);
        horizontal_Comm(myTerrain.meshTopId,tagCpt,COMM::RECV,myTerrain.tempFlux+myTerrain.meshWidth,myTerrain.meshWidth,0,0);
        tagCpt++;

        transferFluxTopBot(myTerrain.meshData+myTerrain.meshWidth,myTerrain.meshData+lastLineIndex,myTerrain.tempFlux,myTerrain.meshWidth);


        if (myTerrain.meshTopId == -1){
            for(int i =0; i< myTerrain.meshWidth; ++i)
                myTerrain.meshData[myTerrain.meshWidth + i] += myTerrain.topFlux[i];
        }

        if (myTerrain.meshBottomId == -1){
            for(int i =0; i< myTerrain.meshWidth; ++i)
                myTerrain.meshData[lastLineIndex+i] += myTerrain.bottomFlux[i];
        }

        memset(myTerrain.topFlux, 0, myTerrain.meshWidth * sizeof(float));
        memset(myTerrain.bottomFlux, 0, myTerrain.meshWidth * sizeof(float));

        printf("[%d][%d/%d] changes : %d \n",rank,i,terrainStep,nbChanges);
    }


    MPI_Gather(myTerrain.meshData+myTerrain.meshWidth,myTerrain.meshSize,MPI_FLOAT,data,myTerrain.meshSize,MPI_FLOAT,0,MPI_COMM_WORLD);

    if (rank == 0)
    {
        //saveBinaryHeightmap("MPI_heightmap2.raw",data,terrainWidth,terrainHeight);
        savePngHeightmap("MPI_heightmap_After.png",data,terrainWidth,terrainHeight);

        printf("Error : %f\n",testConservation(initialData,data,terrainSize));
    }


    MPI_Finalize();
}
