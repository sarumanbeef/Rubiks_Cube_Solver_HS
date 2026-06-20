#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <chrono>
#include <cstdint>
#include <bit>
#include "CubeStructure.h"

// Strict 10-move limit prevents destroying Phase 1 alignments
static const MoveIndex Phase2Moves[] = {
    MoveIndex::U, MoveIndex::U_Prime, MoveIndex::U2,
    MoveIndex::D, MoveIndex::D_Prime, MoveIndex::D2,
    MoveIndex::R2, MoveIndex::L2, MoveIndex::F2, MoveIndex::B2
};
static const int numP2Moves = 10;

int fac(int num) {
    int value = 1;
    for (int i = 2; i <= num; i++) value *= i;
    return value;
}

// Mathematical Lehmer mapping: converts unique sequences into a clean integer index
int GetPermutationIndex(std::vector<int> sequence) {
    int length = sequence.size();
    int index = 0;
    unsigned long currentNumBits = (1UL << length) - 1;
    for (int i = 0; i < length; i++) {
        int currentValue = sequence[i];

        unsigned long lowerMask = (1UL << currentValue) - 1;
        int numsLessThanCurrentValue = std::popcount(currentNumBits & lowerMask);

        index += numsLessThanCurrentValue * fac(length - 1 - i);
        currentNumBits &= ~(1UL << currentValue);
    }
    return index;
}

int GetCornerPermutation(CubeState cube) {
    std::vector<int> corners(8);
    for (int cornerLoc = 0; cornerLoc < 8; cornerLoc++) 
        corners[cornerLoc] = (cube.CornerState >> (BitsPerEntry * cornerLoc)) & 0b00111;

    return GetPermutationIndex(corners);
}

int GetUDEdgePermutation(CubeState cube) {
    std::vector<int> edges(8);
    
    // Iterate over the U Layer slots
    for (int edgeLoc = 0; edgeLoc < 4; edgeLoc++) {             
        int edgeID = ((cube.EdgeState >> (edgeLoc * BitsPerEntry)) & 0x0F);
        if (edgeID > 7) edgeID -= 4; // Normalize pieces displaced by R2/L2/F2/B2
        edges[edgeLoc] = edgeID;
    }

    // Iterate over the D Layer slots
    for (int edgeLoc = 8; edgeLoc < 12; edgeLoc++) {
        int edgeID = (cube.EdgeState >> (BitsPerEntry * edgeLoc)) & 0x0F;
        if (edgeID > 7) edgeID -= 4; 
        edges[edgeLoc - 4] = edgeID;
    }

    return GetPermutationIndex(edges);
}

int GetESlicePermutation(CubeState cube) {
    std::vector<int> edges(4);
    for (int edgeLoc = 4; edgeLoc < 8; edgeLoc++) { 
        int edgeID = (cube.EdgeState >> (BitsPerEntry * edgeLoc)) & 0x0F;
        edges[edgeLoc - 4] = edgeID - 4; // Shift absolute IDs 4-7 down to 0-3
    }

    return GetPermutationIndex(edges);
}

