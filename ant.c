#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include "lodepng.h"

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 1000
#define N_OUTPUTFILENAME 100

#define N_DIRECTIONS 4
enum Direction {UP, RIGHT, DOWN, LEFT};

#define N_COLORS 4
const uint8_t COLORS[N_COLORS][3] = {
    {255, 255, 255},
    {0, 255, 0},
    {0, 0, 255},
    {255, 0, 0},
};
const int COLOR_ACTION[N_COLORS] = {
    1,
    -1,
    -1,
    1,
};

void checkerr(void *p, const char *message) {
    if (p == NULL) {
        fputs(message, stderr);
        exit(1);
    }
}

#define SDLRECT_WIDTH 1 /* Must be a divisor of SCREEN_WIDTH */
#define SDLRECT_HEIGHT 1 /* Must be a divisor of SCREEN_HEIGHT */
struct Square {
    int color;
    SDL_Rect rect;
};

const size_t SQUARES_X_LEN = (SCREEN_WIDTH / SDLRECT_WIDTH);
const size_t SQUARES_Y_LEN = (SCREEN_HEIGHT / SDLRECT_HEIGHT);
struct Square *squares_init(size_t squares_x_len, size_t squares_y_len) {

    struct Square *squares =
        malloc(sizeof(struct Square) * SQUARES_X_LEN * SQUARES_Y_LEN);
    checkerr(squares, "Cannot allocate squares");

    size_t index = 0;
    for (size_t y = 0; y < squares_y_len; y++) {
        for (size_t x = 0; x < squares_x_len; x++, index++) {
            squares[index].color = 0;
            squares[index].rect.x = x * SDLRECT_WIDTH;
            squares[index].rect.y = y * SDLRECT_HEIGHT;
            squares[index].rect.w = SDLRECT_WIDTH;
            squares[index].rect.h = SDLRECT_HEIGHT;
        }
    }

    return squares;
}

#define ANT_STEP 1
struct Ant {
    enum Direction direction;
    ssize_t x;
    ssize_t y;
};

struct Board {
    struct Ant *ants;
    size_t ants_len;
    struct Square *squares; /* 2D array in an 1D array */
    size_t squares_x_len;
    size_t squares_y_len;
};

struct Board *board_init() {
    struct Board *board = malloc(sizeof(struct Board));
    checkerr(board, "Cannot allocate board");

    board->squares_x_len = SQUARES_X_LEN;
    board->squares_y_len = SQUARES_Y_LEN;
    board->squares = squares_init(board->squares_x_len, board->squares_y_len);

    board->ants_len = 5;
    board->ants = malloc(sizeof(struct Ant) * board->ants_len);
    checkerr(board->ants, "Cannot allocate ants");

    board->ants[0].direction = UP;
    board->ants[0].x = board->squares_x_len/2;
    board->ants[0].y = board->squares_y_len/2;

    board->ants[1].direction = UP;
    board->ants[1].x = board->squares_x_len - 40;
    board->ants[1].y = board->squares_y_len - 40;

    board->ants[2].direction = UP;
    board->ants[2].x = 40;
    board->ants[2].y = board->squares_y_len - 40;

    board->ants[3].direction = UP;
    board->ants[3].x = board->squares_x_len - 40;
    board->ants[3].y = 40;

    board->ants[4].direction = UP;
    board->ants[4].x = 40;
    board->ants[4].y = 40;

    return board;
}

struct App {
    SDL_Renderer *renderer;
    SDL_Window *window;
    struct Board *board;
};

struct App *initSDL(void) {
    int rendererFlags, windowFlags;
    rendererFlags = SDL_RENDERER_ACCELERATED;
    windowFlags = 0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    struct App *app = malloc(sizeof(struct App));
    checkerr(app, "Failed to allocate app\n");
    app->window = SDL_CreateWindow("Langton's Ant", SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                                   SCREEN_HEIGHT, windowFlags);
    checkerr(app->window, "Failed to open %d x %d window: %s\n");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    app->renderer = SDL_CreateRenderer(app->window, -1, rendererFlags);
    checkerr(app->renderer, "Failed to create renderer: %s\n");

