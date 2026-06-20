#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include "CubeStructure.h"
using namespace std;

// Color pairs for the 12 edges (Orientation 0 vs Orientation 1)
const string EDGE_COLORS[] = {
	"WG", "WO", "WB", "WR",
	"GO", "BO", "BR", "GR",
	"YG", "YO", "YB", "YR"
};

// Color triplets for the 8 corners (Standard clockwise order of faces)
const string CORNER_COLORS[] = {
	"OWG", "BWO", "RWB", "GWR",
	"GYO", "OYB", "BYR", "RYG"
};

// Look-up matrix to correct corner sticker orientation depending on whether 
// the corner slot's winding order is mirrored along the cube's axes
constexpr int CORNER_LOOKUP_MATRIX[2][3] = {
     {1, 0, 2},
     {1, 2, 0}
};

// Extracts an edge piece from its 5-bit slot, computes its current orientation,
// and returns the target sticker color facing the specified reference plane
char getEdgeColor (int edgeLoc, unsigned long edgeState, bool colorInd) 
{
    int edgeData = edgeState >> (edgeLoc * BitsPerEntry) & BitMask;
	int edgeInd = edgeData & 0b01111; // Extract 4-bit Piece ID
	bool isFlipped = edgeData >> 4 & 0b1; // Extract 1-bit Orientation flag
	int finalInd = (colorInd + isFlipped) % 2; // Offset color index if flipped

	return EDGE_COLORS[edgeInd][finalInd];
}

// Extracts a corner piece from its 5-bit slot, calculates its absolute rotation,
// and maps it to the correct sticker color using the symmetry matrix
char getCornerColor (int cornerLoc, unsigned long cornerState, int colorInd) 
{
    int cornerData = cornerState >> (cornerLoc * BitsPerEntry) & BitMask;
	int cornerInd = cornerData & 0b00111; // Extract 3-bit Piece ID
	int orientation = cornerData >> 3 & 0b11; // Extract 2-bit Twist value (0, 1, or 2)

    bool isMirrored = (cornerLoc == 0) | (cornerLoc == 2) | (cornerLoc == 5) | (cornerLoc == 7);
	int finalInd = CORNER_LOOKUP_MATRIX[isMirrored][((colorInd - orientation + 3) % 3)];

	return CORNER_COLORS[cornerInd][finalInd];
}

// Prints an unfolded 2D net representation of the cube to the console
void displayCube(CubeState state) 
{
	auto E = [&](int edgeLoc, bool colorInd) { return getEdgeColor(edgeLoc, state.EdgeState, colorInd); };
	auto C = [&](int cornerLoc, int colorInd) { return getCornerColor(cornerLoc, state.CornerState, colorInd); };

	// --- UP FACE (White Center) ---
    cout << "       " << C(1, 0) << " " << E(2, 0) << " " << C(2, 0) << "\n";
    cout << "       " << E(1, 0) << " W " << E(3, 0) << "\n";
    cout << "       " << C(0, 0) << " " << E(0, 0) << " " << C(3, 0) << "\n\n";

    // --- MIDDLE BAND (Left, Front, Right, Back) ---
    // Top Row of Middle Band
    cout << C(1, 2) << " " << E(1, 1) << " " << C(0, 2) << "  "  // Left
         << C(0, 1) << " " << E(0, 1) << " " << C(3, 1) << "  "  // Front
         << C(3, 2) << " " << E(3, 1) << " " << C(2, 2) << "  "  // Right
         << C(2, 1) << " " << E(2, 1) << " " << C(1, 1) << "\n"; // Back

    // Center Row of Middle Band
    cout << E(5, 1) << " O " << E(4, 1) << "  "                  // Left
         << E(4, 0) << " G " << E(7, 0) << "  "                  // Front
         << E(7, 1) << " R " << E(6, 1) << "  "                  // Right
         << E(6, 0) << " B " << E(5, 0) << "\n";                 // Back

    // Bottom Row of Middle Band
    cout << C(5, 2) << " " << E(9, 1) << " " << C(4, 2) << "  "  // Left
         << C(4, 1) << " " << E(8, 1) << " " << C(7, 1) << "  "  // Front
         << C(7, 2) << " " << E(11, 1) << " " << C(6, 2) << "  " // Right
         << C(6, 1) << " " << E(10, 1) << " " << C(5, 1) << "\n\n"; // Back

    // --- DOWN FACE (Yellow Center) ---
    cout << "       " << C(4, 0) << " " << E(8, 0) << " " << C(7, 0) << "\n";
    cout << "       " << E(9, 0) << " Y " << E(11, 0) << "\n";
    cout << "       " << C(5, 0) << " " << E(10, 0) << " " << C(6, 0) << "\n\n";
}

// Tokenizes a space-separated string of moves and applies them to the bitboard sequence
CubeState ApplySequence(CubeState cube, string sequence) {
     istringstream iss(sequence);
     string token;

     CubeState newCube = cube;
     while (iss >> token) {
          MoveIndex move = StringToMove.at(token);
          newCube = MakeMove(move, newCube);
     }
     return newCube;
}