#include <SDL.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "game.h"
#include "ai.h"
#include "openai_ai.h" /* optional; implement as a separate module if you want OpenAI support */

/* Modern Tic-Tac-Toe with OpenAI Integration (completed)
   - Dark modern theme with gradient accents
   - Smooth animations and transitions
   - Glow effects and hover states
   - Score tracking
   - Better visual feedback
   - AI vs 2 Player modes
   - LOCAL AI vs OPENAI AI options (OpenAI optional)

   Notes:
   - Do NOT hardcode API keys in source. Set environment variable OPENAI_API_KEY if you want OpenAI features.
   - This file expects game.h / ai.h to provide: init_board, check_winner, get_best_move.
*/

static const int WINDOW_W = 800;
static const int WINDOW_H = 900;

typedef enum { SCENE_WELCOME, SCENE_MODE_SELECT, SCENE_AI_SELECT, SCENE_GAME, SCENE_POPUP } Scene;
typedef enum { MODE_AI, MODE_TWO_PLAYER } GameMode;
typedef enum { AI_LOCAL, AI_OPENAI } AIType;

typedef struct {
    int x_wins;
    int o_wins;
    int draws;
} Score;

// Colors
static const SDL_Color BG_DARK = {15, 23, 42, 255};
static const SDL_Color BG_CARD = {30, 41, 59, 255};
static const SDL_Color ACCENT_PRIMARY = {99, 102, 241, 255};
static const SDL_Color ACCENT_SECONDARY = {168, 85, 247, 255};
static const SDL_Color TEXT_PRIMARY = {248, 250, 252, 255};
static const SDL_Color TEXT_SECONDARY = {148, 163, 184, 255};
static const SDL_Color GRID_COLOR = {51, 65, 85, 255};
static const SDL_Color X_COLOR = {34, 211, 238, 255};
static const SDL_Color O_COLOR = {251, 146, 60, 255};
static const SDL_Color CELL_HOVER = {51, 65, 85, 150};
static const SDL_Color SUCCESS_COLOR = {34, 197, 94, 255};
static const SDL_Color WARNING_COLOR = {234, 179, 8, 255};

static void draw_text(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color color, int x, int y, int center) {
    if (!font || !text) return;
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

    SDL_Rect inner = { r.x + radius, r.y, r.w - 2*radius, r.h };
    SDL_RenderFillRect(ren, &inner);
    inner = (SDL_Rect){ r.x, r.y + radius, r.w, r.h - 2*radius };
    SDL_RenderFillRect(ren, &inner);

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
        float t = (float)y / (float)r.h;
        Uint8 red = (Uint8)(c1.r + (c2.r - c1.r) * t);
        Uint8 green = (Uint8)(c1.g + (c2.g - c1.g) * t);
        Uint8 blue = (Uint8)(c1.b + (c2.b - c1.b) * t);
        SDL_SetRenderDrawColor(ren, red, green, blue, 255);
        SDL_RenderDrawLine(ren, r.x, r.y + y, r.x + r.w, r.y + y);
    }
}

static void render_welcome(SDL_Renderer *ren, TTF_Font *font, TTF_Font *fontSmall, int mouseX, int mouseY, Score *score) {
    SDL_SetRenderDrawColor(ren, BG_DARK.r, BG_DARK.g, BG_DARK.b, 255);
    SDL_RenderClear(ren);

    SDL_Rect titleBox = { 100, 80, 600, 120 };
    draw_gradient_rect(ren, titleBox, ACCENT_PRIMARY, ACCENT_SECONDARY);

    draw_text(ren, font, "TIC TAC TOE", TEXT_PRIMARY, WINDOW_W/2, 110, 1);
    draw_text(ren, fontSmall, "Modern strategy game with AI", TEXT_SECONDARY, WINDOW_W/2, 155, 1);

    SDL_Rect scoreBox = { 100, 230, 600, 100 };
    draw_rounded_rect(ren, scoreBox, 15, BG_CARD);

    char scoreText[64];
    snprintf(scoreText, sizeof(scoreText), "X: %d    Draws: %d    O: %d",
             score->x_wins, score->draws, score->o_wins);
    draw_text(ren, fontSmall, scoreText, TEXT_SECONDARY, WINDOW_W/2, 270, 1);

    SDL_Rect startBtn = { 250, 380, 300, 70 };
    SDL_Rect exitBtn = { 250, 480, 300, 70 };

    SDL_Point mp = { mouseX, mouseY };
    bool startHover = SDL_PointInRect(&mp, &startBtn);
    bool exitHover = SDL_PointInRect(&mp, &exitBtn);

    if (startHover) {
        draw_gradient_rect(ren, startBtn, ACCENT_PRIMARY, ACCENT_SECONDARY);
    } else {
        draw_rounded_rect(ren, startBtn, 15, ACCENT_PRIMARY);
    }
    draw_text(ren, font, "START GAME", TEXT_PRIMARY, WINDOW_W/2, startBtn.y + 20, 1);

    SDL_Color exitColor = exitHover ? (SDL_Color){71, 85, 105, 255} : BG_CARD;
    draw_rounded_rect(ren, exitBtn, 15, exitColor);
    draw_text(ren, font, "EXIT", TEXT_SECONDARY, WINDOW_W/2, exitBtn.y + 20, 1);
}

