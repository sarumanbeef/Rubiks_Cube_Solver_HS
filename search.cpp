#include <iostream>
#include <algorithm>
#include <chrono>
#include <array>
#include "CubeStructure.h"
#include "PruneTables.h"
#include "MoveTables.h"

static const uint8_t numMoves = 18;
static size_t evalCount = 0;
std::chrono::steady_clock::time_point start;
static CubeState ScrambledStartState(SolvedEdgeBits, SolvedCornerBits);

// Flat, zero-overhead fixed global path tracker to replace active vectors
static std::array<MoveIndex, 32> currentPath;

// Only these 10 moves are allowed in Phase 2 to protect orientations and the E-slice!
static const MoveIndex Phase2Moves[] = {
    MoveIndex::U, MoveIndex::U_Prime, MoveIndex::U2,
    MoveIndex::D, MoveIndex::D_Prime, MoveIndex::D2,
    MoveIndex::R2, MoveIndex::L2, MoveIndex::F2, MoveIndex::B2
};
static const uint8_t numP2Moves = 10;

// Global ceiling 
uint8_t bestTotalMoves = 21;
std::vector<MoveIndex> bestPath;

// --- BRANCHLESS LOOKAHEAD TRANSITION MATRICES ---
// Faces: 0=U, 1=D, 2=R, 3=L, 4=F, 5=B, 6=None (Start state)
static const uint8_t P1_NextMoveCount[7] = { 12, 15, 12, 15, 12, 15, 18 };
static const uint8_t P1_NextMoves[7][18] = {
    { 6,7,8, 9,10,11, 12,13,14, 15,16,17 },             // After U: No U, no D
    { 0,1,2, 6,7,8, 9,10,11, 12,13,14, 15,16,17 },       // After D: No D (Allows U)
    { 0,1,2, 3,4,5, 12,13,14, 15,16,17 },                // After R: No R, no L
    { 0,1,2, 3,4,5, 6,7,8, 12,13,14, 15,16,17 },         // After L: No L (Allows R)
    { 0,1,2, 3,4,5, 6,7,8, 9,10,11 },                    // After F: No F, no B
    { 0,1,2, 3,4,5, 6,7,8, 9,10,11, 12,13,14 },          // After B: No B (Allows F)
    { 0,1,2, 3,4,5, 6,7,8, 9,10,11, 12,13,14, 15,16,17 } // Start: All moves legal
};

static const uint8_t P2_NextMoveCount[7] = { 4, 7, 8, 9, 8, 9, 10 };
static const uint8_t P2_NextMoveTableIndx[7][10] = {
    { 6, 7, 8, 9 },                                      // After U: R2, L2, F2, B2
    { 0, 1, 2, 6, 7, 8, 9 },                             // After D: U variations + Face 2-5
    { 0, 1, 2, 3, 4, 5, 8, 9 },                          // After R: U/D variations + F2, B2
    { 0, 1, 2, 3, 4, 5, 6, 8, 9 },                       // After L: U/D variations + R2, F2, B2
    { 0, 1, 2, 3, 4, 5, 6, 7 },                          // After F: U/D variations + R2, L2
    { 0, 1, 2, 3, 4, 5, 6, 7, 8 },                       // After B: U/D variations + R2, L2, F2
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }                     // Start: All Phase 2 choices open
};


// IDA* Search for Phase 2 Subgroup
static bool idaStarPhase2(uint16_t cornerIndx, uint16_t udIndx, uint8_t eIndx, uint8_t currentDistance, uint8_t maxDistance, int8_t lastFace, uint8_t p1Depth) 
{
    evalCount++;

    // Calculate lower bound utilizing Phase 2 specific tables
    uint8_t CornerDistance     = CornerPermutationTable[cornerIndx];
    uint8_t UDEdgeDistance     = UDEdgePermutationTable[udIndx];
    uint8_t ESliceDistance     = ESlicePermutationTable[eIndx];

    uint8_t macroMax          = CornerDistance > UDEdgeDistance ? CornerDistance : UDEdgeDistance;
    uint8_t estimatedMinDistance = macroMax > ESliceDistance ? macroMax : ESliceDistance;

    // Prune branch if the heuristic proves we cannot reach the goal within the bound
    if (maxDistance < currentDistance + estimatedMinDistance) 
        return false;

    // Phase 2 fully solved
    if (estimatedMinDistance == 0)
        return true;

    // Generate constrained Phase 2 moves
    uint8_t moveCount = P2_NextMoveCount[lastFace];
    // #pragma GCC unroll 10
    for (uint8_t i = 0; i < moveCount; i++) {
        uint8_t tableIndx = P2_NextMoveTableIndx[lastFace][i];
        MoveIndex move = Phase2Moves[tableIndx];
        int8_t nextFace = static_cast<int>(move) / 3;
                
        // Single clock-cycle static array overwrite (No vectors, no capacity branching)
        currentPath[p1Depth + currentDistance] = move;
        
        uint16_t nextCorner = CornerMoveTable[cornerIndx][tableIndx];
        uint16_t nextUD     = UDEdgeMoveTable[udIndx][tableIndx];
        uint8_t nextE      = ESliceMoveTable[eIndx][tableIndx];

        if (idaStarPhase2(nextCorner, nextUD, nextE, currentDistance + 1, maxDistance, nextFace, p1Depth)) 
            return true; // Solution found deeper down
    }

    return false;
}

