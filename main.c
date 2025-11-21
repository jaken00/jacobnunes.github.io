#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
//C:\msys64\msys2_shell.cmd -defterm -here -no-start -mingw64
#define SCREEN_WIDTH 512
#define SCREEN_HEIGHT 512
#define MAP_SIZE 34
#define SQUARE_X_SIZE 16
#define SQUARE_Y_SIZE 16

typedef struct BodyNode {
    int x;
    int y;
    struct BodyNode* next;
} BodyNode;
typedef struct {
    int x_pos_head;
    int y_pos_head;
    int x_dir;
    int y_dir;
    int length;  
    BodyNode* body; 

} Snake;
typedef struct {
    int mapBlocks[34][34];
    int fruit_count;
    bool gameOver;
} MapStruct;

typedef struct {
    int num_fruits;
    bool is_eaten;
} Gamestate;

// Global game state for Emscripten main loop
typedef struct {
    Snake snake;
    MapStruct mapstruct;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Rect draw_square;
    Uint32 last_move_time;
    bool quit;
} GameState;

GameState* g_gameState = NULL;

void generateFruit(MapStruct *mapStruct){
    int x, y;
    
    do {
        x = rand() % 32 + 1; 
        y = rand() % 32 + 1;
    } while (mapStruct->mapBlocks[y][x] != 0);  

    mapStruct->mapBlocks[y][x] = 3;
    mapStruct->fruit_count += 1;
}

BodyNode* createBodyNode(int x, int y){
    BodyNode* newBody = malloc(sizeof(BodyNode));
    newBody->x = x;
    newBody->y = y;
    newBody->next = NULL;

    return newBody;
}

Snake initSnake(){
    Snake snake;
    snake.x_pos_head = 4;
    snake.y_pos_head = 8;
    snake.length = 2; 
    snake.x_dir = 0;
    snake.y_dir = 1;    
    snake.body = createBodyNode(snake.x_pos_head, snake.y_pos_head);

    return snake;
}



void moveSnake(MapStruct *map, Snake *snake){

    int old_x = snake->x_pos_head;
    int old_y = snake->y_pos_head;

    int new_x = snake->x_pos_head + snake->x_dir;
    int new_y = snake->y_pos_head + snake->y_dir;

    bool hitWall = (map->mapBlocks[new_y][new_x] == 1);
    bool hitSelf = (map->mapBlocks[new_y][new_x] == 2);

    if(hitSelf || hitWall){
        BodyNode* startPoint = snake->body->next;
        BodyNode* freeBody = startPoint;
        BodyNode* freeHolder = NULL;
        while(freeBody != NULL){
            
            map->mapBlocks[freeBody->y][freeBody->x] = 0;
            freeHolder = freeBody;
            freeBody = freeBody->next;
            free(freeHolder);

        }
        map->mapBlocks[snake->body->y][snake->body->x] = 0;
        map->mapBlocks[snake->y_pos_head][snake->x_pos_head] = 0;
        free(snake->body);

        snake->x_pos_head = 4;
        snake->y_pos_head = 8;
        snake->length = 2; 
        snake->x_dir = 0;
        snake->y_dir = 1; 
        snake->body = createBodyNode(snake->x_pos_head, snake->y_pos_head);

        return;
    }

    map->mapBlocks[snake->y_pos_head][snake->x_pos_head] = 0;
    snake->x_pos_head = new_x;
    snake->y_pos_head = new_y;

    bool justAte = (map->mapBlocks[snake->y_pos_head][snake->x_pos_head] == 3);
    map->mapBlocks[snake->y_pos_head][snake->x_pos_head] = 2;

    BodyNode* newSegment = createBodyNode(old_x, old_y);
    newSegment->next = snake->body;
    snake->body = newSegment;

    if(!justAte){
        BodyNode* current = snake->body;
        BodyNode* previous = NULL;

        while(current->next != NULL){
            previous = current;
            current = current->next;
        }
        if(previous != NULL){
            previous->next = NULL;
            map->mapBlocks[current->y][current->x] = 0;
            free(current);
        } 
    
    BodyNode* currentDraw = snake->body;
    while(currentDraw != NULL){
        map->mapBlocks[currentDraw->y][currentDraw->x] = 2;
        currentDraw = currentDraw->next;
    }
    }
    else {
        generateFruit(map);
    }
}


bool canvas_map(MapStruct* map){
    int current_map_number = 0;
    for(int i = 0; i < MAP_SIZE; i++) {
        for(int j = 0; j < MAP_SIZE; j++) {
            current_map_number = map->mapBlocks[i][j];
            if(current_map_number == 3){
                return true;
            }
        }
    }
    return false;
}

