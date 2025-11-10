#ifndef GAME_H
#define GAME_H

/* Initialize board to empty spaces */
void init_board(char board[9]);

/* Print board to stdout */
void print_board(const char board[9]);

/* Check for winner: returns 'X' or 'O' when someone wins, 'T' for tie, ' ' for game ongoing */
char check_winner(const char board[9]);

#endif /* GAME_H */
