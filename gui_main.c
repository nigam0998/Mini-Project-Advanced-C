#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "game.h"
#include "ai.h"

/* Modern Tic-Tac-Toe with enhanced UI/UX
   - Dark modern theme with gradient accents
   - Smooth animations and transitions
   - Glow effects and hover states
   - Score tracking
   - Better visual feedback
   - AI vs 2 Player modes
*/

static const int WINDOW_W = 800;
static const int WINDOW_H = 900;

typedef enum { SCENE_WELCOME, SCENE_MODE_SELECT, SCENE_AI_SELECT, SCENE_GAME, SCENE_POPUP } Scene;
typedef enum { MODE_AI, MODE_TWO_PLAYER } GameMode;

typedef struct {
    int x_wins;
    int o_wins;
    int draws;
} Score;

// Modern color palette
static const SDL_Color BG_DARK = {15, 23, 42, 255};           // Dark blue-gray
static const SDL_Color BG_CARD = {30, 41, 59, 255};           // Card background
static const SDL_Color ACCENT_PRIMARY = {99, 102, 241, 255};   // Indigo
static const SDL_Color ACCENT_SECONDARY = {168, 85, 247, 255}; // Purple
static const SDL_Color TEXT_PRIMARY = {248, 250, 252, 255};    // Almost white
static const SDL_Color TEXT_SECONDARY = {148, 163, 184, 255};  // Gray
static const SDL_Color GRID_COLOR = {51, 65, 85, 255};         // Grid lines
static const SDL_Color X_COLOR = {34, 211, 238, 255};          // Cyan for X
static const SDL_Color O_COLOR = {251, 146, 60, 255};          // Orange for O
static const SDL_Color CELL_HOVER = {51, 65, 85, 150};         // Hover effect

static void draw_text(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color, int x, int y, int center) {
    if (!font) return;
    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_Rect dst = { x, y, surf->w, surf->h };
    if (center) dst.x -= surf->w / 2;
    SDL_FreeSurface(surf);
    if (tex) {
        SDL_RenderCopy(ren, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
}

static void draw_rounded_rect(SDL_Renderer *ren, SDL_Rect r, int radius, SDL_Color color) {
    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, color.a);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    
    // Center rectangles
    SDL_Rect inner = { r.x + radius, r.y, r.w - 2*radius, r.h };
    SDL_RenderFillRect(ren, &inner);
    inner = (SDL_Rect){ r.x, r.y + radius, r.w, r.h - 2*radius };
    SDL_RenderFillRect(ren, &inner);
    
    // Corner circles
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx*dx + dy*dy <= radius*radius) {
                SDL_RenderDrawPoint(ren, r.x + radius + dx, r.y + radius + dy);
                SDL_RenderDrawPoint(ren, r.x + r.w - radius + dx, r.y + radius + dy);
                SDL_RenderDrawPoint(ren, r.x + radius + dx, r.y + r.h - radius + dy);
                SDL_RenderDrawPoint(ren, r.x + r.w - radius + dx, r.y + r.h - radius + dy);
            }
        }
    }
}

static void draw_gradient_rect(SDL_Renderer *ren, SDL_Rect r, SDL_Color c1, SDL_Color c2) {
    for (int y = 0; y < r.h; ++y) {
        float t = (float)y / r.h;
        Uint8 red = (Uint8)(c1.r + (c2.r - c1.r) * t);
        Uint8 green = (Uint8)(c1.g + (c2.g - c1.g) * t);
        Uint8 blue = (Uint8)(c1.b + (c2.b - c1.b) * t);
        SDL_SetRenderDrawColor(ren, red, green, blue, 255);
        SDL_RenderDrawLine(ren, r.x, r.y + y, r.x + r.w, r.y + y);
    }
}

