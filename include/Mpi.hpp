
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"
#include "PerlinNoiseTerrain.hpp"
#include "FaultFormationTerrain.hpp"
#include "MidpointDisplacement.hpp"
#include "ThermalErosion.hpp"
#include <mpi.h>
#include <fstream>
#include <chrono>
#include <numeric>


void savePngHeightmap(const char* nom_fichier, float* heightmap, int largeur, int hauteur) {
    unsigned char* pixels = new unsigned char[largeur * hauteur];
    
    for (int i = 0; i < largeur * hauteur; i++) {
        pixels[i] = (unsigned char)(heightmap[i] * 255);
    }
    
    stbi_write_png(nom_fichier, largeur, hauteur, 1, pixels, largeur);
    delete[] pixels;

    printf("La heightmap a été enregistré avec succès, sous le nom %s \n",nom_fichier);
}

static inline bool isPowerOfTwo(int n) {
    return n > 0 && (n & (n - 1)) == 0;
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
                                fluxData[i * W + j] += moveAmount; // diagonale
                            }
                        } else if (i == 1 && ni == 0) {
                            if (nj >= 0 && nj < width) {
                                topFlux[nj] += moveAmount;
                            }else {
                                fluxData[i * W + j] += moveAmount; // diagonale
                            }
                        }else if (nj == -1) {
                            fluxData[i * W + j] += moveAmount; // diagonale
                        } else if (nj == W) {
                            fluxData[i * W + j] += moveAmount; // diagonale
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

void generateTerrain(std::unique_ptr<Terrain>& terrain,int width,int height,std::string terrainType)
{
    if (terrainType == "faultFormation")
    {
        auto generator = std::make_unique<FaultFormationTerrain>();
        generator->CreateFaultFormation(width, height, 1000, 0, 255, 1);
        terrain = std::move(generator);

    }else if (terrainType == "midpointDisplacement")
    {   
        auto generator = std::make_unique<MidpointDisplacement>();
        generator->CreateMidpointDisplacement(width, 0, 255, 1, 0.5);
        terrain = std::move(generator);

    }else{
        if (terrainType != "perlinNoise")
            printf("Default terrain : PerlinNoise \n");

        auto generator = std::make_unique<PerlinNoiseTerrain>();
        generator->CreatePerlinNoise(width, height, 0, 255, 1, 0.005);
        terrain = std::move(generator);
    }

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

enum COMM
{
    SEND,
    RECV
};

struct Mesh
{
	float* meshData;
    float* meshFluxData;
    
    float* bottomFlux;
    float* topFlux;
    float* tempFlux;

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
    }
};

void initSplitMesh(int rank,int sizeProc,Mesh& mesh,int terrainWidth,int terrainHeight,int* sizes,int* offsets)
{
    int sizeBlock = terrainHeight/sizeProc;
    int sizeBlockRest = terrainHeight%sizeProc;
    
    int meshHeight = sizeBlock;
    int meshWidth = terrainWidth;

    int meshNbElement = meshHeight*meshWidth;

    if (rank == 0)
    {
        mesh.initMesh(meshWidth,meshHeight,-1,rank+1);
    }
    else if (rank == sizeProc-1)
    {
        mesh.initMesh(meshWidth,meshHeight+sizeBlockRest,rank-1,-1);
    }
    else
    {
        mesh.initMesh(meshWidth,meshHeight,rank-1,rank+1);
    }

    for(int i =0;i<sizeProc;++i)
    {
        if (i== sizeProc-1)
            sizes[i] = meshNbElement + sizeBlockRest*meshWidth;
        else
            sizes[i] = meshNbElement;
    }


    for(int i=0;i<sizeProc;++i)
        offsets[i] = i*meshNbElement;
}

void horizontal_Comm(int targetRank,int tag,COMM comm,float* src,int width,int paddingSend,int paddingRecv)
{
    if (targetRank == -1) {
    return;
    }

    MPI_Status status;
    switch (comm)
    {
        case SEND:
            MPI_Send(src+paddingSend, width, MPI_FLOAT, targetRank, tag, MPI_COMM_WORLD);
            break;
        
        case RECV:
            MPI_Recv(src+paddingRecv, width, MPI_FLOAT, targetRank, tag, MPI_COMM_WORLD, &status);
            break;
    
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
    //printf("Mass before: %.10f, Mass after: %.10f\n", mass_before, mass_after);
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

int lauchMPI(int argc, char *argv[])
{

    int rank,size,nbChanges;
    
    float* data = nullptr;

    ThermalErosion erosion;
    std::unique_ptr<Terrain> terrain;


    std::string terrainType = argv[2];
    int terrainWidth = atoi(argv[3]);
    int terrainHeight = atoi(argv[4]);
    int terrainStep = atoi(argv[5]);
    
    int terrainSize = terrainHeight*terrainWidth;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(size < 2)
    {
        printf("You must launch this code with atleast 2 processors \n");
        return 1;
    }

    float* initialData = nullptr;

    double elapsedAll[size];

    if (terrainType == "midpointDisplacement" && (terrainWidth != terrainHeight || !isPowerOfTwo(terrainWidth - 1))){
        perror("Invalid terrain size : (size must be 2^n + 1)");
        return 1;
    }

    if (rank == 0){
        
        generateTerrain(terrain,terrainWidth,terrainHeight,terrainType);
        erosion.loadTerrainInfo(terrain);

        data = terrain->getData()->data();

        initialData = (float*)malloc(sizeof(float)*terrainHeight*terrainWidth);
        memcpy(initialData,data,terrainSize*sizeof(float));
    }


    int* scatterOffset = (int*)malloc(sizeof(int)*size);
    int* scatterSize = (int*)malloc(sizeof(int)*size);

    Mesh myTerrain;
    initSplitMesh(rank,size,myTerrain,terrainWidth,terrainHeight,scatterSize,scatterOffset);    


    auto start = MPI_Wtime();

    MPI_Scatterv(data,
        scatterSize,
        scatterOffset,
        MPI_FLOAT,
        myTerrain.meshData+myTerrain.meshWidth,
        scatterSize[rank],
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

        if(i%100 == 0)
            printf("[%d][%d/%d] changes : %d \n",rank,i,terrainStep,nbChanges);
    }


    MPI_Gatherv(myTerrain.meshData+myTerrain.meshWidth,scatterSize[rank],MPI_FLOAT,data,scatterSize,scatterOffset,MPI_FLOAT,0,MPI_COMM_WORLD);

    auto stop = MPI_Wtime();
    double elapsed = stop - start;

    MPI_Gather(&elapsed,1,MPI_DOUBLE,elapsedAll,1,MPI_DOUBLE,0,MPI_COMM_WORLD);

    if (rank == 0)
    {
        double elapsedMoy;
        for(int i=0;i<size;i++){
            elapsedMoy += elapsedAll[i];
            if (i==size-1)
                elapsedMoy = elapsedMoy / size;
        }
        printf("-------------- RESULT -------------- \n");
        savePngHeightmap("MPI_heightmap_After.png",data,terrainWidth,terrainHeight);
        printf("Relative error : %f\n",testConservation(initialData,data,terrainSize));
        printf("Temps moyen d'excution de l'érosion : %lf \n",elapsedMoy);
    }

    free(scatterOffset);
    free(scatterSize);
    free(initialData);

    MPI_Finalize();

    return 0;
}
