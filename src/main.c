#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#define NUM_NEIGHBOR	(8)
#define WEIGHT		(50000)

typedef struct _cell {
	uint8_t life:1;
	uint8_t prev_life:1;
	uint8_t prev_prev_life:1;
	uint8_t reserved:5;
	struct _cell *neighbor[NUM_NEIGHBOR];
} cell_t;

static void
map_neighbors(cell_t *c, cell_t *origin, size_t xsize, size_t ysize)
{
	size_t x = (c - origin) % xsize;
	size_t y = (c - origin) / xsize;
	size_t prev_x = (x + xsize - 1) % xsize;
	size_t prev_y = (y + ysize - 1) % ysize;
	size_t next_x = (x + 1) % xsize;
	size_t next_y = (y + 1) % ysize;

#define CELL(base, x, y)	((base) + (x) + (y) * xsize)

	c->neighbor[0] = CELL(origin, prev_x, prev_y);
	c->neighbor[1] = CELL(origin, x, prev_y);
	c->neighbor[2] = CELL(origin, next_x, prev_y);
	c->neighbor[3] = CELL(origin, prev_x, y);
	c->neighbor[4] = CELL(origin, next_x, y);
	c->neighbor[5] = CELL(origin, prev_x, next_y);
	c->neighbor[6] = CELL(origin, x, next_y);
	c->neighbor[7] = CELL(origin, next_x, next_y);
}

static void
init_field(cell_t *field, size_t xsize, size_t ysize)
{
	memset(field, 0, sizeof(cell_t) * xsize * ysize);

	for (size_t y = 0; y < ysize; y++) {
		for (size_t x = 0; x < xsize; x++) {
			map_neighbors(&field[xsize * y + x], &field[0], xsize,
				      ysize);
		}
	}
}

static void
prepare(cell_t *cp, size_t xsize, size_t ysize)
{
	for (size_t y = 0; y < ysize; y++) {
		for (size_t x = 0; x < xsize; x++) {
			cell_t *entry = &cp[x + y * xsize];
			entry->prev_prev_life = entry->prev_life;
			entry->prev_life = entry->life;
		}
	}
}

static uint32_t
update_life(cell_t *cp)
{
	uint32_t sum = 0;

	for (size_t i = 0; i < NUM_NEIGHBOR; i++) {
		sum += cp->neighbor[i]->prev_life;
	}

	if (sum == 3) {
		cp->life = 1;
	}
	else if (sum == 2 && cp->life == 1) {
		cp->life = 1;
	}
	else {
		cp->life = 0;
	}

	return cp->life;
}

static uint32_t
update(cell_t *field, size_t xsize, size_t ysize)
{
	uint32_t sum = 0;

	for (size_t y = 0; y < ysize; y++) {
		for (size_t x = 0; x < xsize; x++) {
			sum += update_life(&field[xsize * y + x]);
		}
	}

	return sum;
}

static uint32_t
play(cell_t *field, size_t xsize, size_t ysize)
{
	prepare(field, xsize, ysize);
	return update(field, xsize, ysize);
}

static void
dump(int i, cell_t *field, size_t xsize, size_t ysize)
{
	printf("%d\n", i);
	for (size_t y = 0; y < ysize; y++) {
		for (size_t x = 0; x < xsize; x++) {
			printf("%c", field[xsize * y + x].life > 0 ? '#' : '.');
		}
		printf("\n");
	}
	printf("\n");
}

static void
check_cell(const cell_t *cp, const cell_t *origin, size_t xsize)
{
	for (size_t n = 0; n < 8; n++) {
		size_t d = cp->neighbor[n] - origin;
		printf("[%u] %u,%u ", n, d % xsize, d / xsize);
	}
}

static void
check_field(const cell_t *field, size_t xsize, size_t ysize)
{
	for (size_t y = 0; y < ysize; y++) {
		for (size_t x = 0; x < xsize; x++) {
			printf("%u %u\n", x, y);
			check_cell(&field[y * xsize + x], field, xsize);
			printf("\n");
		}
	}
	printf("\n");
}

static int
check_state(const cell_t *field, size_t xsize, size_t ysize)
{
	for (size_t y = 0; y < ysize; y++) {
		for (size_t x = 0; x < xsize; x++) {
			const cell_t *cp = &field[y * xsize + x];
			if (cp->life != cp->prev_prev_life) {
				return 1;
			}
		}
	}
	return 0;
}

#define SET(x, y)	set(game, x, y, xsize)
static inline void
set(cell_t *base, size_t x, size_t y, size_t xsize)
{
	base[x + xsize * y].life = 1;
}

typedef struct _pos {
	int x;
	int y;
} pos_t;

static const pos_t acorn[] = {
	{0, 1}, {3, 1}, {0, 2}, {1, 2}, {4, 2}, {5, 2}, {6, 2}
};

static const pos_t die_hard[] = {
	{7, 0}, {1 , 1}, {2, 1}, {2, 2}, {6, 2}, {7, 2}, {8, 0}
};

static const pos_t rpentmino[] = {
	{0, 1}, {0, 2}, {1, 0}, {1, 1}, {2, 1}
};

static const pos_t canon[] = {
	{25, 0}, {25, 1}, {23, 1}, {25, 5}, {25, 6}, {23, 5},
	{21, 2}, {22, 2}, {21, 3}, {22, 3}, {21, 4}, {22, 4},
	{35, 2}, {36, 2}, {35, 3}, {36, 3},
	{13, 2}, {14, 2}, {12, 3}, {16, 3}, {11, 4}, {17, 4},
	{11, 5}, {15, 5}, {17, 5}, {18, 5}, {11, 6}, {17, 6},
	{12, 7}, {16, 7}, {13, 8}, {14, 8},
	{1, 4}, {2, 4}, {1, 5}, {2, 5}
};

int
main(int argc, char *argv[])
{
	int i = 0;
	cell_t *game;
	int xsize = 1 << 13;
	int ysize = 1 << 13;

	game = malloc(sizeof(cell_t) * xsize * ysize);
	if (game == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	init_field(game, xsize, ysize);
	//check_field(game, xsize, ysize);

	for (int i = 0; i < 5; i++) {
		SET(die_hard[i].x, die_hard[i].y);
	}

	struct timeval time1, time2;
	gettimeofday(&time1, NULL);

	do {
		//dump(i, game, xsize, ysize);
		(void)play(game, xsize, ysize);
		//usleep(WEIGHT);
		//i++;
	} while (check_state(game, xsize, ysize));

	gettimeofday(&time2, NULL);

	if (time2.tv_usec > time1.tv_usec) {
		time2.tv_sec -= time1.tv_sec;
		time2.tv_usec -= time1.tv_usec;
	}
	else {
		time2.tv_sec = time2.tv_sec - time1.tv_sec - 1;
		time2.tv_usec = 1000000L - time1.tv_usec + time2.tv_usec;
	}

	printf("time: %ld sec %ld usec\n", time2.tv_sec, time2.tv_usec);
	return EXIT_SUCCESS;
}