static void render_mode_select(SDL_Renderer *ren, TTF_Font *font, TTF_Font *fontSmall, int mouseX, int mouseY) {
    SDL_SetRenderDrawColor(ren, BG_DARK.r, BG_DARK.g, BG_DARK.b, 255);
    SDL_RenderClear(ren);

    draw_text(ren, font, "SELECT GAME MODE", TEXT_PRIMARY, WINDOW_W/2, 100, 1);
    draw_text(ren, fontSmall, "Choose how you want to play", TEXT_SECONDARY, WINDOW_W/2, 150, 1);

    SDL_Rect aiBtn = { 150, 250, 500, 100 };
    SDL_Rect twoPlayerBtn = { 150, 380, 500, 100 };
    SDL_Rect backBtn = { 250, 520, 300, 70 };

    SDL_Point mp = { mouseX, mouseY };
    bool aiHover = SDL_PointInRect(&mp, &aiBtn);
    bool twoPlayerHover = SDL_PointInRect(&mp, &twoPlayerBtn);
    bool backHover = SDL_PointInRect(&mp, &backBtn);

    if (aiHover) {
        draw_gradient_rect(ren, aiBtn, ACCENT_PRIMARY, ACCENT_SECONDARY);
    } else {
        draw_rounded_rect(ren, aiBtn, 15, BG_CARD);
    }
    draw_text(ren, font, "VS AI", TEXT_PRIMARY, WINDOW_W/2, aiBtn.y + 20, 1);
    draw_text(ren, fontSmall, "Play against AI opponent", TEXT_SECONDARY, WINDOW_W/2, aiBtn.y + 60, 1);

    if (twoPlayerHover) {
        draw_gradient_rect(ren, twoPlayerBtn, ACCENT_PRIMARY, ACCENT_SECONDARY);
    } else {
        draw_rounded_rect(ren, twoPlayerBtn, 15, BG_CARD);
    }
    draw_text(ren, font, "TWO PLAYERS", TEXT_PRIMARY, WINDOW_W/2, twoPlayerBtn.y + 20, 1);
    draw_text(ren, fontSmall, "Play with a friend locally", TEXT_SECONDARY, WINDOW_W/2, twoPlayerBtn.y + 60, 1);

    SDL_Color backColor = backHover ? (SDL_Color){71, 85, 105, 255} : BG_CARD;
    draw_rounded_rect(ren, backBtn, 15, backColor);
    draw_text(ren, font, "BACK", TEXT_SECONDARY, WINDOW_W/2, backBtn.y + 20, 1);
}

static void render_ai_select(SDL_Renderer *ren, TTF_Font *font, TTF_Font *fontSmall, TTF_Font *fontTiny, int mouseX, int mouseY) {
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
        draw_gradient_rect(ren, localBtn, SUCCESS_COLOR, ACCENT_PRIMARY);
    } else {
        draw_rounded_rect(ren, localBtn, 15, BG_CARD);
    }
    draw_text(ren, font, "LOCAL MINIMAX AI", TEXT_PRIMARY, WINDOW_W/2, localBtn.y + 25, 1);
    draw_text(ren, fontSmall, "Unbeatable | Instant | Free", SUCCESS_COLOR, WINDOW_W/2, localBtn.y + 65, 1);
    draw_text(ren, fontTiny, "Perfect play, no internet required", TEXT_SECONDARY, WINDOW_W/2, localBtn.y + 92, 1);

    // OpenAI Button
    if (openaiHover) {
        draw_gradient_rect(ren, openaiBtn, ACCENT_SECONDARY, ACCENT_PRIMARY);
    } else {
        draw_rounded_rect(ren, openaiBtn, 15, BG_CARD);
    }
    draw_text(ren, font, "OPENAI GPT AI", TEXT_PRIMARY, WINDOW_W/2, openaiBtn.y + 25, 1);
    draw_text(ren, fontSmall, "Creative | Explains Moves | requires internet", WARNING_COLOR, WINDOW_W/2, openaiBtn.y + 65, 1);
    draw_text(ren, fontTiny, "Powered by a remote model", TEXT_SECONDARY, WINDOW_W/2, openaiBtn.y + 92, 1);

    // Back button
    SDL_Color backColor = backHover ? (SDL_Color){71, 85, 105, 255} : BG_CARD;
    draw_rounded_rect(ren, backBtn, 15, backColor);
    draw_text(ren, font, "BACK", TEXT_SECONDARY, WINDOW_W/2, backBtn.y + 20, 1);
}

