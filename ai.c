#include "ai.h"
#include "game.h"
#include <limits.h>
#include <stdbool.h>

static int evaluate_board(const char board[9], char ai, char human) {
    char w = check_winner(board);
    if (w == ai) return 10;
    if (w == human) return -10;
    return 0;
}

static int minimax(char board[9], char ai, char human, int depth, bool isMax) {
    int score = evaluate_board(board, ai, human);
    if (score == 10) return score - depth; /* prefer faster wins */
    if (score == -10) return score + depth; /* prefer slower losses */

    /* tie */
    bool movesLeft = false;
    for (int i = 0; i < 9; ++i) if (board[i] == ' ') { movesLeft = true; break; }
    if (!movesLeft) return 0;

    if (isMax) {
        int best = INT_MIN;
        for (int i = 0; i < 9; ++i) {
            if (board[i] == ' ') {
                board[i] = ai;
                int val = minimax(board, ai, human, depth+1, false);
                board[i] = ' ';
                if (val > best) best = val;
            }
        }
        return best;
    } else {
        int best = INT_MAX;
        for (int i = 0; i < 9; ++i) {
            if (board[i] == ' ') {
                board[i] = human;
                int val = minimax(board, ai, human, depth+1, true);
                board[i] = ' ';
                if (val < best) best = val;
            }
        }
        return best;
    }
}

int get_best_move(const char board[9], char ai, char human) {
    int bestVal = INT_MIN;
    int bestMove = -1;
    char copy[9];
    for (int i = 0; i < 9; ++i) copy[i] = board[i];

    for (int i = 0; i < 9; ++i) {
        if (copy[i] == ' ') {
            copy[i] = ai;
            int moveVal = minimax(copy, ai, human, 0, false);
            copy[i] = ' ';
            if (moveVal > bestVal) {
                bestVal = moveVal;
                bestMove = i;
            }
        }
    }
    if (bestMove == -1) {
        /* No moves left; shouldn't be called in that state, but default to 0 */
        return 0;
    }
    return bestMove;
}
