#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <menu.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/reboot.h>
#include <sys/reboot.h>

#include "menu_strings.h"
#include "text_strings.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define MIN(a, b) ((a < b) ? a : b)

#define MENU_MARGIN_LEFT 4
#define MENU_MARGIN_RIGHT 4
#define MENU_MARGIN_TOP 9
#define MENU_MARGIN_BOTTOM 2

#define UNREACHABLE assert(false);

#define PRINT_SLEEP_TIME 35000
#define PRINT_CHUNK_SIZE 5


#define KEY_ENTERX 10


// global variables
struct winsize w;

MENU* main_menu;
MENU* missile_menu;
MENU* log_menu;

ITEM** main_menu_items;
ITEM** missile_menu_items;
ITEM** log_menu_items;


MENU* init_menus(WINDOW* menu_window) {
  int i, count;

  inline void set_menu(MENU* menu, WINDOW* menu_window) {
    set_menu_fore(menu, COLOR_PAIR(1) | A_REVERSE | WA_BOLD);
    set_menu_back(menu, COLOR_PAIR(1) | WA_BOLD);
    set_menu_grey(menu, COLOR_PAIR(2) | WA_BOLD);

    set_menu_win(menu, menu_window);
    set_menu_format(menu, w.ws_row - (MENU_MARGIN_TOP + MENU_MARGIN_BOTTOM), 1);
    set_menu_sub(menu, derwin(menu_window, 0, 0, 0, 0));
  }

  // main menu strings
  count = ARRAY_SIZE(main_menu_strings);
  main_menu_items = (ITEM**) calloc (count, sizeof(ITEM*));
  for (i = 0; i < count; ++i)
    main_menu_items[i] = new_item(main_menu_strings[i], NULL);

  main_menu = new_menu((ITEM**)main_menu_items);

  // missile menu strings
  count = ARRAY_SIZE(missile_menu_strings);
  missile_menu_items = (ITEM**) calloc (count, sizeof(ITEM*));
  for (i = 0; i < count; ++i)
    missile_menu_items[i] = new_item(missile_menu_strings[i], missile_menu_desc[i]);

  missile_menu = new_menu((ITEM**)missile_menu_items);
  menu_opts_on(missile_menu, O_SHOWDESC);

  // log menu strings
  count = ARRAY_SIZE(log_menu_strings);
  log_menu_items = (ITEM**) calloc (count, sizeof(ITEM*));
  for (i = 0; i < count; ++i)
    log_menu_items[i] = new_item(log_menu_strings[i], NULL);

  log_menu = new_menu((ITEM**)log_menu_items);

  set_menu(main_menu, menu_window);
  set_menu(missile_menu, menu_window);
  set_menu(log_menu, menu_window);

  return main_menu;
}

void free_menus() {
  int i;

  free_menu(main_menu);
  for (i = 0; i < ARRAY_SIZE(main_menu_strings); i++)
    free_item(main_menu_items[i]);
}

int kbhit(void) {
    if (getch() != ERR)
      return 1;
    return 0;
}

void print_text(WINDOW* text_window, const char* text) {
  int count;

  nodelay(stdscr, TRUE);

  for (; strlen(text); text += count) {
    count = MIN(PRINT_CHUNK_SIZE, strlen(text));
    wprintw(text_window, "%*.*s", count, count, text);
    usleep(PRINT_SLEEP_TIME);
    wrefresh(text_window);

    if (kbhit()) {
      wprintw(text_window, "%s\n", text);
      wrefresh(text_window);
      break;
    }
  }
  nodelay(stdscr, FALSE);
}

void text_window(const char** text, int count) {
  WINDOW* text_window;
  text_window = newwin(w.ws_row - (MENU_MARGIN_TOP + MENU_MARGIN_BOTTOM),
                       w.ws_col - (MENU_MARGIN_LEFT + MENU_MARGIN_RIGHT),
                       MENU_MARGIN_TOP,
                       MENU_MARGIN_LEFT);

  wbkgd(text_window, COLOR_PAIR(1) | WA_BOLD);
  scrollok(text_window, TRUE);

  for (int i = 0; i < count; i++) {
    print_text(text_window, text[i]);
    getch();
    wclear(text_window);
  }

  delwin(text_window);
}