    app->board = board_init();
    return app;
}

void save_image(const struct Board *board, const char *filename) {
    size_t square_width = board->squares[0].rect.w;
    size_t square_height = board->squares[0].rect.h;
    unsigned char *image = malloc(board->squares_x_len * board->squares_y_len * square_width * square_height * 3);
    checkerr(image, "Failed to create image buffer");

    size_t image_index = 0;
    for (size_t y = 0; y < board->squares_y_len; y++) {
        for (size_t h = 0; h < square_height; h++) {
            for (size_t x = 0; x < board->squares_x_len; x++) {
                for (size_t w = 0; w < square_width; w++, image_index++) {
                    const uint8_t *color = COLORS[board->squares[y*board->squares_x_len+x].color];
                    image[image_index++] = color[0];
                    image[image_index++] = color[1];
                    image[image_index] = color[2];
                }
            }
        }
    }
    unsigned int error = lodepng_encode24_file(filename, image, board->squares_y_len * square_width, board->squares_x_len * square_height);
    if (error) {
        fputs(lodepng_error_text(error), stderr);
        exit(1);
    }
}

void doInput(const struct App *app, size_t it) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            char filename[N_OUTPUTFILENAME];
            if (!snprintf(filename, N_OUTPUTFILENAME, "ant_%zu.png", it)) {
                fputs("Cannot create filename", stderr);
                exit(1);
            }
            save_image(app->board, filename);
            exit(0);
            break;

        default:
            break;
        }
    }
}

void change_ant_direction(struct Ant *ant, int color) {
    ant->direction = (ant->direction + COLOR_ACTION[color]) % N_DIRECTIONS;
}

void move_ant(struct Ant *ant, ssize_t step, ssize_t x_max, ssize_t y_max) {
    switch (ant->direction) {
    case UP:
        ant->y -= step;
        if (ant->y < 0) {
            ant->y = (ant->y + y_max) % y_max;
        }
        break;

    case RIGHT:
        ant->x += step;
        if (ant->x >= x_max) {
            ant->x %= x_max;
        }
        break;

    case DOWN:
        ant->y += step;
        if (ant->y >= y_max) {
            ant->y %= y_max;
        }
        break;

    case LEFT:
        ant->x -= step;
        if (ant->x < 0) {
            ant->x = (ant->x + x_max) % x_max;
        }
        break;
    }
}

void change_square_color(struct Square *square) {
    square->color = (square->color + 1) % N_COLORS;
}

void update(struct App *app) {
    struct Board *board = app->board;
    for (size_t i = 0; i < board->ants_len; i++) {
        struct Ant *ant = &(board->ants[i]);
        struct Square *square = &(board->squares[(ant->y * board->squares_x_len) + ant->x]);
        change_ant_direction(ant, square->color);
        change_square_color(square);
        move_ant(ant, ANT_STEP, board->squares_x_len, board->squares_y_len);
    }
}

void draw(const struct App *app) {
    size_t i_max = app->board->squares_x_len * app->board->squares_y_len;
    for (size_t i = 0; i < i_max; i++) {
        const uint8_t *color = COLORS[app->board->squares[i].color];
        SDL_SetRenderDrawColor(app->renderer, color[0], color[1], color[2],
                               255);
        SDL_RenderFillRect(app->renderer, &(app->board->squares[i].rect));
    }
    SDL_RenderPresent(app->renderer);
}

int main(int argc, char **argv) {
    struct App *app = initSDL();

    for (size_t i = 0;; i++) {
        doInput(app, i);
        update(app);
        if (i % 10000 == 0) {
            draw(app);
            printf("%zu\n", i);
            SDL_Delay(10);
        }
    }

    return 0;
}