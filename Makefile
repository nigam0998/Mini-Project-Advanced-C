CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

# Default target: console version
all: tictactoe

# Console version (original)
tictactoe: main.c game.c ai.c
	$(CC) $(CFLAGS) -o tictactoe main.c game.c ai.c

# Console version with OpenAI
tictactoe-openai: main_openai.c game.c ai.c openai_ai.c
	$(CC) $(CFLAGS) -o tictactoe-openai main_openai.c game.c ai.c openai_ai.c -lcurl

# GUI version (requires SDL2 and SDL2_ttf)
gui: gui_main.c game.c ai.c
	$(CC) $(CFLAGS) -o gui_tictactoe gui_main.c game.c ai.c -lSDL2 -lSDL2_ttf

# GUI version with OpenAI (requires SDL2, SDL2_ttf, and libcurl)
gui-openai: gui_main.c game.c ai.c openai_ai.c
	$(CC) $(CFLAGS) -DUSE_OPENAI -o gui_tictactoe_openai gui_main.c game.c ai.c openai_ai.c -lSDL2 -lSDL2_ttf -lcurl

clean:
	rm -f tictactoe tictactoe-openai gui_tictactoe gui_tictactoe_openai *.o

.PHONY: all gui clean