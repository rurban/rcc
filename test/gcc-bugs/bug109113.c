/* GCC Bug #109113 - internal compiler error: in output_constructor_regular_field, at varasm.cc:5521
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109113
 */
/* { dg-do compile } */


#include <stdlib.h>

typedef struct menu_item_s {
    void *fn;
} menu_item_t;

typedef struct menu_list_s {
    int _a;
    menu_item_t items[];
} menu_list_t;

typedef struct menu_s {
    int _a;
    menu_list_t items[];
} menu_t;

typedef struct region_s {
    void *data;
} region_t;

static void fn(void) {
//     // Bug does not trigger when doing something inside this function
//     //int a = 0;
}

static const menu_list_t FILE_MENU = {
    .items = {
        { fn },
    }
};

const menu_t WINDOW_MENU = {
    .items = {
//         FILE_MENU,
//         FILE_MENU,
    },
};

void create_menu_region(region_t *dest, const menu_t *menu)
{
//     // Bug only happens when putting output in the region_t struct
//     dest->data = malloc(sizeof(menu));
//     // Does not trigger bug:
//     //menu_t *data = malloc(sizeof(menu));
//     //  void *data = malloc(sizeof(menu));
//     //  malloc(sizeof(menu));
}

// Just here so that ld doesn't complain
int main(void) {}


