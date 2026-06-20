#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <cstdint>
#include <chrono>
#include "CubeStructure.h"

// Get Index from FlipSlice Index and TwistSlice Index Lookups
unsigned int GetFlipTwistIndex(CubeState cube) {
    unsigned int FlipSlice = GetFlipSliceIndex(cube);
    unsigned int TwistSlice = GetTwistSliceIndex(cube);
    
    unsigned int FlipTwist = (FlipSlice & 0b11111111111) | ((TwistSlice >> 9) << 11);
    return FlipTwist;
}

// function to perform BFS traversal, calculate and store prune table values
void generateFlipTwistPruneTable() {
    std::cout << "Generating FlipTwist Prune Table...\n";
    const int TableSize = 2187 * 2048;

    static std::vector<uint8_t> FlipTwistTable(TableSize, 255);
    static std::queue<CubeState> CubeQ;
    static size_t evalCount = 1;

    // Start from solved cubestate
    CubeState cube(SolvedEdgeBits, SolvedCornerBits);
    FlipTwistTable[GetFlipTwistIndex(cube)] = 0;

    // Note down start time
    auto start = std::chrono::steady_clock::now();
    CubeQ.push(cube);

    // Core BFS Traversal Loop
    while (!CubeQ.empty()) {
        CubeState currentCube = CubeQ.front();
        CubeQ.pop();

        int currentIndx = GetFlipTwistIndex(currentCube);
        uint8_t currentDist = currentCube.distance;

        // Branch out using all 18 standard face turns
        for (int i = 0; i < 18; i++) {
            CubeState nextCube = MakeMove(static_cast<MoveIndex>(i), currentCube);
            int nextIndx = GetFlipTwistIndex(nextCube);
            evalCount++;

            // If index is unvisited (255), record distance and push to queue
            if (FlipTwistTable[nextIndx] == 255) {                  
                FlipTwistTable[nextIndx] = currentDist + 1;
                nextCube.distance = currentDist + 1;
                CubeQ.push(nextCube);
            }        
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "States Evaluated: " << evalCount << "\n";
    std::cout << "Time Taken: " <<  elapsed_ms << "ms\n";


    //Verify Calculations
    std::cout << "\nVerifying Calculations...\n";
    int maxDepth = FlipTwistTable[0];
    for (int i = 0; i < TableSize; i++) 
    {
        if (FlipTwistTable[i] > maxDepth) maxDepth = FlipTwistTable[i];
        if (FlipTwistTable[i] == 255) {
            std::cout << "Index " << i << " not filled\n";
            return;
        }
    }
    std::cout << "Verification complete!\n";
    std::cout << "Max Depth : " << maxDepth << "\n";

    // Format and stream out table values to a text file
    std::cout << "\nWriting table to file...\n";
    std::ofstream outputFile("FlipTwist.txt");

    if (!outputFile) {
        std::cerr << "Error: Could not open or create the file!" << std::endl;
        return; 
    }

    outputFile << "\nuint8_t FlipTwistTable[2048 * 2187] = {";
    for (int i = 0; i < TableSize; i++) {
        if (i % 32 == 0) outputFile << "\n    ";
        outputFile << static_cast<int>(FlipTwistTable[i]);
        if (i != TableSize - 1) outputFile << ", ";
    }
    outputFile << "\n};\n";
    std::cout << "Done!\n";
}