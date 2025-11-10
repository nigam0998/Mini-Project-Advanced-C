#ifndef AI_H
#define AI_H

/* Returns index 0-8 for best move for `ai` given current board. `human` is the opponent symbol. */
int get_best_move(const char board[9], char ai, char human);

#endif /* AI_H */
