#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <cstdint>
#include <chrono>
#include "CubeStructure.h"

// Math Helper: Calculates the Binomial Coefficient (n Choose k)
static int nCk(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;

    if (k > n - k) 
        k = n - k;

    int result = 1;
    for (int i = 1; i <= k; i++) {
        result *= (n - k + i);
        result /= i;
    }

    return result;
}

// Coordinate Encoder: Packs 11 independent Edge Orientations and the 
// Co-lexicographical index of the 4 E-slice edges into a unique 20-bit index
unsigned int GetFlipSliceIndex(CubeState cube) {
    unsigned int FlipIndex  = 0;
    unsigned int SliceIndex = 0;

    // Isolate orientation flags for the first 11 edges (12th is implicitly defined)
    for (int i = 0; i < 11; i++) {
        int EdgeOrientationBitPos = (BitsPerEntry * i) + (BitsPerEntry - 1);
        bool EdgeOrientation = (cube.EdgeState >> EdgeOrientationBitPos) & 0b1;
        FlipIndex |= EdgeOrientation << i;
    }

    // Locate the physical slot locations currently holding the 4 E-Slice pieces (IDs 4 to 7)
    std::vector<int> SliceLoc(4);
    int LocInd = 0;
    for (int EdgeLoc= 0; EdgeLoc < 12; EdgeLoc++) {
        int EdgeInd = cube.EdgeState >> (BitsPerEntry * EdgeLoc) & 0b1111;

        if (EdgeInd >= 4 && EdgeInd <= 7)
            SliceLoc[LocInd++] = (EdgeLoc);
    }

    // Map the 4 slot indices to a unique combination index from 0 to 494 using Lehmer code properties
    int LeftOverCombinations = 0;
    for (LocInd = 0; LocInd < 4; LocInd++) 
        LeftOverCombinations += nCk(11 - SliceLoc[LocInd], 4 - LocInd);

    const int TotalCombination = 495;            // 12 C 4
    SliceIndex = TotalCombination - 1 - LeftOverCombinations;

    // Shift combination index by 11 bits to avoid collision with the edge flip mask
    return (SliceIndex << 11) | FlipIndex;
} 

// Runs a full Breadth-First Search (BFS) graph traversal to map out the entire Phase 1 
// Flip-Slice space, saving distances to a file to build the header file array
void generateFlipSlicePruneTable() { 
    std::cout << "Generating FlipSlice prune table...\n";
    const int TableSize = 2048 * 495; // 1,013,760 total states

    static std::vector<uint8_t> FlipSliceTable(TableSize, 255);
    static std::vector<std::vector<int>> FlipMoveTable(TableSize, std::vector<int>(18, -1));
    static std::queue<CubeState> CubeQ;
    size_t evalCount = 1;

    // Anchor the search at the solved cube state (Distance 0)
    CubeState cube(SolvedEdgeBits, SolvedCornerBits);
    FlipSliceTable[GetFlipSliceIndex(cube)] = 0;         

    auto start = std::chrono::steady_clock::now();
    CubeQ.push(cube);

    // Core BFS Traversal Loop
    while (!CubeQ.empty()) {
        CubeState currentCube = CubeQ.front();
        CubeQ.pop();

        int currentIndx = GetFlipSliceIndex(currentCube);
        uint8_t currentDist = currentCube.distance;

        // Branch out using all 18 standard face turns
        for (int i = 0; i < 18; i++) {
            CubeState nextCube = MakeMove(static_cast<MoveIndex>(i), currentCube);
            int nextIndx = GetFlipSliceIndex(nextCube);
            evalCount++;

            FlipMoveTable[currentIndx][i] = nextIndx;
            // If index is unvisited (255), record distance and push to queue
            if (FlipSliceTable[nextIndx] == 255) {                  
                FlipSliceTable[nextIndx] = currentDist + 1;
                nextCube.distance = currentDist + 1;
                CubeQ.push(nextCube);
            }        
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "States Evaluated: " << evalCount << "\n";
    std::cout << "Time Taken: " <<  elapsed_ms << "ms\n";
    
    std::cout << "\nVerifying Calculations...\n";
    int maxDepth = FlipSliceTable[0];
    for (int i = 0; i < TableSize; i++) {

        for (int j = 0; j < 18; j++) {
            if (FlipMoveTable[i][j] == -1) {
                std::cout << "Index " << i << " Move " << j << " not filled.\n";
                return;
            }
        }

        if (FlipSliceTable[i] > maxDepth) maxDepth = FlipSliceTable[i];
        if (FlipSliceTable[i] == 255)
            std::cout << "Index " << i <<" not filled in IndxTable\n";
    }

    std::cout << "Verification complete!\n";
    std::cout << "Max Depth : " << maxDepth << "\n";

    // Format and stream out table values to a text file
    std::cout << "\nWriting table to file...\n";
    std::ofstream outputFile("FlipMove.txt");

    if (!outputFile) {
        std::cerr << "Error: Could not open or create the file!" << std::endl;
        return; 
    }

    outputFile << "\nint FlipMoveTable[1013760][18] = {";
    for (int i = 0; i < TableSize; i++) {
        outputFile << "\n    {";
        for (int j = 0; j < 18; j++) { 
            outputFile << std::format("{:7d}", (FlipMoveTable[i][j]));
            if (j != 18 - 1) outputFile << ", ";
        }
        outputFile << "}";
        if (i != TableSize - 1) outputFile << ",";
    }
    outputFile << "\n};\n\n\n";
    std::cout << "Done.\n";
}