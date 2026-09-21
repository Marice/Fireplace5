#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

extern "C" {
#include "jar_xm.h"
}

#include "font8x8_basic.h"

/* 16:9 fire buffer, scaled 10x to 1920x1080 by the renderer. */
#define WIDTH 192
#define HEIGHT 108
#define FPS 60

/* PS5 DualSense joystick button numbers (ps5-payload-dev SDL2 port,
   standard SDL_GameController layout). */
#define JOY_BTN_A      0  /* Cross    */
#define JOY_BTN_B      1  /* Circle   */
#define JOY_BTN_X      2  /* Square   */
#define JOY_BTN_Y      3  /* Triangle */
#define JOY_BTN_SELECT 4  /* Touchpad (used as the help/select button) */
#define JOY_BTN_START  6  /* Options  */
#define JOY_BTN_L1     9
#define JOY_BTN_R1     10
#define JOY_BTN_DUP    11
#define JOY_BTN_DDOWN  12
#define JOY_BTN_DLEFT  13
#define JOY_BTN_DRIGHT 14
#define JOY_BTN_MENU   6  /* Options doubles as exit (see event loop) */

/* Cracktro fire-text scroller. */
static const char SCROLL_TEXT[] =
	"*** FIREPLACE5 *** RUNNING NATIVE ON THE PLAYSTATION 5 IN 2026 ... "
	"GREETINGS TO: RETRO HOMEBREW LOVERS ... "
	"OLIVIER <3 - ELISE <3 - CAROLIEN <3 ... "
	"MADE BY MARICE ... "
	"PRESS CROSS TO TOGGLE THIS SCROLLER ... ENJOY THE WARMTH! ...";
#define SCROLL_SPEED  1.6f  /* pixels per frame */
#define SCROLL_Y_BASE 40    /* top of glyphs, before sine offset */
#define SCROLL_AMP    11.0f /* sine amplitude in pixels */
#define SCROLL_FREQ   0.07f /* sine frequency along x */

#define NUM_PALETTES 5

/* Fire intensity: seed value written into the bottom rows. Index 2 is
   the original behaviour (22). Threshold must stay below the seed so
   "seed - fire[i]" cannot underflow. */
#define NUM_LEVELS 5
static const int fire_seed[NUM_LEVELS]      = { 10, 16, 22, 30, 38 };
static const int fire_threshold[NUM_LEVELS] = {  9, 15, 15, 15, 15 };

/* Rising embers. */
#define NUM_EMBERS 24
typedef struct {
	float x, y, vx, vy;
	int life;
} Ember;
static Ember embers[NUM_EMBERS];

/* Parallax starfield in the dark area above the flames. */
#define NUM_STARS 48
typedef struct {
	float x;
	int y;
	float speed;
	uint32_t color;
} Star;
static Star stars[NUM_STARS];

/* XM music, played through a plain SDL2 audio callback. */
static jar_xm_context_t* xm_ctx = NULL;

static void audio_callback(void* userdata, Uint8* stream, int len)
{
	(void)userdata;
	jar_xm_generate_samples(xm_ctx, (float*)stream, len / (2 * sizeof(float)));
}

