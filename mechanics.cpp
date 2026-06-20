#include "CubeStructure.h"

// Contains mask and barrel-shifting values to isolate and move a specific piece slot
struct BitShiftData {
    unsigned long Mask;
    int Shift;
};

// Represents a permutation layout consisting of 4 piece shifts that form a cycle
struct MoveType 
{
    const BitShiftData A, B, C, D;
    const unsigned long ClearMask; // Inverted compound mask used to empty targeted piece slots

    constexpr MoveType(BitShiftData a, BitShiftData b, BitShiftData c, BitShiftData d) :
    A(a), B(b), C(c), D(d),
    ClearMask(~(A.Mask | B.Mask | C.Mask | D.Mask)) {}
};

// Merges an edge cyclic permutation and a corner cyclic permutation into a single move command
struct Move 
{
    const MoveType EdgeMove;
    const MoveType CornerMove;

    constexpr Move(MoveType eM, MoveType cM) :
    EdgeMove(eM), CornerMove(cM) {}
};

// Compile-Time Helper: Calculates bitwise masks and circular shift values to transition 
// a 5-bit piece from its starting index position to its final destination position
constexpr BitShiftData CreateBitShiftEntry(int startLocation, int endLocation) {
    unsigned long mask = BitMask << (endLocation * BitsPerEntry);
    int shift = (endLocation - startLocation) * BitsPerEntry;
    if (shift < 0) shift += 64; // Wrap around if shift offset drops below zero

    return {mask, shift};
}

// Compile-Time Helper: Pairs 4 slots sequentially to generate a quarter-turn cycle map (s0->s1->s2->s3->s0)
constexpr MoveType CreateCycle(int s0, int s1, int s2, int s3) {
    return MoveType(
        CreateBitShiftEntry(s0, s1),
        CreateBitShiftEntry(s1, s2),
        CreateBitShiftEntry(s2, s3),
        CreateBitShiftEntry(s3, s0)
    );
}

// Compile-Time Helper: Pairs slots to generate double-swap cycles used by 180-degree face half-turns
constexpr MoveType CreateDoubleCycle(int s0, int s1, int t1, int t2) {
    return MoveType(
        CreateBitShiftEntry(s0, s1),
        CreateBitShiftEntry(s1, s0),
        CreateBitShiftEntry(t1, t2),
        CreateBitShiftEntry(t2, t1)
    );
}

// Emulates an architecture barrel-roll by executing low-level bitwise shifts
static unsigned long RotateLeft(unsigned long value, int count) {
    return ((value << count) | (value >> (64 - count)));
}

// Blends isolated barrel-rolled piece streams together to assemble a new permutation layout
static unsigned long ApplyBitShifts(MoveType moveType, unsigned long state) 
{
    return (state & moveType.ClearMask)
         | (RotateLeft(state, moveType.A.Shift) & moveType.A.Mask)
         | (RotateLeft(state, moveType.B.Shift) & moveType.B.Mask)
         | (RotateLeft(state, moveType.C.Shift) & moveType.C.Mask)
         | (RotateLeft(state, moveType.D.Shift) & moveType.D.Mask);
}

