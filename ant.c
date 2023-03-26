#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>

#define SCREEN_WIDTH   1000
#define SCREEN_HEIGHT  1000

#define N_DIRECTIONS 4
enum Direction {up, right, down, left};

#define N_COLORS 4
const int colors[N_COLORS][3] = {
	{255, 255, 255},
	{  0, 255,   0},
	{  0,   0, 255},
	{255,   0,   0},
};
const int color_action[N_COLORS] = {
	1,
	-1,
	-1,
	1,
};

void checkerr(void *p, char *message) {
	if (p == NULL) {
		fputs(message, stderr);
		exit(1);
	}
}

#define SDLRECT_WIDTH 10 /* Must be a divisor of SCREEN_WIDTH */
#define SDLRECT_HEIGHT 10 /* Must be a divisor of SCREEN_HEIGHT */
struct Square {
	int color;
	SDL_Rect rect;
};

const int SQUARES_X_LEN = (SCREEN_WIDTH / SDLRECT_WIDTH);
const int SQUARES_Y_LEN = (SCREEN_HEIGHT / SDLRECT_HEIGHT);
struct Square *squares_init(int squares_x_len, int squares_y_len) {

	struct Square *squares = malloc(sizeof(struct Square) * SQUARES_X_LEN * SQUARES_Y_LEN);
	checkerr(squares, "Cannot allocate squares");

	int index = 0;
	for (int x = 0; x < squares_x_len; x++) {
		for (int y = 0; y < squares_y_len; y++, index++) {
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
	int x;
	int y;
};

struct Board {
	struct Ant *ants;
	int ants_len;
	struct Square *squares; /* 2D array in an 1D array */
	int squares_x_len;
	int squares_y_len;
};

struct Board *board_init() {
	struct Board *board = malloc(sizeof(struct Board));
	checkerr(board, "Cannot allocate board");

	board->squares_x_len = SQUARES_X_LEN;
	board->squares_y_len = SQUARES_Y_LEN;
	board->squares = squares_init(board->squares_x_len, board->squares_y_len);

	board->ants_len = 3;
	board->ants = malloc(sizeof(struct Ant) * board->ants_len);
	checkerr(board->ants, "Cannot allocate ants");
	board->ants[0].direction = up;
	board->ants[0].x = board->squares_x_len/2;
	board->ants[0].y = board->squares_y_len/2;

	board->ants[1].direction = up;
	board->ants[1].x = board->squares_x_len/2 + 1;
	board->ants[1].y = board->squares_y_len/2 + 1;

	board->ants[2].direction = up;
	board->ants[2].x = board->squares_x_len/2 - 1;
	board->ants[2].y = board->squares_y_len/2 - 1;

	return board;
}

struct App {
    SDL_Renderer *renderer;
    SDL_Window *window;
	struct Board *board;
};

struct App *initSDL(void)
{
	int rendererFlags, windowFlags;

	rendererFlags = SDL_RENDERER_ACCELERATED;

	windowFlags = 0;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

    struct App *app = malloc(sizeof(struct App));
	checkerr(app, "Failed to allocate app\n");
	app->window = SDL_CreateWindow("Langton's Ant", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);
	checkerr(app->window, "Failed to open %d x %d window: %s\n");

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

	app->renderer = SDL_CreateRenderer(app->window, -1, rendererFlags);
	checkerr(app->renderer, "Failed to create renderer: %s\n");

	app->board = board_init();
    return app;
}

void doInput()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				exit(0);
				break;

			default:
				break;
		}
	}
}

void change_ant_direction(struct Ant *ant, int color) {
	ant->direction = (ant->direction + color_action[color]) % N_DIRECTIONS;
}

void move_ant(struct Ant *ant, int step, int x_max, int y_max) {
	switch (ant->direction) {
	case up:
		ant->x -= step;
		if (ant->x < 0) {
			ant->x = (ant->x + x_max) % x_max;
		}
		break;

	case right:
		ant->y += step;
		if (ant->y >= y_max) {
			ant->y %= y_max;
		}
		break;

	case down:
		ant->x += step;
		if (ant->x >= x_max) {
			ant->x %= x_max;
		}
		break;

	case left:
		ant->y -= step;
		if (ant->y < 0) {
			ant->y = (ant->y + y_max) % y_max;
		}
		break;
	}
}

void change_square_color(struct Square *square) {
	square->color = (square->color + 1) % N_COLORS;
}

void update(struct Board *board) {
	for (int i = 0; i < board->ants_len; i++) {
		struct Ant *ant = &(board->ants[i]);
		struct Square *square = &(board->squares[(ant->x * board->squares_y_len) + ant->y]);
		change_ant_direction(ant, square->color);
		change_square_color(square);
		move_ant(ant, ANT_STEP, board->squares_x_len, board->squares_y_len);
	}
}

void draw(struct App *app) {
	int i_max = app->board->squares_x_len * app->board->squares_y_len;
	for (int i = 0; i < i_max; i++) {
		const int *color = colors[app->board->squares[i].color];
    	SDL_SetRenderDrawColor(app->renderer, color[0], color[1], color[2], 255);
		SDL_RenderFillRect(app->renderer, &(app->board->squares[i].rect));
	}
    SDL_RenderPresent(app->renderer);
}

int main(int argc, char **argv) {
    struct App *app = initSDL();

    for (int i = 0;; i++) {
        doInput();
		update(app->board);
		draw(app);
		printf("%d\n", i);
    }

    return 0;
}