static const uint32_t palette[256] = {
/* Jare's original FirePal. */
#define C(r,g,b) ((((r) * 4) << 16) | ((g) * 4 << 8) | ((b) * 4))
C( 0,   0,   0), C( 0,   1,   1), C( 0,   4,   5), C( 0,   7,   9),
C( 0,   8,  11), C( 0,   9,  12), C(15,   6,   8), C(25,   4,   4),
C(33,   3,   3), C(40,   2,   2), C(48,   2,   2), C(55,   1,   1),
C(63,   0,   0), C(63,   0,   0), C(63,   3,   0), C(63,   7,   0),
C(63,  10,   0), C(63,  13,   0), C(63,  16,   0), C(63,  20,   0),
C(63,  23,   0), C(63,  26,   0), C(63,  29,   0), C(63,  33,   0),
C(63,  36,   0), C(63,  39,   0), C(63,  39,   0), C(63,  40,   0),
C(63,  40,   0), C(63,  41,   0), C(63,  42,   0), C(63,  42,   0),
C(63,  43,   0), C(63,  44,   0), C(63,  44,   0), C(63,  45,   0),
C(63,  45,   0), C(63,  46,   0), C(63,  47,   0), C(63,  47,   0),
C(63,  48,   0), C(63,  49,   0), C(63,  49,   0), C(63,  50,   0),
C(63,  51,   0), C(63,  51,   0), C(63,  52,   0), C(63,  53,   0),
C(63,  53,   0), C(63,  54,   0), C(63,  55,   0), C(63,  55,   0),
C(63,  56,   0), C(63,  57,   0), C(63,  57,   0), C(63,  58,   0),
C(63,  58,   0), C(63,  59,   0), C(63,  60,   0), C(63,  60,   0),
C(63,  61,   0), C(63,  62,   0), C(63,  62,   0), C(63,  63,   0),
/* Followed by "white heat". */
#define W C(63,63,63)
W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W,
W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W,
W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W,
W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W,
W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W,
W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W,
W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W,
W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W
#undef W
#undef C
};

static uint8_t fire[WIDTH * HEIGHT];
static uint8_t prev_fire[WIDTH * HEIGHT];
static uint32_t framebuf[WIDTH * HEIGHT];

/* Palette variants derived from the classic one: classic, blue,
   green, purple and mono. Cycled with the d-pad. */
static uint32_t palettes[NUM_PALETTES][256];

static void init_stars(void)
{
	static const float layer_speed[3] = { 0.05f, 0.12f, 0.25f };
	static const uint32_t layer_color[3] = { 0x505050, 0x909090, 0xe0e0e0 };
	int i;
	srand(51137);
	for (i = 0; i < NUM_STARS; i++) {
			int layer = i % 3;
			stars[i].x = (float)(rand() % WIDTH);
			stars[i].y = 2 + rand() % 22;
			stars[i].speed = layer_speed[layer];
			stars[i].color = layer_color[layer];
	}
}

/* Stars drift slowly left and are occluded by the flames. */
static void update_draw_stars(void)
{
	int i;
	for (i = 0; i < NUM_STARS; i++) {
			stars[i].x -= stars[i].speed;
			if (stars[i].x < 0) stars[i].x += WIDTH;
			int x = (int)stars[i].x;
			int idx = (stars[i].y + 1) * WIDTH + x;
			if (fire[idx] < 4) {
					framebuf[stars[i].y * WIDTH + x] = stars[i].color;
			}
	}
}

static void respawn_ember(Ember* e)
{
	e->x = 4 + (float)(rand() % (WIDTH - 8));
	e->y = HEIGHT - 12 - (float)(rand() % 14);
	e->vx = ((rand() % 100) - 50) / 250.0f;
	e->vy = -0.2f - (rand() % 100) / 180.0f;
	e->life = 60 + rand() % 140;
}

/* Glowing sparks that rise from the flames; colored through the active
   palette so they match the chosen color scheme. */
static void update_draw_embers(int wind, int pal_idx)
{
	int i;
	for (i = 0; i < NUM_EMBERS; i++) {
			Ember* e = &embers[i];
			e->life--;
			e->x += e->vx + wind * 0.4f + ((rand() % 100) - 50) / 500.0f;
			e->y += e->vy;
			if (e->life <= 0 || e->y < 1 || e->x < 1 || e->x > WIDTH - 2) {
					respawn_ember(e);
					continue;
			}
			int ci = e->life / 3;
			if (ci > 63) ci = 63;
			if (ci < 8) ci = 8;
			framebuf[(int)e->y * WIDTH + (int)e->x] = palettes[pal_idx][ci];
	}
}