void game_loop(void* arg) {
    GameState* state = (GameState*)arg;
    if (state->quit) {
        #ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
        #endif
        SDL_DestroyWindow(state->window);
        SDL_DestroyRenderer(state->renderer);
        SDL_Quit();
        free(state);
        return;
    }

    Uint32 current_time = SDL_GetTicks();

    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) state->quit = true;
    }

    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    if(keystate[SDL_SCANCODE_Q]) state->quit = true;

    if(keystate[SDL_SCANCODE_W]){
        if(state->snake.x_dir != 1){
            state->snake.y_dir = 0;
            state->snake.x_dir = -1;
        }
    }
    if(keystate[SDL_SCANCODE_S]){
        if(state->snake.x_dir != -1){
            state->snake.y_dir = 0;
            state->snake.x_dir = 1;
        }
    }
    if(keystate[SDL_SCANCODE_D]){
        if(state->snake.y_dir != -1){
            state->snake.y_dir = 1;
            state->snake.x_dir = 0;
        }
    }
    if(keystate[SDL_SCANCODE_A]){
        if(state->snake.y_dir != 1){
            state->snake.y_dir = -1;
            state->snake.x_dir = 0;
        }
    }
    if(current_time - state->last_move_time >= 100) {
        moveSnake(&state->mapstruct, &state->snake);
        state->last_move_time = current_time;
    }
    
    SDL_RenderClear(state->renderer);
    for(int i = 1; i < 33; i++){     
        for(int j = 1; j < 33; j++){ 
            int current_location = state->mapstruct.mapBlocks[i][j];
            switch (current_location)
            {
            case 0:
                SDL_SetRenderDrawColor(state->renderer, 50, 50, 50, 255);
                state->draw_square.x = (i - 1) * SQUARE_X_SIZE;
                state->draw_square.y = (j - 1) * SQUARE_Y_SIZE;
                SDL_RenderFillRect(state->renderer, &state->draw_square);
                break;
            case 2:
                SDL_SetRenderDrawColor(state->renderer, 0, 255, 255, 255);
                state->draw_square.x = (i - 1) * SQUARE_X_SIZE;
                state->draw_square.y = (j - 1) * SQUARE_Y_SIZE;
                SDL_RenderFillRect(state->renderer, &state->draw_square);
                break;
            case 3:
                SDL_SetRenderDrawColor(state->renderer, 255, 255, 0, 255);
                state->draw_square.x = (i - 1) * SQUARE_X_SIZE;
                state->draw_square.y = (j - 1) * SQUARE_Y_SIZE;
                SDL_RenderFillRect(state->renderer, &state->draw_square);
                break;
            default:
                break;
            }
        }
    }

    SDL_RenderPresent(state->renderer);
}

int main(int argc, char* argv[]) {
    srand(time(NULL)); // Initialize random seed
    Snake snake = initSnake();
    // 0 IS EMPTY , 1 IS WALL, 2 IS PLAYER, 3 WILL BE FOOD
    int map[34][34];
    for(int i = 0; i < MAP_SIZE; i++) {
            for(int j = 0; j < MAP_SIZE; j++) {
                if(i == 0 || j == 33 || i == 33 || j == 0){
                    map[i][j] = 1;
                }
                else{
                    map[i][j] = 0;
                }
            
        }
    }

    map[snake.y_pos_head][snake.x_pos_head] = 2;

    MapStruct mapstruct;
    memcpy(mapstruct.mapBlocks, map, sizeof(map));
    mapstruct.fruit_count = 0;
    mapstruct.gameOver = false;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow(
        "Snake",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); 
    if (renderer == NULL){
        // Try software renderer as fallback
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (renderer == NULL){
            printf("Unable to create renderer Error: %s\n", SDL_GetError());
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }

    SDL_Rect draw_square;
    draw_square.x = 1;
    draw_square.y = 1;
    draw_square.w = 16;
    draw_square.h = 16;

    Uint32 last_move_time = 0;
    generateFruit(&mapstruct);
    
    // Allocate game state
    GameState* gameState = (GameState*)malloc(sizeof(GameState));
    gameState->snake = snake;
    gameState->mapstruct = mapstruct;
    gameState->window = window;
    gameState->renderer = renderer;
    gameState->draw_square = draw_square;
    gameState->last_move_time = last_move_time;
    gameState->quit = false;
    g_gameState = gameState;

    #ifdef __EMSCRIPTEN__
    // Use Emscripten's main loop
    emscripten_set_main_loop_arg(game_loop, gameState, 0, 1);
    #else
    // Standard desktop main loop
    while(!gameState->quit) {
        game_loop(gameState);
        SDL_Delay(16);
    }
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    free(gameState);
    #endif
    
    return 0;
}