void popup_window(char* message) {
  int window_width = MIN(100, w.ws_col - 20);
  WINDOW* popup_window = newwin(5, window_width, (w.ws_row - 7) / 2, (w.ws_col - window_width) / 2);

  box(popup_window, 0, 0);
  wbkgd(popup_window, COLOR_PAIR(1) | WA_BOLD);

  mvwprintw(popup_window, 5/2, (window_width - strlen(message))/2, "%s", message);
  wrefresh(popup_window);

  getch();
  delwin(popup_window);
}

void shutdown_pc() {
  sync();
  reboot(LINUX_REBOOT_CMD_POWER_OFF);
}

int main(int argc, char* argv[]) {
  int c;
  MENU *current_menu;
  WINDOW *menu_window;

  initscr();
  start_color();

  init_pair(1, COLOR_GREEN, COLOR_BLACK);
  init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);

  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  // Set header text
  wbkgd(stdscr, COLOR_PAIR(1));
  attron(A_BOLD);
  mvprintw(2, MENU_MARGIN_LEFT, "Welcome to VAULT B42 Main Termlink Computer");
  mvprintw(3, MENU_MARGIN_LEFT, "Clearance: Overseer Eyes Only");
  mvprintw(5, MENU_MARGIN_LEFT, "CONFIDENTIAL CONFIDENTIAL CONFIDENTIAL");
  mvprintw(6, MENU_MARGIN_LEFT, "OVERSEER EYES ONLY | VIOLATION VTP-0100110");
  attroff(A_BOLD);

  // Get size of current terminal window
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  // create main menu window
  menu_window = newwin(w.ws_row - (MENU_MARGIN_TOP + MENU_MARGIN_BOTTOM),
                       w.ws_col - (MENU_MARGIN_LEFT + MENU_MARGIN_RIGHT),
                       MENU_MARGIN_TOP,
                       MENU_MARGIN_LEFT);
  keypad(menu_window, TRUE);

  current_menu = init_menus(menu_window);

  refresh();

  /* Post the menu */
  post_menu(current_menu);
  wrefresh(menu_window);

  int choice;

  while ((c = wgetch(menu_window)) != KEY_F(1)) {
    switch (c) {
      case KEY_DOWN:
        menu_driver(current_menu, REQ_DOWN_ITEM);
        break;
      case KEY_UP:
        menu_driver(current_menu, REQ_UP_ITEM);
        break;
      case KEY_ENTERX:
        unpost_menu(current_menu);
        choice = item_index(current_item(current_menu));

        if (current_menu == main_menu) {
          switch (choice) {
            case 0:  // [VAULT B42 INSTRUCTIONS] TODO
              text_window(test_text, 2);
              break;
            case 1:  // [Nuclear warheads operation manual] TODO
              text_window(test_text, 2);
              break;
            case 2:  // [Nuclear missiles status]
              current_menu = missile_menu;
              break;
            case 3:  // [Main Base Communication Channel]
              popup_window("Communication channel DISCONNECTED!");
              break;
            case 4:  // [Evacuation Instructions] TODO
              text_window(test_text, 2);
              break;
            case 5:  // [Overseer's Log]
              current_menu = log_menu;
              break;
            case 6:  // [Exit Terminal]
              /* shutdown_pc(); */
              free_menus();
              endwin();
              exit(EXIT_SUCCESS);
              break;
            default:
              UNREACHABLE
          }
          post_menu(current_menu);

        } else if (current_menu == missile_menu) {
          if (choice >= 0 && choice < 6) {
            popup_window("Missile Launch bay is empty!");
          } else if (choice == 6) {
            current_menu = main_menu;
          } else {
            UNREACHABLE
          }

        } else if (current_menu == log_menu) {
          switch (choice) {
            case 0:  // TODO
              text_window(test_text, 2);
              break;
            case 1:  // TODO
              text_window(test_text, 2);
              break;
            case 2:  // [Back]
              current_menu = main_menu;
              break;
            default:
              UNREACHABLE
          }
        } else {
          UNREACHABLE
        }

        post_menu(current_menu);
        break;
      default:
        break;
    }
    wrefresh(menu_window);
  }
  UNREACHABLE
}