// Global Const Array defining bitwise operations for all 18 standard Rubik's Cube turns
constexpr Move MoveSet[18] = {
    // --- UP FACE (U) ---
    Move(CreateCycle(0,1,2,3), CreateCycle(0,1,2,3)), // U
    Move(CreateCycle(3,2,1,0), CreateCycle(3,2,1,0)), // U'
    Move(CreateDoubleCycle(0,2,1,3), CreateDoubleCycle(0,2,1,3)), // U2

    // --- DOWN FACE (D) ---
    Move(CreateCycle(11,10,9,8), CreateCycle(7,6,5,4)), // D
    Move(CreateCycle(8,9,10,11), CreateCycle(4,5,6,7)), // D'
    Move(CreateDoubleCycle(8,10,9,11), CreateDoubleCycle(4,6,5,7)), // D2

    // --- RIGHT FACE (R) ---
    Move(CreateCycle(3,6, 11, 7), CreateCycle(3,2,6,7)), // R
    Move(CreateCycle(7,11, 6,3), CreateCycle(7,6,2,3)), // R'
    Move(CreateDoubleCycle(3,11,6,7), CreateDoubleCycle(3,6,2,7)), // R2

    // --- LEFT FACE (L) ---
    Move(CreateCycle(1,4,9,5), CreateCycle(0,4,5,1)), // L
    Move(CreateCycle(5,9,4,1), CreateCycle(1,5,4,0)), // L'
    Move(CreateDoubleCycle(1,9,4,5), CreateDoubleCycle(0,5,4,1)), // L2

    // --- FRONT FACE (F) ---
    Move(CreateCycle(0,7,8,4), CreateCycle(0,3,7,4)), // F
    Move(CreateCycle(4,8,7,0), CreateCycle(4,7,3,0)), // F'
    Move(CreateDoubleCycle(0,8,7,4), CreateDoubleCycle(0,7,3,4)), // F2

    // --- BACK FACE (B) ---
    Move(CreateCycle(5,10,6,2), CreateCycle(5,6,2,1)), // B
    Move(CreateCycle(2,6,10,5), CreateCycle(1,2,6,5)), // B'
    Move(CreateDoubleCycle(5,6,10,2), CreateDoubleCycle(5,2,6,1))  // B2
};

// Core Physics Engine: Executes a move command on the 128-bit state by permuting items
// and dynamically transforming edge/corner flip and twist orientation markers
CubeState MakeMove(MoveIndex moveInd, CubeState state) 
{
    Move move = MoveSet[moveInd];

    //--- UPDATE EDGE ORIENTATION ---
    // Edge orientations change exclusively during Front (F/F') and Back (B/B') quarter turns
    bool isFrontOrBackQuarterTurn = (moveInd == F || moveInd == F_Prime | moveInd == B || moveInd == B_Prime);
    if (isFrontOrBackQuarterTurn) {
        const unsigned long EdgeOrientationMask = 0b10000'10000'10000'10000'10000'10000'10000'10000'10000'10000'10000;
        const unsigned long MovedBitsMask = ~move.EdgeMove.ClearMask;
        const unsigned long EdgeFlipMask = MovedBitsMask & EdgeOrientationMask;
        state.EdgeState ^= EdgeFlipMask; // Toggle orientation bits using a bitwise XOR operational mask
    }

    // Execute the cyclic slot position shifts
    unsigned long edgeStateNew = ApplyBitShifts(move.EdgeMove, state.EdgeState);
    unsigned long cornerStateNew = ApplyBitShifts(move.CornerMove, state.CornerState);

    //--- UPDATE CORNER ORIENTATION ---
    const unsigned long CornerOrientationMask = 0b11000'11000'11000'11000'11000'11000'11000'11000;
    unsigned long cornerOrientations = cornerStateNew & CornerOrientationMask;

    switch (moveInd) {
        // Top/Bottom turns transform orientations: (0 -> 0), (1 -> 2), (2 -> 1)
        case U: case U_Prime: case D: case D_Prime: 
            cornerOrientations = (cornerOrientations << 1) | (cornerOrientations >> 1);
            break;
        
        // Left/Right turns transform orientations: (0 -> 1), (1 -> 0), (2 -> 2)
        case L: case L_Prime: case R: case R_Prime:
            cornerOrientations = cornerOrientations ^ ((~cornerOrientations & CornerOrientationMask) >> 1);
            break;

        // Front/Back turns transform orientations: (0 -> 2), (1 -> 1), (2 -> 0)
        case F: case F_Prime: case B: case B_Prime:
            cornerOrientations = cornerOrientations ^ ((~cornerOrientations & CornerOrientationMask) << 1);
            break;
            
        // Half-turn have do not affect orientation
        default:
            break;
    }

    // Reblend modified corner orientations cleanly back into the main state register
    unsigned long cornerOrientationApplyMask = ~move.CornerMove.ClearMask & CornerOrientationMask;
    cornerStateNew = (cornerStateNew & ~cornerOrientationApplyMask) | (cornerOrientations & cornerOrientationApplyMask);

    return {edgeStateNew, cornerStateNew};
}