static void render_welcome(SDL_Renderer *ren, TTF_Font *font, TTF_Font *fontSmall, int mouseX, int mouseY, Score *score) {
    // Background
    SDL_SetRenderDrawColor(ren, BG_DARK.r, BG_DARK.g, BG_DARK.b, 255);
    SDL_RenderClear(ren);
    
    // Title card with gradient
    SDL_Rect titleBox = { 100, 80, 600, 120 };
    draw_gradient_rect(ren, titleBox, ACCENT_PRIMARY, ACCENT_SECONDARY);
    draw_rounded_rect(ren, titleBox, 20, (SDL_Color){0, 0, 0, 0}); // Border effect
    
    draw_text(ren, font, "TIC TAC TOE", TEXT_PRIMARY, WINDOW_W/2, 110, 1);
    draw_text(ren, fontSmall, "Modern strategy game", TEXT_SECONDARY, WINDOW_W/2, 155, 1);
    
    // Score display
    SDL_Rect scoreBox = { 100, 230, 600, 100 };
    draw_rounded_rect(ren, scoreBox, 15, BG_CARD);
    
    char scoreText[64];
    snprintf(scoreText, sizeof(scoreText), "X: %d    Draws: %d    O: %d", 
             score->x_wins, score->draws, score->o_wins);
    draw_text(ren, fontSmall, scoreText, TEXT_SECONDARY, WINDOW_W/2, 270, 1);
    
    // Buttons
    SDL_Rect startBtn = { 250, 380, 300, 70 };
    SDL_Rect exitBtn = { 250, 480, 300, 70 };
    
    SDL_Point mp = { mouseX, mouseY };
    bool startHover = SDL_PointInRect(&mp, &startBtn);
    bool exitHover = SDL_PointInRect(&mp, &exitBtn);
    
    // Start button
    if (startHover) {
        draw_gradient_rect(ren, startBtn, ACCENT_PRIMARY, ACCENT_SECONDARY);
    } else {
        draw_rounded_rect(ren, startBtn, 15, ACCENT_PRIMARY);
    }
    draw_text(ren, font, "START GAME", TEXT_PRIMARY, WINDOW_W/2, startBtn.y + 20, 1);
    
    // Exit button
    SDL_Color exitColor = exitHover ? (SDL_Color){71, 85, 105, 255} : BG_CARD;
    draw_rounded_rect(ren, exitBtn, 15, exitColor);
    draw_text(ren, font, "EXIT", TEXT_SECONDARY, WINDOW_W/2, exitBtn.y + 20, 1);
}

static void render_mode_select(SDL_Renderer *ren, TTF_Font *font, TTF_Font *fontSmall, int mouseX, int mouseY) {
    // Background
    SDL_SetRenderDrawColor(ren, BG_DARK.r, BG_DARK.g, BG_DARK.b, 255);
    SDL_RenderClear(ren);
    
    // Title
    draw_text(ren, font, "SELECT GAME MODE", TEXT_PRIMARY, WINDOW_W/2, 100, 1);
    draw_text(ren, fontSmall, "Choose how you want to play", TEXT_SECONDARY, WINDOW_W/2, 150, 1);
    
    // Mode buttons
    SDL_Rect aiBtn = { 150, 250, 500, 100 };
    SDL_Rect twoPlayerBtn = { 150, 380, 500, 100 };
    SDL_Rect backBtn = { 250, 520, 300, 70 };
    
    SDL_Point mp = { mouseX, mouseY };
    bool aiHover = SDL_PointInRect(&mp, &aiBtn);
    bool twoPlayerHover = SDL_PointInRect(&mp, &twoPlayerBtn);
    bool backHover = SDL_PointInRect(&mp, &backBtn);
    
    // AI Mode button
    if (aiHover) {
        draw_gradient_rect(ren, aiBtn, ACCENT_PRIMARY, ACCENT_SECONDARY);
    } else {
        draw_rounded_rect(ren, aiBtn, 15, BG_CARD);
    }
    draw_text(ren, font, "VS AI", TEXT_PRIMARY, WINDOW_W/2, aiBtn.y + 20, 1);
    draw_text(ren, fontSmall, "Play against unbeatable AI", TEXT_SECONDARY, WINDOW_W/2, aiBtn.y + 60, 1);
    
    // Two Player Mode button
    if (twoPlayerHover) {
        draw_gradient_rect(ren, twoPlayerBtn, ACCENT_PRIMARY, ACCENT_SECONDARY);
    } else {
        draw_rounded_rect(ren, twoPlayerBtn, 15, BG_CARD);
    }
    draw_text(ren, font, "TWO PLAYERS", TEXT_PRIMARY, WINDOW_W/2, twoPlayerBtn.y + 20, 1);
    draw_text(ren, fontSmall, "Play with a friend locally", TEXT_SECONDARY, WINDOW_W/2, twoPlayerBtn.y + 60, 1);
    
    // Back button
    SDL_Color backColor = backHover ? (SDL_Color){71, 85, 105, 255} : BG_CARD;
    draw_rounded_rect(ren, backBtn, 15, backColor);
    draw_text(ren, font, "BACK", TEXT_SECONDARY, WINDOW_W/2, backBtn.y + 20, 1);
}

