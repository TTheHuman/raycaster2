#define SDL_MAIN_HANDLED
#include <windows.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_hints.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_scancode.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

float pitch;

#define ASSERT(cond, ...)                                                      \
  if (!cond) {                                                                 \
    fprintf(stderr, __VA_ARGS__);                                              \
    exit(1);                                                                   \
  }

#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define MAP_SIZE 16
const uint8_t MAP[MAP_SIZE * MAP_SIZE] = {
	1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 3, 3, 3, 0, 0, 0, 0, 0, 0, 2, 0, 1,
	1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 2, 0, 1, 
	1, 0, 0, 0, 0, 0, 3, 2, 2, 2, 2, 2, 2, 2, 0, 1,
	1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 2, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 2, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 2, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

#define PI 3.14159265f
const float maxDepth = 20.0f;

typedef struct v2_f{ float x, y; } v2;
typedef struct v2_i{ int x, y; } v2i;

static inline v2 rotate(v2 v, float a){
    return(v2){
        (v.x * cos(a)) - (v.y * sin(a)),
        (v.x * sin(a)) + (v.y * cos(a)),
    };
}

static inline v2 segintersection(v2 a0, v2 a1, v2 b0, v2 b1){
    const float d = ((a0.x - a1.x) * (b0.y - b1.y)) - ((a0.y - a1.y) * (b0.x - b1.x));

    if (fabsf(d) < 0.0000001f){ return(v2){ NAN, NAN }; }

    const float t = ((a0.x - a1.x) * (b0.y - b1.y)) - ((a0.y - a1.y) * (b0.x - b1.x)) / d, u = ((a0.x - a1.x) * (b0.y - b1.y)) - ((a0.y - a1.y) * (b0.x - b1.x)) / d;

        return (t >= 0 && t <= 1 && u >= 0 && u <= 1) ?
        ((v2) {
            a0.x + (t * (a1.x - a0.x)),
            a0.y + (t * (a1.y - a0.y)) })
        : ((v2) { NAN, NAN });
}

struct sectors{
    int id;
    int index;
    int wallc;
    int floor;
    int celing;
};

struct walls{
    v2 x0, y0;
    v2 x1, y1;
    int portalsec;
};

typedef enum {NorthSouth, EastWest} Side;
typedef struct {
    float x, y;
} Vec2F;
typedef struct {
    int x, y;
} Vec2I;
typedef struct {
	SDL_Window* window;
	SDL_Renderer* renderer;
	bool quit;
} State;
typedef struct {
	Vec2F pos;
	Vec2F dir;
    Vec2F plane;
} Player;
typedef struct{
    uint8_t r,g,b,a;
} ColorRGBA;
ColorRGBA RGBA_Red   = {.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF};
ColorRGBA RGBA_Green = {.r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF};
ColorRGBA RGBA_Blue  = {.r = 0x00, .g = 0x00, .b = 0xFF, .a = 0xFF}; 
ColorRGBA RGBA_Yellow = {.r = 0xFF, .g = 0xFF, .b = 0x00, .a = 0xFF};

double SECTORS[] = { 1, 0, 5, 0.0, 5.0 };
int WALLS1[] = { 4, 4, 10, 4, 0 };
int WALLS2[] = { 10, 4, 10, 10, 0 };
int WALLS3[] = { 10, 10, 7, 14, 2 };
int WALLS4[] = { 7, 14, 3, 10, 0 };
int WALLS5[] = { 3, 10, 4, 4, 0 };

int xy2index(int x, int y, int w) {
    return y * w + x;
}

void map(){
    struct sectors s1;
    struct walls w1;

    s1.id = SECTORS[0];
    s1.index = SECTORS[1];
    s1.wallc = SECTORS[2];
    s1.floor = SECTORS[3];
    s1.celing = SECTORS[4];

    w1.portalsec = WALLS1[4];
}

void render(State *state, Player* player) {
    for (int x = 0; x < SCREEN_WIDTH; ++x) {
        // calculate ray position and direction
        float cameraX = 2 * x / (float)SCREEN_WIDTH - 1; // x-coordinate in camera space
        Vec2F rayDir = {
            .x = player->dir.x + player->plane.x * cameraX,
            .y = player->dir.y + player->plane.y * cameraX,
        };

        // wich box of the map we're in 
        Vec2I mapBox = {
            .x = (int)player->pos.x, 
            .y = (int)player->pos.y
        };
        // Length of ray from current position to next x- or y-side
        Vec2F sideDist = {};
        // Lenth of ray from one x- or y-side to next x- or y-side 
        Vec2F deltaDist = {
            .x = (rayDir.x == 0) ? 1e30 : fabsf(1 / rayDir.x),
            .y = (rayDir.y == 0) ? 1e30 : fabsf(1 / rayDir.y),
        };
        float perpWallDist;
        // What direction to step in x- or y-direction (either +1 or -1)
        Vec2I stepDir = {};

        bool hit = false; // was there a wall hit
        Side side; // was a NorthSouth or EastWest wall hit

        // calculate stepDir and initial sideDist
        if (rayDir.x < 0) {
            stepDir.x = -1;
            sideDist.x = (player->pos.x - mapBox.x) * deltaDist.x;
        } else {
            stepDir.x = 1;
            sideDist.x = (mapBox.x + 1.0f - player->pos.x) * deltaDist.x;
        }
        if (rayDir.y < 0) {
            stepDir.y = -1;
            sideDist.y = (player->pos.y - mapBox.y) * deltaDist.y;
        } else {
            stepDir.y = 1;
            sideDist.y = (mapBox.y + 1.0f - player->pos.y) * deltaDist.y;
        }

        // Perform DDA
        while (!hit) {
            // jump to next map square
            if (sideDist.x < sideDist.y) {
                sideDist.x += deltaDist.x;
                mapBox.x += stepDir.x;
                side = EastWest;
            } else {
                sideDist.y += deltaDist.y;
                mapBox.y += stepDir.y;
                side = NorthSouth;
            }
            // check if ray has hit a wall
            if (MAP[xy2index(mapBox.x, mapBox.y, MAP_SIZE)] > 0) {
                hit = true;
            }
        }

        // Calculate the distance projceted on camera direction
        // (Euclidian distance would give fisheye effect)
        switch (side) {
            case EastWest:
                perpWallDist = (sideDist.x - deltaDist.x);
                break;
            case NorthSouth:
                perpWallDist = (sideDist.y - deltaDist.y);
                break;
        }

        // Calculate height of line to draw on screen 
        int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);
        // calculate lowest and highest pixel to fill in current stripe
        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2 - pitch;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2 - pitch; 
        if(MAP[xy2index(mapBox.x, mapBox.y, MAP_SIZE)] > 3){}
        if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT;

        const uint8_t* keycode = SDL_GetKeyboardState(NULL);
        // this is the dumbest and most inneficent way to do this
        // anyone who comes across this code feel free to just fucking kill me
        // TODO - MAKE THIS GOOD
        if(keycode[SDL_SCANCODE_SPACE]) {
                drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2 - pitch + 50;
                drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2 - pitch + 50;
            }
        

        // choose wall color
        ColorRGBA color;
        switch (MAP[xy2index(mapBox.x, mapBox.y, MAP_SIZE)]) {
            case 1: color = RGBA_Red; break;
            case 2: color = RGBA_Green; break;
            case 3: color = RGBA_Blue; break;
            case 4: color = RGBA_Yellow; break;
        }

        // give x and y sides different brightness
        if (side == NorthSouth) {
            color.r /= 2; 
            color.g /= 2; 
            color.b /= 2; 
        }
        // Draw
        SDL_SetRenderDrawColor(state->renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(state->renderer, x, drawStart, x, drawEnd);
	}
}

int main(void) {

	ASSERT(!SDL_Init(SDL_INIT_VIDEO),
		   "SDL failed to initialize; %s\n",
		   SDL_GetError());
	State state = {
        .quit = false,
    };
	state.window =
		SDL_CreateWindow("Raycast",
						 SDL_WINDOWPOS_CENTERED_DISPLAY(0),
						 SDL_WINDOWPOS_CENTERED_DISPLAY(0),
						 SCREEN_WIDTH,
						 SCREEN_HEIGHT,
						 SDL_WINDOW_ALLOW_HIGHDPI);
	ASSERT(state.window,
		   "failed to create SDL window: %s\n",
		   SDL_GetError());

	state.renderer =
		SDL_CreateRenderer(state.window,
						   -1,
						   SDL_RENDERER_PRESENTVSYNC);
	ASSERT(state.renderer,
		   "failed to create SDL renderer: %s\n",
		   SDL_GetError());

    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1");
    SDL_SetRelativeMouseMode((SDL_bool)true);

	Player player = {
        .pos = {.x =  4.0f, .y =  4.0f},
        .dir = {.x = -1.0f, .y =  0.0f},
        .plane = {.x = 0.0f, .y = 0.66f},
    };

	const float rotateSpeed = 0.025;
	float moveSpeed = 0.05;
    int pitchSpeed = 0.025;
	
	while (!state.quit) {
		SDL_Event event;
        int mouse_xrel = 0;
        int mouse_yrel = 0;
        int mouse_yrel2 = 0;
        int yrelup;
        int yreldown;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
		        case SDL_QUIT:
		        	state.quit = true;
		        	break;
                case SDL_MOUSEMOTION:
                    mouse_xrel = event.motion.xrel;
                    mouse_yrel = event.motion.yrel;
                    mouse_yrel2 = SDL_MOUSEMOTION;
                    break;
            }
		}

    int ticks_before = SDL_GetTicks();
    int ticks_after = SDL_GetTicks();
    int ticks_passed = ticks_after - ticks_before;
    int amount_to_wait = 24 - ticks_passed;
    if(amount_to_wait > 0) {
        SDL_Delay(amount_to_wait);
    }

        const uint8_t* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_ESCAPE]) state.quit = true;
        if (mouse_xrel != 0) { // rotate x
            float rotSpeed = rotateSpeed * (mouse_xrel * -0.1);
            // both camera direction and camera plane must be rotated
            Vec2F oldDir = player.dir;
            player.dir.x = player.dir.x * cosf(rotSpeed) - player.dir.y * sinf(rotSpeed);
            player.dir.y = oldDir.x     * sinf(rotSpeed) + player.dir.y * cosf(rotSpeed);

            Vec2F oldPlane = player.plane;
            player.plane.x = player.plane.x * cosf(rotSpeed) - player.plane.y * sinf(rotSpeed);
            player.plane.y = oldPlane.x     * sinf(rotSpeed) + player.plane.y * cosf(rotSpeed);
        }

        if (mouse_yrel != 0) { // rotate y
            if(mouse_yrel < 0){ 
                pitch = pitch - 15;
                pitch--;
            }
            if(mouse_yrel > 0){
                pitch = pitch + 15;
                pitch++;
        }
        }

        Vec2F deltaPos = {
            .x = player.dir.x * moveSpeed,
            .y = player.dir.y * moveSpeed,
        };

        if (keystate[SDL_SCANCODE_LSHIFT] && keystate[SDL_SCANCODE_W]) { // forwards run
            if (MAP[xy2index(
                        player.pos.x + deltaPos.x, 
                        player.pos.y, 
                        MAP_SIZE)] == 0) {
                player.pos.x += deltaPos.x;
            }
            if (MAP[xy2index(
                        player.pos.x, 
                        player.pos.y + deltaPos.y, 
                        MAP_SIZE)] == 0) {
                player.pos.y += deltaPos.y;
            }
        }

        if (keystate[SDL_SCANCODE_W]) { // forwards
            if (MAP[xy2index(
                        player.pos.x + deltaPos.x, 
                        player.pos.y, 
                        MAP_SIZE)] == 0) {
                player.pos.x += deltaPos.x;
            }
            if (MAP[xy2index(
                        player.pos.x, 
                        player.pos.y + deltaPos.y, 
                        MAP_SIZE)] == 0) {
                player.pos.y += deltaPos.y;
            }
        }

        if (keystate[SDL_SCANCODE_LSHIFT] && keystate[SDL_SCANCODE_S]) { // forwards run
            if (MAP[xy2index(
                        player.pos.x - deltaPos.x, 
                        player.pos.y, 
                        MAP_SIZE)] == 0) {
                player.pos.x -= deltaPos.x;
            }
            if (MAP[xy2index(
                        player.pos.x, 
                        player.pos.y - deltaPos.y, 
                        MAP_SIZE)] == 0) {
                player.pos.y -= deltaPos.y;
            }

        }

        if (keystate[SDL_SCANCODE_S]) { // backwards
            if (MAP[xy2index(
                        player.pos.x - deltaPos.x, 
                        player.pos.y, 
                        MAP_SIZE)] == 0) {
                player.pos.x -= deltaPos.x;
            }
            if (MAP[xy2index(
                        player.pos.x, 
                        player.pos.y - deltaPos.y, 
                        MAP_SIZE)] == 0) {
                player.pos.y -= deltaPos.y;
            }
        }
        if (keystate[SDL_SCANCODE_A]) { // strafe left
            if (MAP[xy2index(
                        player.pos.x - deltaPos.y, 
                        player.pos.y, 
                        MAP_SIZE)] == 0) {
                player.pos.x -= deltaPos.y;
            }
            if (MAP[xy2index(
                        player.pos.x, 
                        player.pos.y - -deltaPos.x, 
                        MAP_SIZE)] == 0) {
                player.pos.y -= -deltaPos.x;
            }
        }
        if (keystate[SDL_SCANCODE_D]) { // strafe right
            if (MAP[xy2index(
                        player.pos.x - -deltaPos.y, 
                        player.pos.y, 
                        MAP_SIZE)] == 0) {
                player.pos.x -= -deltaPos.y;
            }
            if (MAP[xy2index(
                        player.pos.x, 
                        player.pos.y - deltaPos.x, 
                        MAP_SIZE)] == 0) {
                player.pos.y -= deltaPos.x;
            }
        }

        SDL_SetRenderDrawColor(state.renderer, 0x18, 0x18, 0x18, 0xFF);
        SDL_RenderClear(state.renderer);

		render(&state, &player);

		SDL_RenderPresent(state.renderer);
	}

	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	return 0;
}