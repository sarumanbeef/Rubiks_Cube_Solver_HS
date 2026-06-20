#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

// Represents the complete physical state of the Rubik's Cube
// using two 64-bit integers for edges and corners respectively.
struct CubeState
{
	unsigned long EdgeState;
	unsigned long CornerState;
    uint8_t distance = 0; // Tracks depth during table generation and search

	CubeState(unsigned long edgeState, unsigned long cornerState) :
	EdgeState(edgeState), CornerState(cornerState) {}
};

// 5 bits are used to pack each piece's ID and orientation
const int BitsPerEntry = 5;
const unsigned long BitMask = (1 << BitsPerEntry) - 1;      // 0b11111

// The pristine, solved bitboard states for edges and corners
const unsigned long SolvedEdgeBits = 0b01011'01010'01001'01000'00111'00110'00101'00100'00011'00010'00001'00000;
const unsigned long SolvedCornerBits = 0b00111'00110'00101'00100'00011'00010'00001'00000;

// All 18 possible face turns on a Rubik's cube
enum MoveIndex {
    U, U_Prime, U2,
    D, D_Prime, D2,
    R, R_Prime, R2,
    L, L_Prime, L2,
    F, F_Prime, F2,
    B, B_Prime, B2
};

// String mappings for displaying and parsing move sequences
const std::string MoveInString[] = {
    "U", "U'", "U2",
    "D", "D'", "D2",
    "R", "R'", "R2",
    "L", "L'", "L2",
    "F", "F'", "F2",
    "B", "B'", "B2",
};

const std::unordered_map<std::string, MoveIndex> StringToMove = {
    {"U", U}, {"U'", U_Prime}, {"U2", U2},
    {"D", D}, {"D'", D_Prime}, {"D2", D2},
    {"R", R}, {"R'", R_Prime}, {"R2", R2},
    {"L", L}, {"L'", L_Prime}, {"L2", L2},
    {"F", F}, {"F'", F_Prime}, {"F2", F2},
    {"B", B}, {"B'", B_Prime}, {"B2", B2},
};


CubeState MakeMove(MoveIndex moveInd, CubeState state);

void SolveCube(CubeState startState);

unsigned int GetFlipSliceIndex(CubeState cube);
unsigned int GetTwistSliceIndex(CubeState cube);
unsigned int GetFlipTwistIndex(CubeState cube);
int GetCornerPermutation(CubeState cube);
int GetUDEdgePermutation(CubeState cube);
int GetESlicePermutation(CubeState cube);

void generateFlipSlicePruneTable();
void generateTwistSlicePruneTable();
void generateFlipTwistPruneTable();
void generatePhaseTwoTable();

void displayCube(CubeState cubeState);
CubeState ApplySequence(CubeState cube, std::string sequence);