#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "ai.h"
#include "openai_ai.h"

int main(void) {
    char board[9];
    init_board(board);

    printf("=== Tic-Tac-Toe with OpenAI ===\n\n");
    
    // Initialize OpenAI
    const char *api_key = "YOUR_OPENAI_API_KEY"; // Replace with your API key or use environment variable
    
    if (openai_init(api_key) != 0) {
        fprintf(stderr, "Failed to initialize OpenAI. Using local minimax AI instead.\n");
    }

    printf("Choose AI opponent:\n");
    printf("1) Local Minimax AI (unbeatable, instant)\n");
    printf("2) OpenAI GPT AI (creative, requires internet)\n");
    
    int ai_choice = 1;
    char line[128];
    while (1) {
        printf("Choice (1/2): ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("No input, using local AI.\n");
            break;
        }
        if (line[0] == '1') { ai_choice = 1; break; }
        if (line[0] == '2') { ai_choice = 2; break; }
        printf("Invalid choice, please enter 1 or 2.\n");
    }

    char human = 'X';
    while (1) {
        printf("\nChoose your symbol:\n1) X (goes first)\n2) O (goes second)\nChoice (1/2): ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("No input, exiting.\n");
            openai_cleanup();
            return 0;
        }
        char c = line[0];
        if (c == '1' || c == 'X' || c == 'x') { human = 'X'; break; }
        if (c == '2' || c == 'O' || c == 'o') { human = 'O'; break; }
        printf("Invalid choice, please enter 1 or 2.\n");
    }
    
    char ai = (human == 'X') ? 'O' : 'X';
    int human_turn = (human == 'X');

    printf("\n=== Game Start! ===\n");

    while (1) {
        print_board(board);
        char winner = check_winner(board);
        if (winner != ' ') {
            if (winner == 'T') printf("\n🤝 Game over: It's a draw!\n");
            else if (winner == human) printf("\n🎉 Game over: You win!\n");
            else printf("\n🤖 Game over: AI wins!\n");
            break;
        }

        if (human_turn) {
            int pos = -1;
            printf("Your turn (%c). Enter position (1-9) or Q to quit: ", human);
            if (!fgets(line, sizeof(line), stdin)) {
                printf("No input, exiting.\n");
                break;
            }
            if (line[0] == 'q' || line[0] == 'Q') { 
                printf("Quitting.\n"); 
                break; 
            }
            
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
            printf("\n🤖 AI is thinking...\n");
            int mv = -1;
            
            if (ai_choice == 2) {
                // Use OpenAI
                mv = openai_get_move(board, ai, human);
                if (mv >= 0 && mv < 9 && board[mv] == ' ') {
                    board[mv] = ai;
                    printf("AI plays position %d\n", mv + 1);
                    
                    // Get explanation
                    char *explanation = openai_explain_move(board, mv, ai);
                    if (explanation) {
                        printf("💭 AI says: %s\n", explanation);
                        free(explanation);
                    }
                } else {
                    // Fallback to minimax if OpenAI fails
                    printf("OpenAI failed, using local AI...\n");
                    mv = get_best_move(board, ai, human);
                    board[mv] = ai;
                    printf("AI plays position %d\n", mv + 1);
                }
            } else {
                // Use local minimax
                mv = get_best_move(board, ai, human);
                board[mv] = ai;
                printf("AI plays position %d\n", mv + 1);
            }
        }

        human_turn = !human_turn;
    }

    print_board(board);
    printf("\nThanks for playing!\n");
    
    openai_cleanup();
    return 0;
}