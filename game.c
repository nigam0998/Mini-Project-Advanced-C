#include "game.h"
#include <stdio.h>

static int win_lines[8][3] = {
    {0,1,2},{3,4,5},{6,7,8},
    {0,3,6},{1,4,7},{2,5,8},
    {0,4,8},{2,4,6}
};

void init_board(char board[9]) {
    for (int i = 0; i < 9; ++i) board[i] = ' ';
}

void print_board(const char b[9]) {
    printf("\n");
    for (int i = 0; i < 9; ++i) {
        char c = (b[i] == ' ') ? ' ' : b[i];
        printf(" %c ", c);
        if (i % 3 != 2) printf("|");
        if (i % 3 == 2 && i < 8) printf("\n---+---+---\n");
    }
    printf("\n\n");
}

char check_winner(const char b[9]) {
    for (int i = 0; i < 8; ++i) {
        int a = win_lines[i][0], c = win_lines[i][1], d = win_lines[i][2];
        if (b[a] != ' ' && b[a] == b[c] && b[c] == b[d]) return b[a];
    }
    for (int i = 0; i < 9; ++i) if (b[i] == ' ') return ' ';
    return 'T';
}
