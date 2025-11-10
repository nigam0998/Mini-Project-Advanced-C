Tic-Tac-Toe with AI (C)

This is a simple console Tic-Tac-Toe game written in C with an AI opponent using the minimax algorithm.

Files:
- `main.c` — program entry, user interaction loop.
- `game.h` / `game.c` — board representation and utility functions.
- `ai.h` / `ai.c` — minimax-based AI.
- `Makefile` — simple build rule for `gcc`.

Additionally:
- `gui_main.c` — SDL2 GUI version with clickable UI, animations and popup.

Build (using GCC/MinGW on Windows):

Open PowerShell in the project folder and run:

```powershell
gcc main.c game.c ai.c -o tictactoe -std=c99 -Wall -Wextra
.\\tictactoe.exe
```

Or use `make` if available:

```powershell
make
.\\tictactoe.exe
```

GUI build (requires SDL2 and SDL2_ttf development libraries):

```powershell
make gui
.\\gui_tictactoe.exe
```

If you don't have SDL2/SDL2_ttf installed on Windows, install via MSYS2 (recommended):

```powershell
# Install MSYS2 then in the MSYS2 MinGW64 shell:
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf
```

Then open a MinGW64 shell and run `make gui`.

Run the program and follow prompts. You can play as X or O and choose who starts.

Notes:
- The AI uses minimax and plays optimally. If both players play perfectly the game will end in a draw.
- If `gcc` is not installed, install MinGW or use Visual Studio's compiler (adjust build command accordingly).

Notes about the GUI:
- The GUI looks for a TTF font at `C:/Windows/Fonts/arial.ttf` or common Linux fonts; adjust the path in `gui_main.c` if needed.
- The GUI uses simple fade and scale animations and shows a popup when the game ends. Click "Start" to begin and click cells to place your move.