/* Small white text with a black outline at a fixed position (clock). */
static void draw_text_outlined(int tx, int ty, const char* s)
{
	int pass, col, row, dx, dy;
	for (pass = 0; pass < 2; pass++) {
			const char* p = s;
			int x0 = tx;
			for (; *p; p++, x0 += 8) {
					unsigned char c = (unsigned char)*p;
					if (c > 127 || c == ' ') continue;
					for (col = 0; col < 8; col++) {
							int x = x0 + col;
							if (x < 1 || x > WIDTH - 2) continue;
							for (row = 0; row < 8; row++) {
									if (!(font8x8_basic[c][row] & (1 << col))) continue;
									int y = ty + row;
									if (y < 1 || y > HEIGHT - 4) continue;
									if (pass == 0) {
											for (dy = -1; dy <= 1; dy++)
													for (dx = -1; dx <= 1; dx++)
															framebuf[(y + dy) * WIDTH + x + dx] = 0x000000;
									} else {
											framebuf[y * WIDTH + x] = 0xffffff;
									}
							}
					}
			}
	}
}

/* Help overlay: dim the frame and list all controls. */
static void draw_help(void)
{
	static const char* lines[] = {
			"CROSS: SCROLLER",
			"CIRCLE: EMBERS/CLOCK",
			"SQR/TRI: FIRE -/+",
			"DPAD: COLORS",
			"L1/R1: WIND",
			"R2: CRT FILTER",
			"OPTIONS: MUSIC",
			"TOUCHPAD: HELP",
			"L2+R2: EXIT",
	};
	int i, n = (int)(sizeof(lines) / sizeof(lines[0]));
	for (i = 2 * WIDTH; i < (HEIGHT - 4) * WIDTH; i++) {
			framebuf[i] = (framebuf[i] >> 2) & 0x3f3f3f;
	}
	for (i = 0; i < n; i++) {
			draw_text_outlined(4, 5 + i * 9, lines[i]);
	}
}

static void draw_clock(void)
{
	char buf[6];
	time_t t = time(NULL);
	struct tm* lt = localtime(&t);
	if (!lt) return;
	snprintf(buf, sizeof(buf), "%02d:%02d", lt->tm_hour, lt->tm_min);
	draw_text_outlined(WIDTH - 5 * 8 - 3, 3, buf);
}

static void build_palettes(void)
{
	int i;
	for (i = 0; i < 256; i++) {
			uint32_t r = (palette[i] >> 16) & 0xff;
			uint32_t g = (palette[i] >> 8) & 0xff;
			uint32_t b = palette[i] & 0xff;
			uint32_t lum = (2 * r + 5 * g + b) / 8;
			palettes[0][i] = palette[i];                  /* classic  */
			palettes[1][i] = (b << 16) | (g << 8) | r;    /* blue     */
			palettes[2][i] = (g << 16) | (r << 8) | b;    /* green    */
			palettes[3][i] = (r << 16) | (b << 8) | g;    /* purple   */
			palettes[4][i] = (lum << 16) | (lum << 8) | lum; /* mono  */
	}
}

/* Chrome fill for the scroller glyphs: one color per glyph row,
   dark blue -> white shine band -> dark blue. */
static const uint32_t chrome[8] = {
	0x102870, /* dark navy      */
	0x3060c0, /* mid blue       */
	0x80b0f0, /* light blue     */
	0xd8e8ff, /* near white     */
	0xffffff, /* white shine    */
	0x4878d0, /* mid blue       */
	0x2048a0, /* darker blue    */
	0x102870  /* dark navy      */
};

/* Draw the scroller glyphs over the rendered frame: chrome-gradient
   text with a 1px black outline, waving on a sine. Two passes so the
   outline of one glyph never covers the fill of its neighbour. */