static void draw_X(SDL_Renderer *ren, SDL_Rect r, float scale, Uint8 alpha) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    int cx = r.x + r.w/2;
    int cy = r.y + r.h/2;
    int half = (int)(r.w/2 * 0.4f * scale);
    for (int t = -4; t <= 4; ++t) {
        Uint8 glowAlpha = (Uint8)(alpha * (1.0f - fabsf((float)t) / 5.0f));
        SDL_SetRenderDrawColor(ren, X_COLOR.r, X_COLOR.g, X_COLOR.b, glowAlpha);
        SDL_RenderDrawLine(ren, cx-half, cy-half+t, cx+half, cy+half+t);
        SDL_RenderDrawLine(ren, cx-half, cy+half+t, cx+half, cy-half+t);
    }
}

static void draw_O(SDL_Renderer *ren, SDL_Rect r, float scale, Uint8 alpha) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    int cx = r.x + r.w/2;
    int cy = r.y + r.h/2;
    int radius = (int)(r.w/2 * 0.4f * scale);
    for (int r_offset = -4; r_offset <= 4; ++r_offset) {
        int curr_radius = radius + r_offset;
        if (curr_radius < 0) continue;
        Uint8 glowAlpha = (Uint8)(alpha * (1.0f - fabsf((float)r_offset) / 5.0f));
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

    // Initialize OpenAI (if you want remote AI). Use env var instead of hardcoding key.
    const char *api_key = getenv("OPENAI_API_KEY");
    bool openai_available = false;
    if (api_key && strlen(api_key) > 0) {
        if (openai_init(api_key) == 0) openai_available = true;
    }

    SDL_Window *win = SDL_CreateWindow("Tic-Tac-Toe with OpenAI", SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    if (!win) {
        fprintf(stderr, "CreateWindow: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        fprintf(stderr, "CreateRenderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        return 1;
    }

    const char *font_paths[] = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        NULL
    };

    TTF_Font *font = NULL, *fontSmall = NULL, *fontTiny = NULL;
    for (int i = 0; font_paths[i]; ++i) {
        font = TTF_OpenFont(font_paths[i], 36);
        if (font) {
            fontSmall = TTF_OpenFont(font_paths[i], 24);
            fontTiny = TTF_OpenFont(font_paths[i], 16);
            break;
        }
    }
    if (!font) {
        fprintf(stderr, "Warning: no system font found; text may not render.\n");
    }

    bool running = true;
    Scene scene = SCENE_WELCOME;
    GameMode gameMode = MODE_AI;
    AIType aiType = AI_LOCAL;
    SDL_Event e;
    int mouseX = 0, mouseY = 0;

    char board[9];
    init_board(board);
    char current_player = 'X';
    char human = 'X', ai = 'O';
    int player_can_move = 1;
    bool ai_thinking = false;

    float place_scale[9];
    Uint8 place_alpha[9];
    for (int i=0;i<9;++i){ place_scale[i]=0.0f; place_alpha[i]=0; }
    int hover_cell = -1;

    Score score = {0,0,0};
    Uint32 lastTime = SDL_GetTicks();
    Uint32 aiThinkStartTime = 0;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            if (e.type == SDL_MOUSEMOTION) {
                mouseX = e.motion.x;
                mouseY = e.motion.y;
                if (scene == SCENE_GAME) {
                    int gridX = 150, gridY = 280, gridSize = 500;
                    int cellW = gridSize / 3;
                    int gx = mouseX - gridX, gy = mouseY - gridY;
                    if (gx >= 0 && gy >= 0 && gx < gridSize && gy < gridSize) {
                        int cx = gx / cellW;
                        int cy = gy / cellW;
                        int idx = cy * 3 + cx;
                        if (gameMode == MODE_TWO_PLAYER) {
                            hover_cell = (board[idx] == ' ') ? idx : -1;
                        } else {
                            hover_cell = (board[idx] == ' ' && player_can_move && !ai_thinking) ? idx : -1;
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
                        current_player = 'X'; player_can_move = 1; ai_thinking = false;
                        for (int i=0;i<9;++i){ place_scale[i]=0.0f; place_alpha[i]=0; }
                    } else if (SDL_PointInRect(&(SDL_Point){mx, my}, &backBtn)) {
                        scene = SCENE_WELCOME;
                    }
                } else if (scene == SCENE_AI_SELECT) {
                    SDL_Rect localBtn = { 100, 200, 600, 120 };
                    SDL_Rect openaiBtn = { 100, 350, 600, 120 };
                    SDL_Rect backBtn = { 250, 520, 300, 70 };
                    if (SDL_PointInRect(&(SDL_Point){mx, my}, &localBtn)) {
                        aiType = AI_LOCAL;
                        scene = SCENE_GAME;
                        init_board(board);
                        current_player = 'X'; human = 'X'; ai = 'O'; player_can_move = 1; ai_thinking = false;
                        for (int i=0;i<9;++i){ place_scale[i]=0.0f; place_alpha[i]=0; }
                    } else if (SDL_PointInRect(&(SDL_Point){mx, my}, &openaiBtn)) {
                        if (openai_available) {
                            aiType = AI_OPENAI;
                            scene = SCENE_GAME;
                            init_board(board);
                            current_player = 'X'; human = 'X'; ai = 'O'; player_can_move = 1; ai_thinking = false;
                            for (int i=0;i<9;++i){ place_scale[i]=0.0f; place_alpha[i]=0; }
                        }
                    } else if (SDL_PointInRect(&(SDL_Point){mx, my}, &backBtn)) {
                        scene = SCENE_MODE_SELECT;
                    }
                } else if (scene == SCENE_GAME && !ai_thinking) {
                    int gridX = 150, gridY = 280, gridSize = 500;
                    int cellW = gridSize / 3;
                    int gx = mx - gridX, gy = my - gridY;
                    if (gx >= 0 && gy >= 0 && gx < gridSize && gy < gridSize) {
                        int cx = gx / cellW;
                        int cy = gy / cellW;
                        int idx = cy * 3 + cx;
                        if (board[idx] == ' ') {
                            if (gameMode == MODE_TWO_PLAYER) {
                                board[idx] = current_player;
                                place_scale[idx] = 0.0f; place_alpha[idx] = 0;
                                current_player = (current_player == 'X') ? 'O' : 'X';
                            } else if (gameMode == MODE_AI && player_can_move) {
                                board[idx] = human;
                                place_scale[idx] = 0.0f; place_alpha[idx] = 0;
                                player_can_move = 0; ai_thinking = true; aiThinkStartTime = SDL_GetTicks();
                            }
                        }
                    }
                } else if (scene == SCENE_POPUP) {
                    init_board(board);
                    scene = SCENE_WELCOME;
                    current_player = 'X'; ai_thinking = false;
                    for (int i=0;i<9;++i){ place_scale[i]=0.0f; place_alpha[i]=0; }
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
                    int inc = (int)(dt * 600);
                    int val = place_alpha[i] + inc;
                    if (val > 255) val = 255;
                    place_alpha[i] = (Uint8)val;
                }
            }
        }

        // AI turn (only in AI mode)
        char w = check_winner(board);
        if (gameMode == MODE_AI && ai_thinking && w == ' ' && scene == SCENE_GAME) {
            if (now - aiThinkStartTime > (aiType == AI_OPENAI ? 500 : 300)) {
                int mv = -1;
                if (aiType == AI_OPENAI && openai_available) {
                    mv = openai_get_move(board, ai, human);
                    if (mv < 0 || mv >= 9 || board[mv] != ' ') {
                        printf("OpenAI failed or returned invalid move, using local AI fallback\n");
                        mv = get_best_move(board, ai, human);
                    }
                } else {
                    mv = get_best_move(board, ai, human);
                }
                if (mv >= 0 && mv < 9 && board[mv] == ' ') {
                    board[mv] = ai;
                    place_scale[mv] = 0.0f; place_alpha[mv] = 0;
                }
                player_can_move = 1; ai_thinking = false;
            }
        }

        // Check end game
        w = check_winner(board);
        if (w != ' ' && scene == SCENE_GAME && !ai_thinking) {
            if (w == 'X') score.x_wins++;
            else if (w == 'O') score.o_wins++;
            else if (w == 'T') score.draws++;
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
            render_ai_select(ren, font, fontSmall, fontTiny, mouseX, mouseY);
        } else {
            // Game header
            draw_text(ren, font, "TIC TAC TOE", TEXT_PRIMARY, WINDOW_W/2, 50, 1);

            char turnText[128];
            if (gameMode == MODE_TWO_PLAYER) {
                snprintf(turnText, sizeof(turnText), "Player %c's turn", current_player);
            } else {
                if (ai_thinking) {
                    if (aiType == AI_OPENAI) snprintf(turnText, sizeof(turnText), "OpenAI is thinking...");
                    else snprintf(turnText, sizeof(turnText), "AI is thinking...");
                } else if (player_can_move) {
                    snprintf(turnText, sizeof(turnText), "Your turn (%c)", human);
                } else {
                    snprintf(turnText, sizeof(turnText), "Waiting for AI...");
                }
            }
            draw_text(ren, fontSmall, turnText, TEXT_SECONDARY, WINDOW_W/2, 100, 1);

            // Draw board card
            SDL_Rect boardCard = { 130, 260, 540, 540 };
            draw_rounded_rect(ren, boardCard, 20, BG_CARD);

            int gridX = 150, gridY = 280, gridSize = 500;
            int cellW = gridSize / 3;

            // Draw grid lines
            SDL_SetRenderDrawColor(ren, GRID_COLOR.r, GRID_COLOR.g, GRID_COLOR.b, GRID_COLOR.a);
            for (int i = 1; i <= 2; ++i) {
                int x = gridX + i * cellW;
                SDL_RenderDrawLine(ren, x, gridY, x, gridY + gridSize);
                int y = gridY + i * cellW;
                SDL_RenderDrawLine(ren, gridX, y, gridX + gridSize, y);
            }

            // Draw hover cell highlight
            if (hover_cell >= 0) {
                int cx = hover_cell % 3;
                int cy = hover_cell / 3;
                SDL_Rect cellRect = { gridX + cx*cellW + 4, gridY + cy*cellW + 4, cellW - 8, cellW - 8 };
                draw_rounded_rect(ren, cellRect, 10, CELL_HOVER);
            }

            // Draw X/O
            for (int i = 0; i < 9; ++i) {
                int cx = i % 3;
                int cy = i / 3;
                SDL_Rect r = { gridX + cx*cellW + 8, gridY + cy*cellW + 8, cellW - 16, cellW - 16 };
                if (board[i] == 'X') draw_X(ren, r, place_scale[i], place_alpha[i]);
                else if (board[i] == 'O') draw_O(ren, r, place_scale[i], place_alpha[i]);
            }

            // Footer: controls
            SDL_Rect restartBtn = { 150, 840, 200, 40 };
            SDL_Rect menuBtn = { 450, 840, 200, 40 };
            SDL_Point mp = { mouseX, mouseY };
            bool restartHover = SDL_PointInRect(&mp, &restartBtn);
            bool menuHover = SDL_PointInRect(&mp, &menuBtn);
            SDL_Color restartColor = restartHover ? ACCENT_PRIMARY : BG_CARD;
            SDL_Color menuColor = menuHover ? ACCENT_SECONDARY : BG_CARD;
            draw_rounded_rect(ren, restartBtn, 8, restartColor);
            draw_rounded_rect(ren, menuBtn, 8, menuColor);
            draw_text(ren, fontSmall, "RESTART", TEXT_PRIMARY, restartBtn.x + restartBtn.w/2, restartBtn.y + 8, 1);
            draw_text(ren, fontSmall, "MENU", TEXT_PRIMARY, menuBtn.x + menuBtn.w/2, menuBtn.y + 8, 1);

            // If popup show result overlay
            if (scene == SCENE_POPUP) {
                SDL_Rect overlay = { 160, 300, 480, 240 };
                draw_rounded_rect(ren, overlay, 12, BG_CARD);
                char resultText[64];
                if (w == 'X') snprintf(resultText, sizeof(resultText), "X wins!");
                else if (w == 'O') snprintf(resultText, sizeof(resultText), "O wins!");
                else snprintf(resultText, sizeof(resultText), "Draw!");
                draw_text(ren, font, resultText, TEXT_PRIMARY, WINDOW_W/2, overlay.y + 40, 1);
                draw_text(ren, fontSmall, "Click anywhere to continue", TEXT_SECONDARY, WINDOW_W/2, overlay.y + 90, 1);
            }
        }

        SDL_RenderPresent(ren);

        // small delay to avoid 100% CPU
        SDL_Delay(8);
    }

    // Cleanup
    if (openai_available) openai_shutdown();
    if (font) TTF_CloseFont(font);
    if (fontSmall) TTF_CloseFont(fontSmall);
    if (fontTiny) TTF_CloseFont(fontTiny);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
