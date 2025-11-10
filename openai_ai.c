#include "openai_ai.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static char api_key[256] = {0};

typedef struct {
    char *data;
    size_t size;
} ResponseData;

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    ResponseData *mem = (ResponseData *)userp;
    
    char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "Out of memory\n");
        return 0;
    }
    
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
    
    return realsize;
}

int openai_init(const char *key) {
    if (!key || strlen(key) == 0) {
        fprintf(stderr, "Invalid API key\n");
        return -1;
    }
    strncpy(api_key, key, sizeof(api_key) - 1);
    api_key[sizeof(api_key) - 1] = '\0';
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    return 0;
}

void openai_cleanup(void) {
    curl_global_cleanup();
}

static char* call_openai(const char *prompt) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return NULL;
    }
    
    ResponseData response = {0};
    
    // Build JSON request
    char json_request[4096];
    snprintf(json_request, sizeof(json_request),
        "{"
        "\"model\": \"gpt-3.5-turbo\","
        "\"messages\": [{\"role\": \"user\", \"content\": \"%s\"}],"
        "\"temperature\": 0.7,"
        "\"max_tokens\": 150"
        "}", prompt);
    
    // Build authorization header
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_request);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        if (response.data) free(response.data);
        return NULL;
    }
    
    return response.data;
}

int openai_get_move(const char board[9], char player, char opponent) {
    char board_str[32];
    char prompt[512];
    
    // Convert board to string
    for (int i = 0; i < 9; i++) {
        board_str[i] = (board[i] == ' ') ? '-' : board[i];
    }
    board_str[9] = '\0';
    
    snprintf(prompt, sizeof(prompt),
        "You are playing Tic-Tac-Toe. The board is represented as a 9-character string "
        "where positions 0-8 correspond to: 0|1|2, 3|4|5, 6|7|8. "
        "Current board: %s (- means empty). "
        "You are '%c', opponent is '%c'. "
        "Reply with ONLY a single digit 0-8 for your best move. No explanation.",
        board_str, player, opponent);
    
    char *response = call_openai(prompt);
    if (!response) return -1;
    
    // Parse response to extract move
    int move = -1;
    for (size_t i = 0; i < strlen(response); i++) {
        if (response[i] >= '0' && response[i] <= '8') {
            move = response[i] - '0';
            // Verify move is valid
            if (board[move] == ' ') {
                break;
            }
        }
    }
    
    free(response);
    
    // Fallback: find first empty space if parsing failed
    if (move == -1 || board[move] != ' ') {
        for (int i = 0; i < 9; i++) {
            if (board[i] == ' ') {
                move = i;
                break;
            }
        }
    }
    
    return move;
}

char* openai_explain_move(const char board[9], int move, char player) {
    char board_str[32];
    char prompt[512];
    
    for (int i = 0; i < 9; i++) {
        board_str[i] = (board[i] == ' ') ? '-' : board[i];
    }
    board_str[9] = '\0';
    
    snprintf(prompt, sizeof(prompt),
        "Explain in 1-2 sentences why playing '%c' at position %d "
        "is a good move in this Tic-Tac-Toe board: %s (positions 0-8).",
        player, move, board_str);
    
    char *response = call_openai(prompt);
    if (!response) {
        return strdup("AI is thinking about this move.");
    }
    
    // Extract content from JSON (simple parsing)
    char *content_start = strstr(response, "\"content\":");
    if (content_start) {
        content_start = strchr(content_start, '"');
        if (content_start) {
            content_start++; // Skip opening quote
            char *content_end = strchr(content_start + 1, '"');
            if (content_end) {
                size_t len = content_end - content_start;
                char *result = malloc(len + 1);
                if (result) {
                    strncpy(result, content_start, len);
                    result[len] = '\0';
                    free(response);
                    return result;
                }
            }
        }
    }
    
    return response;
}