static void draw_scroller(float scroll_x, int frame)
{
	int len = (int)strlen(SCROLL_TEXT);
	float phase = frame * 0.08f;
	int pass, ci, col, row, dx, dy;

	for (pass = 0; pass < 2; pass++) {
			for (ci = 0; ci < len; ci++) {
					unsigned char c = (unsigned char)SCROLL_TEXT[ci];
					if (c > 127 || c == ' ') continue;
					for (col = 0; col < 8; col++) {
							int x = (int)scroll_x + ci * 8 + col;
							if (x < 1 || x > WIDTH - 2) continue;
							int y0 = SCROLL_Y_BASE +
									 (int)(SCROLL_AMP * sinf(phase + x * SCROLL_FREQ));
							for (row = 0; row < 8; row++) {
									if (!(font8x8_basic[c][row] & (1 << col))) continue;
									int y = y0 + row;
									if (y < 1 || y > HEIGHT - 4) continue;
									if (pass == 0) {
											for (dy = -1; dy <= 1; dy++) {
													for (dx = -1; dx <= 1; dx++) {
															framebuf[(y + dy) * WIDTH + x + dx] = 0x000000;
													}
											}
									} else {
											framebuf[y * WIDTH + x] = chrome[row];
									}
							}
					}
			}
	}
}