static void render_ai_select(SDL_Renderer *ren, TTF_Font *font, TTF_Font *fontSmall, int mouseX, int mouseY) {
    SDL_SetRenderDrawColor(ren, BG_DARK.r, BG_DARK.g, BG_DARK.b, 255);
    SDL_RenderClear(ren);
    
    draw_text(ren, font, "SELECT AI TYPE", TEXT_PRIMARY, WINDOW_W/2, 80, 1);
    draw_text(ren, fontSmall, "Choose your AI opponent", TEXT_SECONDARY, WINDOW_W/2, 130, 1);
    
    SDL_Rect localBtn = { 100, 200, 600, 120 };
    SDL_Rect openaiBtn = { 100, 350, 600, 120 };
    SDL_Rect backBtn = { 250, 520, 300, 70 };
    
    SDL_Point mp = { mouseX, mouseY };
    bool localHover = SDL_PointInRect(&mp, &localBtn);
    bool openaiHover = SDL_PointInRect(&mp, &openaiBtn);
    bool backHover = SDL_PointInRect(&mp, &backBtn);
    
    // Local AI Button
    if (localHover) {
        draw_gradient_rect(ren, localBtn, (SDL_Color){34, 197, 94, 255}, ACCENT_PRIMARY);
    } else {
        draw_rounded_rect(ren, localBtn, 15, BG_CARD);
    }
    draw_text(ren, font, "LOCAL MINIMAX AI", TEXT_PRIMARY, WINDOW_W/2, localBtn.y + 30, 1);
    draw_text(ren, fontSmall, "Unbeatable | Instant | Free", TEXT_SECONDARY, WINDOW_W/2, localBtn.y + 75, 1);
    
    // OpenAI Button
    if (openaiHover) {
        draw_gradient_rect(ren, openaiBtn, ACCENT_SECONDARY, ACCENT_PRIMARY);
    } else {
        draw_rounded_rect(ren, openaiBtn, 15, BG_CARD);
    }
    draw_text(ren, font, "OPENAI GPT AI", TEXT_PRIMARY, WINDOW_W/2, openaiBtn.y + 30, 1);
    draw_text(ren, fontSmall, "Creative | Requires API Key", TEXT_SECONDARY, WINDOW_W/2, openaiBtn.y + 75, 1);
    
    // Back button
    SDL_Color backColor = backHover ? (SDL_Color){71, 85, 105, 255} : BG_CARD;
    draw_rounded_rect(ren, backBtn, 15, backColor);
    draw_text(ren, font, "BACK", TEXT_SECONDARY, WINDOW_W/2, backBtn.y + 20, 1);
}

static void draw_X(SDL_Renderer *ren, SDL_Rect r, float scale, Uint8 alpha) {
    SDL_SetRenderDrawColor(ren, X_COLOR.r, X_COLOR.g, X_COLOR.b, alpha);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    
    int cx = r.x + r.w/2;
    int cy = r.y + r.h/2;
    int half = (int)(r.w/2 * 0.4 * scale);
    
    // Draw thicker X with glow
    for (int t = -4; t <= 4; ++t) {
        Uint8 glowAlpha = alpha * (1.0f - abs(t) / 5.0f);
        SDL_SetRenderDrawColor(ren, X_COLOR.r, X_COLOR.g, X_COLOR.b, glowAlpha);
        SDL_RenderDrawLine(ren, cx-half, cy-half+t, cx+half, cy+half+t);
        SDL_RenderDrawLine(ren, cx-half, cy+half+t, cx+half, cy-half+t);
    }
}