// Master interface for bridging Phase 1 to Phase 2
void SolvePhaseTwo(CubeState startState, int maxPhase2Depth, uint8_t p1Depth) {
    uint16_t cornerIndx = GetCornerPermutation(startState);
    uint16_t udIndx = GetUDEdgePermutation(startState);
    uint8_t eIndx = GetESlicePermutation(startState);
    uint8_t startDistance = std::max({CornerPermutationTable[cornerIndx],
                                      UDEdgePermutationTable[udIndx],
                                      ESlicePermutationTable[eIndx]});
    
    // Only engage search if a theoretical improvement is mathematically possible
    if (startDistance <= maxPhase2Depth) {
        int8_t lastFace = -1;                                            
        if (p1Depth > 0) {
            lastFace = static_cast<int>(currentPath[p1Depth - 1]) / 3;
        }

        // Map native -1 sentinel to matrix index 6
        int8_t lookupFace = (lastFace == -1) ? 6 : lastFace;

        for(uint8_t p2Bound = startDistance; p2Bound <= maxPhase2Depth; p2Bound++) {
            if (idaStarPhase2(cornerIndx, udIndx, eIndx, 0, p2Bound, lookupFace, p1Depth)) {
                auto end = std::chrono::steady_clock::now();
                auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                
                // Update the global ceiling with the new best run
                bestTotalMoves = p1Depth + p2Bound;
                bestPath.clear();
                
                // Copy array to bestPath when shorter path found
                bestPath.insert(bestPath.end(), currentPath.begin(), currentPath.begin() + bestTotalMoves);

                std::cout << "New solution found: " << static_cast<int>(bestTotalMoves) << " moves!\n";
                std::cout << "Time Taken: " << elapsed_ms << "ms\n";
                std::cout << "Total Nodes Evaluated: " << evalCount << "\n";
                std::cout << "Move Sequence: ";

                for (MoveIndex m : bestPath) {
                    std::cout << MoveInString[static_cast<int>(m)] << " ";
                }
                std::cout << std::endl << std::endl;

                return;
            }
        }
    }
}

// IDA* Search for Phase 1 (Orientation & E-Slice Isolation)
void idaStarPhase1(int flipSliceIndx, int twistSliceIndx, uint8_t currentDistance, uint8_t maxDistance, int8_t lastFace) 
{
    evalCount++;

    // Safety Optimization: If Phase 1 has already matched or exceeded our global ceiling, abort immediately
    if (currentDistance >= bestTotalMoves)
        return;

    unsigned int flipTwistIndx = (flipSliceIndx & 0b11111111111) | ((twistSliceIndx >> 9) << 11);
    uint8_t flipSliceDistance     = FlipSliceTable[flipSliceIndx];
    uint8_t twistSliceDistance    = TwistSliceTable[twistSliceIndx];
    uint8_t flipTwistDistance     = FlipTwistTable[flipTwistIndx];

    uint8_t macroMax              = flipSliceDistance > twistSliceDistance ? flipSliceDistance : twistSliceDistance;
    uint8_t estimatedMinDistance  = macroMax > flipTwistDistance ? macroMax : flipTwistDistance;

    if (maxDistance < currentDistance + estimatedMinDistance) 
        return;

    // Phase 1 Subgroup successfully reached!
    if (estimatedMinDistance == 0) {
        // Yield to Phase 2: Squeeze the depth limit to strictly beat the current record
        if (currentDistance >= bestTotalMoves) return;
        uint8_t maxPhase2Depth = bestTotalMoves - currentDistance - 1;

        // Calculate CubeState to handoff
        CubeState p2State = ScrambledStartState;
        for (uint8_t i = 0; i < currentDistance; i++)
            p2State = MakeMove(currentPath[i], p2State);

        SolvePhaseTwo(p2State, maxPhase2Depth, currentDistance);
        return; // Always return to force Phase 1 to keep exploring alternatives
    }

    // Generate all 18 moves
    uint8_t moveCount = P1_NextMoveCount[lastFace]; 
    for (uint8_t i = 0; i < moveCount; i++) {
        uint8_t rawMoveIndx = P1_NextMoves[lastFace][i];
        MoveIndex move = static_cast<MoveIndex>(rawMoveIndx);
        int nextFace = rawMoveIndx / 3;

        currentPath[currentDistance] = move;

        // Instant state transitions
        int nextFlipSlice = FlipMoveTable[flipSliceIndx][rawMoveIndx];
        int nextTwistSlice = TwistMoveTable[twistSliceIndx][rawMoveIndx];

        idaStarPhase1(nextFlipSlice, nextTwistSlice, currentDistance + 1, maxDistance, nextFace);
    }
}

// Master execution loop
void SolveCube(CubeState startState) {
    start = std::chrono::steady_clock::now();
    evalCount = 0;
    bestTotalMoves = 21; 
    ScrambledStartState = startState; // Assign to global variable so that Phase 1 can use it at handoff

    int startFlipSlice = GetFlipSliceIndex(startState);
    int startTwistSlice = GetTwistSliceIndex(startState);
    int startFlipTwist = GetFlipTwistIndex(startState);
    int p1StartDistance = std::max({FlipSliceTable[startFlipSlice], 
                                    TwistSliceTable[startTwistSlice],
                                    FlipTwistTable[startFlipTwist]});

    // Loop automatically terminates when p1Bound eclipses the dynamically shrinking bestTotalMoves
    for(int p1Bound = p1StartDistance; p1Bound <= bestTotalMoves; p1Bound++) {
        idaStarPhase1(startFlipSlice, startTwistSlice, 0, p1Bound, 6);  // 6 represents no last face
    }
}
