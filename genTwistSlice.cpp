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

// Coordinate Encoder: Packs 7 independent Corner Orientations (base 3) and 
// the Co-lexicographical combination index of the 4 E-slice edges (0 to 494)
unsigned int GetTwistSliceIndex(CubeState cube) {
    unsigned int TwistIndex = 0;
    unsigned int SliceIndex = 0;

    // Isolate orientations for the first 7 corners (8th orientation is implied by law of total twist)
    for (int i = 0; i < 7; i++) {
        int CornerOrientationBitPos = (BitsPerEntry * i) + (BitsPerEntry - 2);
        int CornerOrientation = (cube.CornerState >> CornerOrientationBitPos) & 0b11;   
        TwistIndex *= 3; // Shift index into base-3 representation space
        TwistIndex += CornerOrientation;
    }

    // Locate physical slot layouts currently holding the 4 E-Slice pieces (IDs 4 to 7)
    std::vector<int> SliceLoc(4);
    int LocInd = 0;
    for (int EdgeLoc= 0; EdgeLoc < 12; EdgeLoc++) {
        int EdgeInd = cube.EdgeState >> (BitsPerEntry * EdgeLoc) & 0b1111;

        if (EdgeInd >= 4 && EdgeInd <= 7)
            SliceLoc[LocInd++] = (EdgeLoc);
    }

    // Map slot layout configuration to combination coordinate indices
    int LeftOverCombinations = 0;
    for (LocInd = 0; LocInd < 4; LocInd++) 
        LeftOverCombinations += nCk(11 - SliceLoc[LocInd], 4 - LocInd);

    const int TotalCombination = 495;            // 12 C 4
    SliceIndex = TotalCombination - 1 - LeftOverCombinations;

    // Use arithmetic tight packing: TwistIndex * 495 + SliceIndex
    return (TwistIndex << 9) + SliceIndex;
} 

// Runs a full BFS graph traversal to map out the companion Phase 1 
// Twist-Slice space, saving distances to a file for table compilation
void generateTwistSlicePruneTable() {
    std::cout << "Generating TwistSlice prune table...\n";
    const long TableSize = 2187 * 512; 

    static std::vector<uint8_t> TwistSliceTable(TableSize, 255);
    static std::vector<std::vector<int>> TwistMoveTable(TableSize, std::vector<int>(18, -1));
    static std::queue<CubeState> CubeQ;
    size_t evalCount = 1;

    // Anchor search at the solved cube state (Distance 0)
    CubeState cube(SolvedEdgeBits, SolvedCornerBits);
    TwistSliceTable[GetTwistSliceIndex(cube)] = 0;         

    auto start = std::chrono::steady_clock::now();
    CubeQ.push(cube);

    // Core BFS Traversal Loop
    while (!CubeQ.empty()) {
        CubeState currentCube = CubeQ.front();
        CubeQ.pop();

        int currentIndx = GetTwistSliceIndex(currentCube);
        uint8_t currentDist = currentCube.distance;

        // Branch out across all 18 basic face moves
        for (int i = 0; i < 18; i++) {
            CubeState nextCube = MakeMove(static_cast<MoveIndex>(i), currentCube);
            int nextIndx = GetTwistSliceIndex(nextCube);
            evalCount++;

            TwistMoveTable[currentIndx][i] = nextIndx;
            if (TwistSliceTable[nextIndx] == 255) {                  
                TwistSliceTable[nextIndx] = currentDist + 1;
                nextCube.distance = currentDist + 1;
                CubeQ.push(nextCube);
            }        
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "States Evaluated: " << evalCount << "\n";
    std::cout << "Time Taken: " <<  elapsed_ms << "ms\n";
    
    /*std::cout << "\nVerifying Calculations...\n";
    int maxDepth = TwistSliceTable[0];
    for (int i = 0; i < TableSize; i++) {

        for (int j = 0; j < 18; j++) {
            if (TwistMoveTable[i][j] == -1) {
                std::cout << "Index " << i << " Move " << j << " Not filled.\n";
                return;
            }
        }

        if (TwistSliceTable[i] > maxDepth) maxDepth = TwistSliceTable[i];
        if (TwistSliceTable[i] == 255) {
            std::cout << "Index " << i << " not filled in IndxTable\n";
            return;
        }
    }
    std::cout << "Verification complete!\n";
    std::cout << "Max Depth : " << maxDepth << "\n";*/


    // Format and dump lookup structures to text file
    std::cout << "\nWriting table to file...\n";
    std::ofstream outputFile("TwistSlice.txt");

    if (!outputFile) {
        std::cerr << "Error: Could not open or create the file!" << std::endl;
        return; 
    }

    outputFile << "\nconst std::vector<uint8_t> TwistSliceTable = {";
    for (int i = 0; i < TableSize; i++) {
        if (i % 33 == 0) outputFile << "\n    ";
        outputFile << std::format("{:3d}", static_cast<int>(TwistSliceTable[i]));
        if (i != TableSize - 1) outputFile << ", ";
    }
    outputFile << "\n};\n";
    std::cout << "Done!\n";


    // Format and dump lookup structures to text file
    /*std::cout << "\nWriting table to file...\n";
    std::ofstream outputFile("TwistMove.txt");

    if (!outputFile) {
        std::cerr << "Error: Could not open or create the file!" << std::endl;
        return; 
    }

    outputFile << "\nint TwistMoveTable[2187 * 512][18] = {";
    for (int i = 0; i < TableSize; i++) {
        outputFile << "\n    {";
        for (int j = 0; j < 18; j++) { 
            outputFile << std::format("{:7d}", (TwistMoveTable[i][j]));
            if (j != 18 - 1) outputFile << ", ";
        }
        outputFile << "}";
        if (i != TableSize - 1) outputFile << ",";
    }
    outputFile << "\n};\n\n\n";
    std::cout << "Done.\n";*/
}