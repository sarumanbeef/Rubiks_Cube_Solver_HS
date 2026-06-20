# High Speed Kociemba Two-Phase Rubik's Cube Solver

An optimized, single threaded C++ implementation of Kociemba's Algorithm that runs decently fast with a memory footprint of **~140MB of RAM**.
Checkout the [Low Memory](https://github.com/sarumanbeef/Rubiks_Cube_Solver_LM) version to run on very low memory devices. 

## Highlights
* **Short Solution:** The engine first quickly comes with a short solution equal to or less than 20 moves.
* **Hunt For Optimal Solution:** If given enough time, the engine continues to search for better solution. The program exits when the shortest possible solution for the given scramble has been found.

---

## 🛠️ Build and Compilation Instructions

This project utilizes `CMake` for building native binaries. Follow the instructions below to compile and run the solver on a machine.

## Pre-requisites
Ensure you have a modern C++ compiler supporting standard feautures (C++20 or newer)

#### Debian:
```bash
sudo apt update
sudo apt install build-essential cmake
sudo apt-get install git-lfs
```

#### Arch:
```bash
sudo pacman -Syu base-devel cmake
sudo pacman -S git-lfs
```

#### Fedora:
```
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake
sudo dnf install git-lfs
```

#### Mac:
```bash
xcode-select --install
brew install cmake
brew install git-lfs
```

#### Android:
Download the [Termux](https://play.google.com/store/apps/details?id=com.termux) app from PlayStore.
```bash
pkg update
pkg install git clang
pkg install build-essential cmake
```
## Build Steps
1. Clone the repository:
   ```bash
   git clone https://github.com/sarumanbeef/Rubiks_Cube_Solver_HS/
   cd Rubiks_Cube_Solver_HS
   ```
2. Forcefully download MoveTable.h (>25MB)
   ```bash
   git lfs install
   git lfs pull
   ```
3. Generate the compiler build configuration:
   ```bash
   cmake -B build
   ```
4. Compile and link the final executable:
   ```bash
   cmake --build build
   ```

---

## 🏃 Running the Solver

Once successfully compiled, invoke the output binary file:
```bash
./build/cube_solver
```

### Example Log Output
Upon running, you will be prompted to enter a space-separated string of standard face-turns. The solver maps out the net representation, narrows down to the optimal solution:

```plaintext
Enter scramble sequence (eg: R U R' U' ... ) -> R U' R2 D' L F2 R' B2 U D' R2 L

       R B O
       Y W G
       O Y O

W B G  W R B  Y R Y  G O B
O O G  W G Y  G R R  B B W
O R B  Y B W  R O G  R O W

       R W G
       W Y Y
       B G Y

New solution found: 20 moves!
Time Taken: 13ms
Total Nodes Evaluated: 1885881
Move Sequence: B2 D' L2 R U L U2 L U' L2 U2 F2 D B2 D' B2 D R2 U L2 

New solution found: 19 moves!
Time Taken: 23ms
Total Nodes Evaluated: 3412567
Move Sequence: F2 U F2 L' R' D L F2 R D B2 R2 U' L2 U' F2 U2 R2 U 

New solution found: 18 moves!
Time Taken: 76ms
Total Nodes Evaluated: 6616115
Move Sequence: L D' U B2 L B2 R' D L2 U L D U' B2 F2 D U' L2 

New solution found: 12 moves!
Time Taken: 542ms
Total Nodes Evaluated: 34288493
Move Sequence: L' R2 D U' B2 R F2 L' D R2 U R'
```

---

## 📄 License
This project is open-source and available under the [MIT License](LICENSE).
