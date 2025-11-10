#ifndef OPENAI_AI_H
#define OPENAI_AI_H

/* Initialize OpenAI client with API key */
int openai_init(const char *api_key);

/* Get best move using OpenAI API */
int openai_get_move(const char board[9], char player, char opponent);

/* Get move explanation from OpenAI */
char* openai_explain_move(const char board[9], int move, char player);

/* Cleanup OpenAI resources */
void openai_cleanup(void);

#endif /* OPENAI_AI_H */