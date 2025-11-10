#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "ai.h"

int main(void) {
    char board[9];
    init_board(board);

    printf("Tic-Tac-Toe with AI\n");
    printf("You can choose to play as X or O. X goes first.\n");

    /* Choose symbol: accept 1/2 or X/O/text */
    char human = 'X';
    while (1) {
        char line[128];
        printf("Choose your symbol:\n1) X\n2) O\nChoice (1/2 or X/O): ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("No input, exiting.\n");
            return 0;
        }
        /* find first non-space char */
        char c = '\0';
        for (int i = 0; line[i]; ++i) {
            if (line[i] != ' ' && line[i] != '\t' && line[i] != '\r' && line[i] != '\n') { c = line[i]; break; }
        }
        if (c == '1' || c == 'X' || c == 'x') { human = 'X'; break; }
        if (c == '2' || c == 'O' || c == 'o') { human = 'O'; break; }
        printf("Invalid choice, please enter 1, 2, X or O.\n");
    }
    char ai = (human == 'X') ? 'O' : 'X';
    int human_turn = (human == 'X');

    while (1) {
        print_board(board);
        char winner = check_winner(board);
        if (winner != ' ') {
            if (winner == 'T') printf("Game over: It's a draw!\n");
            else printf("Game over: %c wins!\n", winner);
            break;
        }

        if (human_turn) {
            int pos = -1;
            char line[128];
            printf("Enter position (1-9) or Q to quit: ");
            if (!fgets(line, sizeof(line), stdin)) {
                printf("No input, exiting.\n");
                break;
            }
            /* allow 'q' to quit */
            if (line[0] == 'q' || line[0] == 'Q') { printf("Quitting.\n"); break; }
            char *endptr = NULL;
            long v = strtol(line, &endptr, 10);
            if (endptr == line || v < 1 || v > 9) {
                printf("Invalid input, please enter a number 1-9.\n");
                continue;
            }
            pos = (int)(v - 1);
            if (board[pos] != ' ') {
                printf("Cell already occupied, try again.\n");
                continue;
            }
            board[pos] = human;
        } else {
            printf("AI is thinking...\n");
            int mv = get_best_move(board, ai, human);
            board[mv] = ai;
            printf("AI plays %d\n", mv + 1);
        }

        human_turn = !human_turn;
    }

    print_board(board);
    return 0;
}
