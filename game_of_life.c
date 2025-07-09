#define _XOPEN_SOURCE_EXTENDED 1
#include <ncurses.h>
#include <wchar.h>
#include <stdbool.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

void update_game_win(WINDOW *win, bool *grid, int width, int height, int player_x, int player_y);

int main(int argc, char **argv) {
	setlocale(LC_ALL, ""); // UTF-8
	initscr();
	curs_set(0);
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	WINDOW *game_win;
	int win_height = LINES - 4;
	int win_width = COLS - 2;
	game_win = newwin(win_height, win_width, (LINES - win_height) / 2, (COLS - win_width) / 2);
	refresh();
	box(game_win, 0, 0);

	int grid_width = win_width - 2;
	int grid_height = (win_height - 2) * 2;
	mvprintw(0, 0, "board width: %d\nboard height: %d", grid_width, grid_height);

	// flat matrix
	bool grid[grid_width * grid_height];
	bool grid_to_update[grid_width * grid_height];

	// initialise grids
	for (int y = 0; y < grid_height; y++) {
		for (int x = 0; x < grid_width; x++) {
			grid[y * grid_width + x] = false;
			grid_to_update[y * grid_width + x] = false;
		}
	}

	// two blocks in the corners, just for fun
	grid[0 * grid_width + 0] = true;
	grid[0 * grid_width + 1] = true;
	grid[1 * grid_width + 0] = true;
	grid[1 * grid_width + 1] = true;

	grid[(grid_height - 1) * grid_width + (grid_width - 1)] = true;
	grid[(grid_height - 2) * grid_width + (grid_width - 1)] = true;
	grid[(grid_height - 1) * grid_width + (grid_width - 2)] = true;
	grid[(grid_height - 2) * grid_width + (grid_width - 2)] = true;

	int player_x = grid_width / 2, player_y = grid_height / 2;
	update_game_win(game_win, grid, grid_width, grid_height, player_x, player_y);

	while (1) {
		move(LINES -1, 0);
		clrtoeol();
		mvprintw(LINES - 1, 0, "-- INSERT -- <ENTER>: run | <SPACE>: toggle cell | ↑ ↓ → ←: movement | <r>: randomise cells | <q>: quit");
		bool quit = false;
		switch (getch()) {
			case 'r':
				for (int i = 0; i < 1234; i++) {
					int rand_x = rand() % grid_width;
					int rand_y = rand() % grid_height;
					grid[rand_y * grid_width + rand_x] = true;
				}
				break;
			case KEY_UP:
				if (player_y > 0) player_y--;
				break;
			case KEY_DOWN:
				if (player_y < grid_height - 1) player_y++;
				break;
			case KEY_RIGHT:
				if (player_x < grid_width - 1) player_x++;
				break;
			case KEY_LEFT:
				if (player_x > 0) player_x--;
				break;
			case ' ':
				grid[player_y * grid_width + player_x] = !grid[player_y * grid_width + player_x];
				break;
			case 10:
				move(LINES -1, 0);
				clrtoeol();
				mvprintw(LINES - 1, 0, "-- RUNNING -- <ENTER>: step | <q>: stop)");
				while (getch() != 'q') {
					for (int y = 0; y < grid_height; y++) {
						for (int x = 0; x < grid_width; x++) {
							int alive_neigh = 0;
							for (int dy = -1; dy <= 1; dy++) {
								for (int dx = -1; dx <= 1; dx++) {
									if (dx == 0 && dy == 0) continue;
									int nx = x + dx;
									int ny = y + dy;
									if (nx >= 0 && nx < grid_width && ny >= 0 && ny < grid_height)
										if (grid[ny * grid_width + nx])
											alive_neigh++;
								}
							}
							bool current = grid[y * grid_width + x];
							if (current && (alive_neigh == 2 || alive_neigh == 3))
								grid_to_update[y * grid_width + x] = true;
							else if (!current && alive_neigh == 3)
								grid_to_update[y * grid_width + x] = true;
							else
								grid_to_update[y * grid_width + x] = false;
						}
					}
					memcpy(grid, grid_to_update, sizeof(bool) * grid_width * grid_height);
					update_game_win(game_win, grid, grid_width, grid_height, -100, -100);
				}
				break;
			case 'q':
				quit = true;
				break;
		}
		if (quit) break;
		move(LINES - 1, 0);
		clrtoeol();
		update_game_win(game_win, grid, grid_width, grid_height, player_x, player_y);
	}

	refresh();
	endwin();
	return 0;
}

void update_game_win(WINDOW *win, bool *grid, int width, int height, int player_x, int player_y) {
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y += 2) {
			cchar_t pixel_t;
			bool upper = grid[y * width + x];
			bool lower = (y + 1 < height) ? grid[(y + 1) * width + x] : false;

			if (upper && lower)
				pixel_t.chars[0] = 0x2588;
			else if (upper)
				pixel_t.chars[0] = 0x2580;
			else if (lower)
				pixel_t.chars[0] = 0x2584;
			else
				pixel_t.chars[0] = ' ';

			pixel_t.chars[1] = 0;
			pixel_t.attr = A_NORMAL;
			mvwadd_wch(win, (y + 2) / 2, x + 1, &pixel_t);
		}
	}
	cchar_t player_t;
	if (player_y % 2 != 0 && grid[(player_y + 1) * width + player_x])
		player_t.chars[0] = 0x2584;
	else if (player_y % 2 != 0 && grid[(player_y - 1) * width + player_x])
		player_t.chars[0] = 0x2588;
	else if (player_y % 2 == 0 && grid[(player_y + 1) * width + player_x])
		player_t.chars[0] = 0x2588;
	else
		player_t.chars[0] = player_y % 2 == 0 ? 0x2580 : 0x2584;
	player_t.attr = A_BLINK;
	mvwadd_wch(win, (player_y + 2) / 2, player_x + 1, &player_t);

	wrefresh(win);
}