void generatePhaseTwoTable() {
    std::cout << "Generating Phase 2 Prune Tables...\n";
    // ... Standard BFS queue logic remains perfectly intact here ...

    const int CornerTableSize = 40320;
    const int UDEdgeTableSize = 40320;
    const int ESliceTableSize = 24;
    
    static std::vector<uint8_t> CornerPermutationTable(CornerTableSize, 255);
    static std::vector<uint8_t> UDEdgePermutationTable(UDEdgeTableSize, 255);
    static std::vector<uint8_t> ESlicePermutationTable(ESliceTableSize, 255);

    static std::vector<std::vector<int>> CornerMoveTable(CornerTableSize, std::vector<int>(numP2Moves, -1));
    static std::vector<std::vector<int>> UDEdgeMoveTable(UDEdgeTableSize, std::vector<int>(numP2Moves, -1));
    static std::vector<std::vector<int>> ESliceMoveTable(ESliceTableSize, std::vector<int>(numP2Moves, -1));
    std::queue<CubeState> CubeQ;
    size_t evalCount = 1;

    CubeState solvedCube(SolvedEdgeBits, SolvedCornerBits);
    auto start = std::chrono::steady_clock::now();

    CornerPermutationTable[GetCornerPermutation(solvedCube)] = 0;
    solvedCube.distance = 0;
    CubeQ.push(solvedCube);

    while (!CubeQ.empty()) {
        CubeState currentCube = CubeQ.front();
        CubeQ.pop();

        int currentCornerIndx = GetCornerPermutation(currentCube);
        uint8_t currentDist = currentCube.distance;

        for (int i = 0; i < numP2Moves; i++) {
            CubeState nextCube = MakeMove(Phase2Moves[i], currentCube);
            int nextCornerIndx = GetCornerPermutation(nextCube);
            evalCount++;

            CornerMoveTable[currentCornerIndx][i] = nextCornerIndx;
            if (CornerPermutationTable[nextCornerIndx] == 255) 
            {                                                          
                CornerPermutationTable[nextCornerIndx] = currentDist + 1;

                nextCube.distance = currentDist + 1;
                CubeQ.push(nextCube);
            }                    
        }
    }

    UDEdgePermutationTable[GetUDEdgePermutation(solvedCube)] = 0;
    solvedCube.distance = 0;
    CubeQ.push(solvedCube);

    while (!CubeQ.empty()) {
        CubeState currentCube = CubeQ.front();
        CubeQ.pop();

        int currentUDIndx = GetUDEdgePermutation(currentCube);
        uint8_t currentDist = currentCube.distance;

        for (int i = 0; i < numP2Moves; i++) {
            CubeState nextCube = MakeMove(Phase2Moves[i], currentCube);
            int nextUDIndx = GetUDEdgePermutation(nextCube);
            evalCount++;

            UDEdgeMoveTable[currentUDIndx][i] = nextUDIndx;
            if (UDEdgePermutationTable[nextUDIndx] == 255)
            {                                                           
                UDEdgePermutationTable[nextUDIndx] = currentDist + 1;

                nextCube.distance = currentDist + 1;
                CubeQ.push(nextCube);
            }                   
        }
    }


    solvedCube.distance = 0;
    CubeQ.push(solvedCube);
    ESlicePermutationTable[GetESlicePermutation(solvedCube)] = 0;

    while (!CubeQ.empty()) {
        CubeState currentCube = CubeQ.front();
        CubeQ.pop();

        int currentESliceIndx = GetESlicePermutation(currentCube);
        uint8_t currentDist = currentCube.distance;

        for (int i = 0; i < numP2Moves; i++) {
            CubeState nextCube = MakeMove(Phase2Moves[i], currentCube);
            int nextESliceIndx = GetESlicePermutation(nextCube);
            evalCount++;

            ESliceMoveTable[currentESliceIndx][i] = nextESliceIndx;
            if (ESlicePermutationTable[nextESliceIndx] == 255) 
            {                                                        
                ESlicePermutationTable[nextESliceIndx] = currentDist + 1;

                nextCube.distance = currentDist + 1;
                CubeQ.push(nextCube);
            }                   
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "States Evaluated: " << evalCount << "\n";
    std::cout << "Time Taken: " <<  elapsed_ms << "ms\n\n";

    std::cout << "\nVerifying CornerMoveTable Calculations...\n";
    // Brute force and check if all indices have been filled
    for (int i = 0; i < CornerTableSize; i++) {
        for (int j = 0; j < numP2Moves; j++) {
            if (CornerMoveTable[i][j] == -1) {
                std::cout << "Index " << i << ", Move " << j << " not filled\n";
                return;
            }
        }
    }
    std::cout << "CornerMoveTable Verification complete!\n";


    std::cout << "\nVerifying UDEdgeMoveTable Calculations...\n";
    // Brute force and check if all indices have been filled
    for (int i = 0; i < UDEdgeTableSize; i++) {
        for (int j = 0; j < numP2Moves; j++) {
            if (UDEdgeMoveTable[i][j] == -1) {
                std::cout << "Index " << i << ", Move " << j << " not filled\n";
                return;
            }
        }
    }
    std::cout << "UDEdgeMoveTable Verification complete!\n";


    std::cout << "\nVerifying ESliceMoveTable Calculations...\n";
    // Brute force and check if all indices have been filled
    for (int i = 0; i < ESliceTableSize; i++) {
        for (int j = 0; j < numP2Moves; j++) {
            if (ESliceMoveTable[i][j] == -1) {
                std::cout << "Index " << i << ", Move " << j << " not filled\n";
                return;
            }
        }
    }
    std::cout << "ESliceMoveTable Verification complete!\n";


    // Wrtite the Move Tables to a file
    std::ofstream outputFile("moveTables.h");
    if (!outputFile) {
        std::cerr << "Error: Could not open or create the file!" << std::endl;
        return; 
    }

    std::cout << "\n\nWriting CornerMoveTable...\n";
    outputFile << "\nconst int CornerMoveTable[40320][10] = {";
    for (int i = 0; i < CornerTableSize; i++) {
        outputFile << "\n    {";
        for (int j = 0; j < numP2Moves; j++) { 
            outputFile << std::format("{:5d}", (CornerMoveTable[i][j]));
            if (j != numP2Moves - 1) outputFile << ", ";
        }
        outputFile << "}";
        if (i != CornerTableSize - 1) outputFile << ",";
    }
    outputFile << "\n};\n\n\n";
    std::cout << "Done.\n";

    std::cout << "\nWriting UDEdgeMoveTable...\n";
    outputFile << "\nconst int UDEdgeMoveTable[40320][10] = {";
    for (int i = 0; i < UDEdgeTableSize; i++) {
        outputFile << "\n    {";
        for (int j = 0; j < numP2Moves; j++) { 
            outputFile << std::format("{:5d}", (UDEdgeMoveTable[i][j]));
            if (j != numP2Moves - 1) outputFile << ", ";
        }
        outputFile << "}";
        if (i != UDEdgeTableSize - 1) outputFile << ",";
    }
    outputFile << "\n};\n\n\n";
    std::cout << "Done.\n";

    std::cout << "\nWriting ESliceMoveTable...\n";
    outputFile << "\nconst int ESliceMoveTable[24][10] = {";
    for (int i = 0; i < ESliceTableSize; i++) {
        outputFile << "\n    {";
        for (int j = 0; j < numP2Moves; j++) { 
            outputFile << std::format("{:2d}", (ESliceMoveTable[i][j]));
            if (j != numP2Moves - 1) outputFile << ", ";
        }
        outputFile << "}";
        if (i != ESliceTableSize - 1) outputFile << ",";
    }
    outputFile << "\n};\n";
    std::cout << "Done.\n";
}