int main()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0) {
		fprintf(stderr, "Failed SDL_Init: %s\n", SDL_GetError());
		return 1;
	}

	/* Music: load the XM module and stream it through an SDL audio
	   device. The app keeps running without music if this fails. */
	SDL_AudioDeviceID audio_dev = 0;
	const char* xm_path = "external.xm"; /* pak layout: next to the binary */
	FILE* probe = fopen(xm_path, "rb");
	if (!probe) {
			xm_path = "assets/external.xm"; /* repo layout for the native build */
	} else {
			fclose(probe);
	}
	jar_xm_create_context_from_file(&xm_ctx, 48000, xm_path);
	if (xm_ctx) {
			jar_xm_set_max_loop_count(xm_ctx, 0); /* loop forever */
			SDL_AudioSpec want, have;
			SDL_zero(want);
			want.freq = 48000;
			want.format = AUDIO_F32SYS;
			want.channels = 2;
			want.samples = 1024;
			want.callback = audio_callback;
			audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
			if (audio_dev) {
					SDL_PauseAudioDevice(audio_dev, 0);
			} else {
					fprintf(stderr, "No audio device: %s\n", SDL_GetError());
			}
	} else {
			fprintf(stderr, "Could not load external.xm, running silent\n");
	}

	int i;
	uint32_t sum;
	uint8_t avg;

	/* May be NULL when no joystick is present (e.g. desktop test build). */
	SDL_Joystick* joy = SDL_JoystickOpen(0);

	SDL_Window* window = SDL_CreateWindow("Fireplace5", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);
	if (!window) {
		fprintf(stderr, "Failed CreateWindow: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (!renderer) {
		fprintf(stderr, "Failed CreateRenderer: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	if (texture == NULL) {
		fprintf(stderr, "Failed CreateTexture: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	/* CRT scanlines are built at output resolution (one texture column,
	   stretched full-width): alternating transparent / dark rows. */
	int out_w = 0, out_h = 0;
	SDL_GetRendererOutputSize(renderer, &out_w, &out_h);
	if (out_h <= 0) out_h = 768;
	SDL_Texture* scanlines = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, out_h);
	if (scanlines) {
			uint32_t* rows = (uint32_t*)malloc(out_h * sizeof(uint32_t));
			if (rows) {
					for (i = 0; i < out_h; i++) {
							rows[i] = (i & 1) ? 0x46000000 : 0x00000000;
					}
					SDL_UpdateTexture(scanlines, NULL, rows, sizeof(uint32_t));
					free(rows);
					SDL_SetTextureBlendMode(scanlines, SDL_BLENDMODE_BLEND);
			}
	}

	build_palettes();
	init_stars();
	for (i = 0; i < NUM_EMBERS; i++) {
			respawn_ember(&embers[i]);
			embers[i].life = rand() % 200; /* desync initial spawn burst */
	}

	/* Set FIREPLACE_SCREENSHOT=<path.bmp> to save a screenshot after a
	   few seconds and exit (used to produce store screenshots). The
	   scroller is suppressed during capture. */
	const char* shot_path = SDL_getenv("FIREPLACE_SCREENSHOT");

	bool running = true;
	bool scroller_on = true;
	bool crt_on = false;
	bool help_on = false;
	bool r2_down = false;
	bool l2_down = false;
	bool music_paused = false;
	int ambience = 1;  /* 0 = off, 1 = embers, 2 = embers + clock */
	int level = 2;     /* fire intensity, 0..NUM_LEVELS-1 */
	int wind = 0;      /* -1 left, 0 none, +1 right (held) */
	int pal_idx = 0;
	int frame = 0;
	float scroll_x = WIDTH;
	float scroll_wrap = -8.0f * (float)strlen(SCROLL_TEXT);

	while (running)
	{
			SDL_Event ev;
			while (SDL_PollEvent(&ev)) {
					if (ev.type == SDL_QUIT) {
							running = false;
					} else if (ev.type == SDL_JOYBUTTONDOWN) {
							/* D-pad arrives as buttons on the DualSense. */
							switch (ev.jbutton.button) {
							case JOY_BTN_A:    scroller_on = !scroller_on; break;
							case JOY_BTN_B:    ambience = (ambience + 1) % 3; break;
							case JOY_BTN_Y:
									if (level < NUM_LEVELS - 1) level++;
									break;
							case JOY_BTN_X:
									if (level > 0) level--;
									break;
							case JOY_BTN_SELECT: help_on = !help_on; break;
							case JOY_BTN_START:
									if (audio_dev) {
											music_paused = !music_paused;
											SDL_PauseAudioDevice(audio_dev, music_paused);
									}
									break;
							case JOY_BTN_L1: wind = -1; break;
							case JOY_BTN_R1: wind = 1; break;
							case JOY_BTN_DRIGHT:
							case JOY_BTN_DUP:
									pal_idx = (pal_idx + 1) % NUM_PALETTES;
									break;
							case JOY_BTN_DLEFT:
							case JOY_BTN_DDOWN:
									pal_idx = (pal_idx + NUM_PALETTES - 1) % NUM_PALETTES;
									break;
							}
					} else if (ev.type == SDL_JOYBUTTONUP) {
							if ((ev.jbutton.button == JOY_BTN_L1 && wind == -1) ||
								(ev.jbutton.button == JOY_BTN_R1 && wind == 1)) {
									wind = 0;
							}
					} else if (ev.type == SDL_JOYAXISMOTION) {
							/* L2 (axis 4) and R2 (axis 5) are analog triggers.
							   R2 toggles the CRT filter; holding both L2+R2
							   quits back to the launcher. */
							if (ev.jaxis.axis == 5) {
									if (!r2_down && ev.jaxis.value > 20000) {
											r2_down = true;
											crt_on = !crt_on;
									} else if (r2_down && ev.jaxis.value < 10000) {
											r2_down = false;
									}
							} else if (ev.jaxis.axis == 4) {
									l2_down = ev.jaxis.value > 20000;
							}
							if (l2_down && r2_down) running = false;
					} else if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
							/* Keyboard fallback for desktop test builds. */
							switch (ev.key.keysym.sym) {
							case SDLK_ESCAPE: running = false; break;
							case SDLK_SPACE:  scroller_on = !scroller_on; break;
							case SDLK_b:      ambience = (ambience + 1) % 3; break;
							case SDLK_EQUALS:
									if (level < NUM_LEVELS - 1) level++;
									break;
							case SDLK_MINUS:
									if (level > 0) level--;
									break;
							case SDLK_c:      crt_on = !crt_on; break;
							case SDLK_h:      help_on = !help_on; break;
							case SDLK_p:
									if (audio_dev) {
											music_paused = !music_paused;
											SDL_PauseAudioDevice(audio_dev, music_paused);
									}
									break;
							case SDLK_q:      wind = -1; break;
							case SDLK_e:      wind = 1; break;
							case SDLK_RIGHT:
							case SDLK_UP:
									pal_idx = (pal_idx + 1) % NUM_PALETTES;
									break;
							case SDLK_LEFT:
							case SDLK_DOWN:
									pal_idx = (pal_idx + NUM_PALETTES - 1) % NUM_PALETTES;
									break;
							}
					} else if (ev.type == SDL_KEYUP) {
							if ((ev.key.keysym.sym == SDLK_q && wind == -1) ||
								(ev.key.keysym.sym == SDLK_e && wind == 1)) {
									wind = 0;
							}
					}
			}

			for (int y = 1; y < HEIGHT - 1; y++) {
					for (int x = 1; x < WIDTH - 1; x++) {
							i = y * WIDTH + x;

							/* Wind skews where rising heat is sampled from,
							   making the flames lean; disabled at the edges. */
							int w = wind;
							if (x - 1 - w < 0 || x + 1 - w > WIDTH - 1) w = 0;

							/* Average the eight neighbours. */
							sum = prev_fire[i - WIDTH - 1] +
								  prev_fire[i - WIDTH    ] +
								  prev_fire[i - WIDTH + 1] +
								  prev_fire[i - 1] +
								  prev_fire[i + 1] +
								  prev_fire[i + WIDTH - 1 - w] +
								  prev_fire[i + WIDTH     - w] +
								  prev_fire[i + WIDTH + 1 - w];
							avg = (uint8_t)(sum / 8);

							/* "Cool" the pixel if the two bottom bits of the
							   sum are clear (somewhat random). For the bottom
							   rows, cooling can overflow, causing "sparks". */
							if (!(sum & 3) &&
								(avg > 0 || i >= (HEIGHT - 4) * WIDTH)) {
									avg--;
							}
							fire[i] = avg;
					}
			}

			/* Copy back and scroll up one row.
			   The bottom row is all zeros, so it can be skipped. */
			for (i = 0; i < (HEIGHT - 2) * WIDTH; i++) {
					prev_fire[i] = fire[i + WIDTH];
			}

			/* Seed the bottom rows: replace dark pixels by fuel. The seed
			   value scales with the selected fire intensity level. */
			for (i = (HEIGHT - 7) * WIDTH; i < (HEIGHT - 1) * WIDTH; i++) {
					if (fire[i] < fire_threshold[level]) {
							fire[i] = fire_seed[level] - fire[i];
					}
			}

			/* Copy to framebuffer and map to RGBA, scrolling up one row. */
			for (i = 0; i < (HEIGHT - 2) * WIDTH; i++) {
					framebuf[i] = palettes[pal_idx][fire[i + WIDTH]];
			}

			update_draw_stars();

			if (ambience >= 1) {
					update_draw_embers(wind, pal_idx);
			}

			/* The scroller is hidden while capturing a store screenshot. */
			if (scroller_on && !shot_path) {
					draw_scroller(scroll_x, frame);
					scroll_x -= SCROLL_SPEED;
					if (scroll_x < scroll_wrap) {
							scroll_x = WIDTH;
					}
			}

			if (ambience == 2) {
					draw_clock();
			}

			if (help_on) {
					draw_help();
			}
			frame++;

			/* Update the texture and render it. */
			SDL_UpdateTexture(texture, NULL, framebuf, WIDTH * sizeof(framebuf[0]));
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, texture, NULL, NULL);
			if (crt_on && scanlines) {
					SDL_RenderCopy(renderer, scanlines, NULL, NULL);
			}

			if (shot_path && frame == 250) {
					SDL_Surface* shot = SDL_CreateRGBSurfaceWithFormat(0, out_w, out_h, 32, SDL_PIXELFORMAT_ARGB8888);
					if (shot &&
						SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, shot->pixels, shot->pitch) == 0) {
							SDL_SaveBMP(shot, shot_path);
					}
					if (shot) SDL_FreeSurface(shot);
					running = false;
			}

			SDL_RenderPresent(renderer);

			SDL_Delay(1000 / FPS);
	}

	if (audio_dev) {
			SDL_CloseAudioDevice(audio_dev); /* stops the callback first */
	}
	if (xm_ctx) {
			jar_xm_free_context(xm_ctx);
	}

	if (scanlines) {
			SDL_DestroyTexture(scanlines);
	}
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	if (joy) {
			SDL_JoystickClose(joy);
	}

	SDL_Quit();

	return 0;
}