static void draw_O(SDL_Renderer *ren, SDL_Rect r, float scale, Uint8 alpha) {
    SDL_SetRenderDrawColor(ren, O_COLOR.r, O_COLOR.g, O_COLOR.b, alpha);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    
    int cx = r.x + r.w/2;
    int cy = r.y + r.h/2;
    int radius = (int)(r.w/2 * 0.4 * scale);
    
    // Draw circle with thickness and glow
    for (int r_offset = -4; r_offset <= 4; ++r_offset) {
        int curr_radius = radius + r_offset;
        if (curr_radius < 0) continue;
        Uint8 glowAlpha = alpha * (1.0f - abs(r_offset) / 5.0f);
        SDL_SetRenderDrawColor(ren, O_COLOR.r, O_COLOR.g, O_COLOR.b, glowAlpha);
        
        for (int y = -curr_radius; y <= curr_radius; ++y) {
            int dx = (int)(sqrt((double)(curr_radius*curr_radius - y*y)));
            SDL_RenderDrawPoint(ren, cx-dx, cy+y);
            SDL_RenderDrawPoint(ren, cx+dx, cy+y);
        }
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Tic-Tac-Toe", SDL_WINDOWPOS_CENTERED, 
                                       SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, 
                                       SDL_WINDOW_SHOWN);
    if (!win) {
        fprintf(stderr, "CreateWindow: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        fprintf(stderr, "CreateRenderer: %s\n", SDL_GetError());
        return 1;
    }

    const char *font_paths[] = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        NULL
    };
    
    TTF_Font *font = NULL, *fontSmall = NULL;
    for (int i = 0; font_paths[i]; ++i) {
        font = TTF_OpenFont(font_paths[i], 36);
        if (font) {
            fontSmall = TTF_OpenFont(font_paths[i], 24);
            break;
        }
    }

    bool running = true;
    Scene scene = SCENE_WELCOME;
    GameMode gameMode = MODE_AI;
    SDL_Event e;
    int mouseX = 0, mouseY = 0;

    char board[9];
    init_board(board);
    char current_player = 'X';  // Track current player
    char human = 'X', ai = 'O';
    int player_can_move = 1;  // For AI mode or turn management

    float place_scale[9] = {0};
    Uint8 place_alpha[9] = {0};
    int hover_cell = -1;
    
    Score score = {0, 0, 0};
    
    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            if (e.type == SDL_MOUSEMOTION) {
                mouseX = e.motion.x;
                mouseY = e.motion.y;
                
                // Update hover state for board
                if (scene == SCENE_GAME) {
                    int gridX = 150, gridY = 280, gridSize = 500;
                    int cellW = gridSize / 3;
                    int gx = mouseX - gridX, gy = mouseY - gridY;
                    
                    if (gx >= 0 && gy >= 0 && gx < gridSize && gy < gridSize) {
                        int cx = gx / cellW;
                        int cy = gy / cellW;
                        int idx = cy * 3 + cx;
                        
                        // Show hover for valid moves
                        if (gameMode == MODE_TWO_PLAYER) {
                            hover_cell = (board[idx] == ' ') ? idx : -1;
                        } else {
                            hover_cell = (board[idx] == ' ' && player_can_move) ? idx : -1;
                        }
                    } else {
                        hover_cell = -1;
                    }
                }
            }
            
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;
                
                if (scene == SCENE_WELCOME) {
                    SDL_Rect startBtn = { 250, 380, 300, 70 };
                    SDL_Rect exitBtn = { 250, 480, 300, 70 };
                    
                    if (SDL_PointInRect(&(SDL_Point){mx, my}, &startBtn)) {
                        scene = SCENE_MODE_SELECT;
                    } else if (SDL_PointInRect(&(SDL_Point){mx, my}, &exitBtn)) {
                        running = false;
                    }
                } else if (scene == SCENE_MODE_SELECT) {
                    SDL_Rect aiBtn = { 150, 250, 500, 100 };
                    SDL_Rect twoPlayerBtn = { 150, 380, 500, 100 };
                    SDL_Rect backBtn = { 250, 520, 300, 70 };
                    
                    if (SDL_PointInRect(&(SDL_Point){mx, my}, &aiBtn)) {
                        gameMode = MODE_AI;
                        scene = SCENE_AI_SELECT;
                        
                    } else if (SDL_PointInRect(&(SDL_Point){mx, my}, &twoPlayerBtn)) {
                        gameMode = MODE_TWO_PLAYER;
                        scene = SCENE_GAME;
                        init_board(board);
                        current_player = 'X';
                        player_can_move = 1;
                        for (int i = 0; i < 9; i++) {
                            place_scale[i] = 0.0f;
                            place_alpha[i] = 0;
                        }
                    } else if (SDL_PointInRect(&(SDL_Point){mx, my}, &backBtn)) {
                        scene = SCENE_WELCOME;
                    }
                } else if (scene == SCENE_AI_SELECT) {
                    SDL_Rect localBtn = { 100, 200, 600, 120 };
                    SDL_Rect openaiBtn = { 100, 350, 600, 120 };
                    SDL_Rect backBtn = { 250, 520, 300, 70 };
                    
                    if (SDL_PointInRect(&(SDL_Point){mx, my}, &localBtn)) {
                        // Start game with Local AI
                        scene = SCENE_GAME;
                        init_board(board);
                        current_player = 'X';
                        human = 'X';
                        ai = 'O';
                        player_can_move = 1;
                        for (int i = 0; i < 9; i++) {
                            place_scale[i] = 0.0f;
                            place_alpha[i] = 0;
                        }
                    } else if (SDL_PointInRect(&(SDL_Point){mx, my}, &openaiBtn)) {
                        // For OpenAI - currently falls back to local AI
                        printf("OpenAI not yet implemented. Using Local AI.\n");
                        scene = SCENE_GAME;
                        init_board(board);
                        current_player = 'X';
                        human = 'X';
                        ai = 'O';
                        player_can_move = 1;
                        for (int i = 0; i < 9; i++) {
                            place_scale[i] = 0.0f;
                            place_alpha[i] = 0;
                        }
                    } else if (SDL_PointInRect(&(SDL_Point){mx, my}, &backBtn)) {
                        scene = SCENE_MODE_SELECT;
                    }
                } else if (scene == SCENE_GAME) {
                    int gridX = 150, gridY = 280, gridSize = 500;
                    int cellW = gridSize / 3;
                    int gx = mx - gridX, gy = my - gridY;
                    
                    if (gx >= 0 && gy >= 0 && gx < gridSize && gy < gridSize) {
                        int cx = gx / cellW;
                        int cy = gy / cellW;
                        int idx = cy * 3 + cx;
                        
                        if (board[idx] == ' ') {
                            if (gameMode == MODE_TWO_PLAYER) {
                                // Two player mode - alternate turns
                                board[idx] = current_player;
                                place_scale[idx] = 0.0f;
                                place_alpha[idx] = 0;
                                current_player = (current_player == 'X') ? 'O' : 'X';
                            } else if (gameMode == MODE_AI && player_can_move) {
                                // AI mode - player move
                                board[idx] = human;
                                place_scale[idx] = 0.0f;
                                place_alpha[idx] = 0;
                                player_can_move = 0;
                            }
                        }
                    }
                } else if (scene == SCENE_POPUP) {
                    init_board(board);
                    scene = SCENE_WELCOME;
                    current_player = 'X';
                    for (int i = 0; i < 9; i++) {
                        place_scale[i] = 0.0f;
                        place_alpha[i] = 0;
                    }
                }
            }
        }

        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTime) / 1000.0f;
        lastTime = now;

        // Update animations
        for (int i = 0; i < 9; ++i) {
            if (board[i] != ' ') {
                if (place_scale[i] < 1.0f) {
                    place_scale[i] += dt * 3.5f;
                    if (place_scale[i] > 1.0f) place_scale[i] = 1.0f;
                }
                if (place_alpha[i] < 255) {
                    place_alpha[i] += (Uint8)(dt * 600);
                }
            }
        }

        // AI turn (only in AI mode)
        char w = check_winner(board);
        if (gameMode == MODE_AI && !player_can_move && w == ' ' && scene == SCENE_GAME) {
            SDL_Delay(300);
            int mv = get_best_move(board, ai, human);
            if (board[mv] == ' ') {
                board[mv] = ai;
                place_scale[mv] = 0.0f;
                place_alpha[mv] = 0;
            }
            player_can_move = 1;
        }

        // Check end game
        w = check_winner(board);
        if (w != ' ' && scene == SCENE_GAME) {
            if (w == 'X') score.x_wins++;
            else if (w == 'O') score.o_wins++;
            else if (w == 'T') score.draws++;  // Fixed: properly handle draw
            scene = SCENE_POPUP;
        }

        // Render
        SDL_SetRenderDrawColor(ren, BG_DARK.r, BG_DARK.g, BG_DARK.b, 255);
        SDL_RenderClear(ren);

        if (scene == SCENE_WELCOME) {
            render_welcome(ren, font, fontSmall, mouseX, mouseY, &score);
        } else if (scene == SCENE_MODE_SELECT) {
            render_mode_select(ren, font, fontSmall, mouseX, mouseY);
        } else if (scene == SCENE_AI_SELECT) {
            render_ai_select(ren, font, fontSmall, mouseX, mouseY);
        } else {
            // Game header
            draw_text(ren, font, "TIC TAC TOE", TEXT_PRIMARY, WINDOW_W/2, 50, 1);
            
            char turnText[64];
            if (gameMode == MODE_TWO_PLAYER) {
                snprintf(turnText, sizeof(turnText), "Player %c's turn", current_player);
            } else {
                if (player_can_move) {
                    snprintf(turnText, sizeof(turnText), "Your turn (%c)", human);
                } else {
                    snprintf(turnText, sizeof(turnText), "AI thinking...");
                }
            }
            draw_text(ren, fontSmall, turnText, TEXT_SECONDARY, WINDOW_W/2, 100, 1);
            
            // Score bar
            SDL_Rect scoreBar = { 200, 150, 400, 60 };
            draw_rounded_rect(ren, scoreBar, 10, BG_CARD);
            char scoreText[64];
            snprintf(scoreText, sizeof(scoreText), "X: %d  |  Draws: %d  |  O: %d", 
                     score.x_wins, score.draws, score.o_wins);
            draw_text(ren, fontSmall, scoreText, TEXT_SECONDARY, WINDOW_W/2, 170, 1);
            
            // Game board
            int gridX = 150, gridY = 280, gridSize = 500;
            SDL_Rect gridBg = { gridX - 15, gridY - 15, gridSize + 30, gridSize + 30 };
            draw_rounded_rect(ren, gridBg, 20, BG_CARD);
            
            // Draw grid lines
            SDL_SetRenderDrawColor(ren, GRID_COLOR.r, GRID_COLOR.g, GRID_COLOR.b, 255);
            for (int i = 1; i <= 2; ++i) {
                int x = gridX + i * (gridSize / 3);
                SDL_Rect vr = { x - 2, gridY, 4, gridSize };
                SDL_RenderFillRect(ren, &vr);
                
                int y = gridY + i * (gridSize / 3);
                SDL_Rect hr = { gridX, y - 2, gridSize, 4 };
                SDL_RenderFillRect(ren, &hr);
            }
            
            // Draw hover effect
            if (hover_cell >= 0) {
                int row = hover_cell / 3;
                int col = hover_cell % 3;
                SDL_Rect cell = {
                    gridX + col * (gridSize / 3),
                    gridY + row * (gridSize / 3),
                    gridSize / 3,
                    gridSize / 3
                };
                draw_rounded_rect(ren, cell, 10, CELL_HOVER);
            }
            
            // Draw pieces
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    int idx = r * 3 + c;
                    SDL_Rect cell = {
                        gridX + c * (gridSize / 3),
                        gridY + r * (gridSize / 3),
                        gridSize / 3,
                        gridSize / 3
                    };
                    
                    if (board[idx] == 'X') {
                        draw_X(ren, cell, place_scale[idx], place_alpha[idx]);
                    } else if (board[idx] == 'O') {
                        draw_O(ren, cell, place_scale[idx], place_alpha[idx]);
                    }
                }
            }
            
            // End game popup
            if (scene == SCENE_POPUP) {
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
                SDL_Rect overlay = { 0, 0, WINDOW_W, WINDOW_H };
                SDL_RenderFillRect(ren, &overlay);
                
                SDL_Rect popup = { 150, 300, 500, 250 };
                draw_rounded_rect(ren, popup, 25, BG_CARD);
                
                char msg[64];
                w = check_winner(board);
                
                SDL_Color resultColor = TEXT_PRIMARY;
                
                if (w == 'T') {
                    snprintf(msg, sizeof(msg), "It's a Draw!");
                    resultColor = TEXT_SECONDARY;
                } else if (gameMode == MODE_TWO_PLAYER) {
                    snprintf(msg, sizeof(msg), "Player %c Wins!", w);
                    resultColor = (w == 'X') ? X_COLOR : O_COLOR;
                } else {
                    if (w == human) {
                        snprintf(msg, sizeof(msg), "You Win!");
                        resultColor = X_COLOR;
                    } else {
                        snprintf(msg, sizeof(msg), "AI Wins!");
                        resultColor = O_COLOR;
                    }
                }
                
                draw_text(ren, font, msg, resultColor, WINDOW_W/2, 360, 1);
                draw_text(ren, fontSmall, "Click anywhere to continue", TEXT_SECONDARY, WINDOW_W/2, 480, 1);
            }
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    if (font) TTF_CloseFont(font);
    if (fontSmall) TTF_CloseFont(fontSmall);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}