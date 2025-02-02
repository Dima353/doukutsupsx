#include <string.h>

#include "engine/common.h"
#include "engine/graphics.h"
#include "engine/memory.h"
#include "engine/sound.h"
#include "engine/input.h"
#include "engine/timer.h"
#include "engine/org.h"
#include "engine/mcrd.h"

#include "game/player.h"
#include "game/stage.h"
#include "game/game.h"
#include "game/hud.h"
#include "game/tsc.h"
#include "game/profile.h"
#include "game/menu.h"

#define MAIN_TOOLTIP_OFSY 40

static int menu_id = MENU_NONE;

// triangle, circle, cross, square
static gfx_texrect_t rc_padbuttons[4];

static gfx_texrect_t rc_pause = {{ 0, 132, 126, 144 }};

static gfx_texrect_t rc_menubox[3] = {
  {{ 0, 0,  244, 8  }},
  {{ 0, 8,  244, 16 }},
  {{ 0, 16, 244, 24 }},
};

static gfx_texrect_t rc_cursor_arms[2] = {
  {{ 0,  88, 40, 128 }},
  {{ 40, 88, 80, 128 }},
};
static gfx_texrect_t rc_cursor_items[2] = {
  {{ 80, 88,  112, 104 }},
  {{ 80, 104, 112, 120 }},
};
static gfx_texrect_t rc_cursor_menu[4] = {
  {{ 0,  16, 16, 32 }},
  {{ 16, 16, 32, 32 }},
  {{ 0,  16, 16, 32 }},
  {{ 32, 16, 48, 32 }},
};

static gfx_texrect_t rc_title[2] = {
  {{ 80, 48, 144, 56 }},
  {{ 80, 56, 144, 64 }},
};

static gfx_texrect_t rc_stage[8];

static gfx_texrect_t rc_stagesel_title = {{ 80, 64, 144, 72 }};
static gfx_texrect_t rc_main_title = {{ 0, 0, 144, 40 }};

static gfx_texrect_t rc_heart = {{ 32, 80, 48, 96 }};

static gfx_texrect_t rc_island_sky = {{ 0, 0, 160, 80 }};
static gfx_texrect_t rc_island_ground = {{ 0, 48, 160, 80 }};
static gfx_texrect_t rc_island_sprite = {{ 0, 0, 40, 24 }};

// common variables used by simple multiple-choice menus
static int main_sel = 0;
static int main_count = 0;
static int main_tip_y = MAIN_TOOLTIP_OFSY;
static u32 main_tick = 0;
static const char **main_choices;
static const u8 main_bg_rgb[] = { 32, 32, 32 };
static const u8 main_title_rgb[] = { 243, 226, 152 };
static const u8 main_tip_rgb[] = { 176, 176, 176 };
static const u8 main_black_rgb[] = { 0, 0, 0 };
static const u8 main_barbg_rgb[] = { 16, 16, 16 };

void menu_init(void) {
  menu_id = MENU_NONE;

  for (int i = 0; i < 4; ++i) {
    rc_padbuttons[i].r.left = 196 + i * 12;
    rc_padbuttons[i].r.top = 132;
    rc_padbuttons[i].r.right = rc_padbuttons[i].r.left + 12;
    rc_padbuttons[i].r.bottom = rc_padbuttons[i].r.top + 12;
    gfx_set_texrect(&rc_padbuttons[i], SURFACE_ID_TEXT_BOX);
  }

  for (int i = 0; i < 3; ++i)
    gfx_set_texrect(&rc_menubox[i], SURFACE_ID_TEXT_BOX);

  for (int i = 0; i < 2; ++i) {
    gfx_set_texrect(&rc_cursor_arms[i], SURFACE_ID_TEXT_BOX);
    gfx_set_texrect(&rc_cursor_items[i], SURFACE_ID_TEXT_BOX);
    gfx_set_texrect(&rc_title[i], SURFACE_ID_TEXT_BOX);
  }

  for (int i = 0; i < 4; ++i)
    gfx_set_texrect(&rc_cursor_menu[i], SURFACE_ID_MY_CHAR);

  for (int i = 0; i < 8; ++i) {
    rc_stage[i].r.left =  i * 32;
    rc_stage[i].r.top = 0;
    rc_stage[i].r.right = rc_stage[i].r.left + 32;
    rc_stage[i].r.bottom = rc_stage[i].r.top + 16;
    gfx_set_texrect(&rc_stage[i], SURFACE_ID_STAGE_ITEM);
  }

  gfx_set_texrect(&rc_pause, SURFACE_ID_TEXT_BOX);
  gfx_set_texrect(&rc_stagesel_title, SURFACE_ID_TEXT_BOX);
  gfx_set_texrect(&rc_heart, SURFACE_ID_NPC_SYM);
}

static inline int menu_padbutton_index(const u32 input) {
  switch (input) {
    case PAD_CROSS:    return 2;
    case PAD_CIRCLE:   return 1;
    case PAD_SQUARE:   return 3;
    case PAD_TRIANGLE:
    default:
      return 0;
  }
}

static inline void draw_string_shadow(const char *str, const u8 *rgb, const int x, const int y) {
  // draw shadow
  gfx_draw_string_rgb(str, main_black_rgb, GFX_LAYER_FRONT, x + 1, y + 1);
  // draw the string
  if (rgb)
    gfx_draw_string_rgb(str, rgb, GFX_LAYER_FRONT, x, y);
  else
    gfx_draw_string(str, GFX_LAYER_FRONT, x, y);
}

static inline void draw_string_centered(const char *str, const u8 *rgb, const int x, const int y) {
  const int w = strlen(str) * GFX_FONT_WIDTH;
  draw_string_shadow(str, rgb, x - w / 2, y);
}

static inline u32 menu_generic_control(void) {
  if (input_trig & IN_UP) {
    snd_play_sound(PRIO_HIGH, 1, FALSE);
    if (--main_sel < 0)
      main_sel = main_count - 1;
  } else if (input_trig & IN_DOWN) {
    snd_play_sound(PRIO_HIGH, 1, FALSE);
    if (++main_sel >= main_count)
      main_sel = 0;
  }
  if (input_trig & IN_OK)
    return IN_OK;
  if (input_trig & IN_CANCEL)
    return IN_CANCEL;
  return 0;
}

static inline void menu_generic_draw(int x, int y) {
  int yofs = 8;
  gfx_draw_texrect_16x16(&rc_cursor_menu[++main_tick / 10 % 4], GFX_LAYER_FRONT, x - 24, y + 5 + main_sel * 20);
  for (int i = 0; i < main_count; ++i, yofs += 20)
    draw_string_shadow(main_choices[i], NULL, x, y + yofs);
}

static inline void menu_draw_tooltip(void) {
  const int btn_cancel = menu_padbutton_index(input_binds[11]);
  const int btn_ok = menu_padbutton_index(input_binds[10]);
  const int y = VID_HEIGHT - GFX_FONT_HEIGHT - 16 + main_tip_y;
  if (main_tip_y) main_tip_y -= 2;
  gfx_draw_texrect(&rc_padbuttons[btn_cancel], GFX_LAYER_FRONT, VID_WIDTH / 2 - 10 * GFX_FONT_WIDTH - 2, y);
  draw_string_shadow("OTMEHA", main_tip_rgb, VID_WIDTH / 2 - 8 * GFX_FONT_WIDTH, y);
  gfx_draw_texrect(&rc_padbuttons[btn_ok], GFX_LAYER_FRONT, VID_WIDTH / 2 + 2 * GFX_FONT_WIDTH + 2, y);
  draw_string_shadow("BraOP", main_tip_rgb, VID_WIDTH / 2 + 4 * GFX_FONT_WIDTH + 4, y);
}

/* default func */

static void menu_null(void) {
  // nada
}

/* main menu */

#define MAIN_LEFT ((VID_WIDTH / 2) - 72)
#define MAIN_TOP 40

static void menu_title_open(void) {
  static const char *submenu_text[3] = { "HOBAv gbPA", "fAbPkfgTs", "HACTPOhKg" };
  main_count = 3;
  main_sel = 0;
  main_choices = submenu_text;
  main_tip_y = MAIN_TOOLTIP_OFSY;
  // the title surface is only loaded in one specific map, so we can't do this in init
  gfx_set_texrect(&rc_main_title, SURFACE_ID_TITLE);
  // set title music (should be loaded with the "u" stagebank)
  stage_change_music(0x18);
}

static void menu_title_act(void) {
  static const int submenus[] = {
    -1, MENU_LOAD, MENU_OPTIONS,
  };
  const u32 btn = menu_generic_control();
  if (btn == IN_OK) {
    snd_play_sound(PRIO_HIGH, 18, FALSE);
    menu_id = 0;
    if (submenus[main_sel] == -1) {
      game_reset(TRUE);
      game_start_new();
    } else {
      menu_open(submenus[main_sel]);
    }
  }
}

static void menu_title_draw(void) {
  // clear the screen
  gfx_draw_clear(main_bg_rgb, GFX_LAYER_FRONT);
  // draw the menu
  gfx_draw_texrect(&rc_main_title, GFX_LAYER_FRONT, MAIN_LEFT, MAIN_TOP);
  menu_generic_draw(MAIN_LEFT + 56, MAIN_TOP + 80);
  // draw counter if it exists
  if (game_stopwatch)
    hud_draw_time(16, 16);
}

/* pause */

static const char *str_pause[] = {
  "jPOcOiegTs", "HACTPOhKg", "BrXOc",
};

static void menu_pause_open(void) {
  main_sel = 0;
  main_count = 3;
  main_choices = str_pause;
  main_tip_y = MAIN_TOOLTIP_OFSY;
}

static void menu_pause_act(void) {
  const u32 btn = menu_generic_control();
  if (btn == IN_OK) {
    snd_play_sound(PRIO_HIGH, 18, FALSE);
    menu_id = MENU_NONE;
    switch (main_sel) {
      case 1: // Options
        menu_open(MENU_OPTIONS);
        break;
      case 2: // Quit
        game_reset(TRUE);
        game_start_intro();
        break;
    }
  } else if (input_trig & (IN_CANCEL | IN_PAUSE)) {
    menu_id = MENU_NONE;
  } else if (input_trig & IN_DEBUG) {
    plr_debug_cheat();
  }
}

static void menu_pause_draw(void) {
  // clear screen
  gfx_draw_clear(gfx_clear_rgb, GFX_LAYER_FRONT);
  menu_generic_draw(MAIN_LEFT + 56, (VID_HEIGHT - 20 * 3) / 2);
  menu_draw_tooltip();
}

/* inventory */

#define INV_BOX_LEFT TEXT_BOX_LEFT
#define INV_BOX_TOP  ((VID_HEIGHT / 2) - 108)
#define INV_CUR_LEFT (INV_BOX_LEFT + 10)
#define INV_CUR_TOP  (INV_BOX_TOP + 16)
#define INV_ITEM_TOP (INV_CUR_TOP + 52)

static struct {
  s32 arm_idx;
  s32 item_idx;
  s32 title_y;
  u32 flash;
  bool in_items;
} inventory;

static void menu_inventory_open(void) {
  tsc_switch_script(TSC_SCRIPT_ARMS_ITEM);

  inventory.in_items = FALSE;
  inventory.title_y = INV_BOX_TOP + 16;
  inventory.arm_idx = player.arm;
  inventory.item_idx = 0;

  if (player.num_arms)
    tsc_start_event(1000 + player.arms[inventory.arm_idx].id);
  else
    tsc_start_event(5000 + player.items[inventory.item_idx]);
}

static inline void menu_inventory_controls(void) {
  if (!player.num_items && !player.num_arms)
    return; // empty

  bool changed = FALSE;

  if (inventory.in_items) {
    // in items menu

    if (input_trig & IN_LEFT) {
      if (inventory.item_idx % 6)
        --inventory.item_idx;
      else
        inventory.item_idx += 5; // loop around to end of row
      changed = TRUE;
    } else if (input_trig & IN_RIGHT) {
      if (inventory.item_idx == player.num_items - 1)
        inventory.item_idx = (inventory.item_idx / 6) * 6; // loop around to end of row
      else if (inventory.item_idx % 6 == 5)
        inventory.item_idx -= 5; // loop around to end of row
      else
        ++inventory.item_idx;
      changed = TRUE;
    }

    if (input_trig & IN_UP) {
      if (inventory.item_idx < 6)
        inventory.in_items = FALSE; // cursor was on row 1, swap to arms menu
      else
        inventory.item_idx -= 6; // move one row up
      changed = TRUE;
    } else if (input_trig & IN_DOWN) {
      if (inventory.item_idx / 6 == (player.num_items - 1) / 6)
        inventory.in_items = FALSE; // cursor was on last row, swap to arms menu
      else
        inventory.item_idx += 6; // move one row down
      changed = TRUE;
    }

    // keep index in range
    if (inventory.item_idx >= player.num_items)
      inventory.item_idx = player.num_items - 1;

    // select item if OK is pressed and we're still in items
    if ((inventory.in_items) && (input_trig & IN_OK))
      tsc_start_event(6000 + player.items[inventory.item_idx]);
  } else {
    // in arms menu

    if (input_trig & IN_LEFT) {
      --inventory.arm_idx;
      changed = TRUE;
    } else if (input_trig & IN_RIGHT) {
      ++inventory.arm_idx;
      changed = TRUE;
    }

    if (input_trig & (IN_UP | IN_DOWN)) {
      // if there are items, change to items menu
      if (player.num_items)
        inventory.in_items = TRUE;
      changed = TRUE;
    }

    // keep index in range
    if (inventory.arm_idx < 0)
      inventory.arm_idx = player.num_arms - 1;
    if (inventory.arm_idx >= player.num_arms)
      inventory.arm_idx = 0;
  }

  if (changed) {
    if (inventory.in_items) {
      // change item and display new item's name
      snd_play_sound(PRIO_HIGH, 1, FALSE);
      tsc_start_event(5000 + player.items[inventory.item_idx]);
    } else {
      // change weapon and display new weapon's name
      snd_play_sound(PRIO_HIGH, 4, FALSE);
      player.arm = inventory.arm_idx;
      player.arms_x = 32;
      tsc_start_event(1000 + player.arms[inventory.arm_idx].id);
    }
  }
}

static void menu_inventory_act(void) {
  if (game_flags & GFLAG_INPUT_ENABLED)
    menu_inventory_controls();

  // check for exit buttons
  if (inventory.in_items) {
    if ((game_flags & GFLAG_INPUT_ENABLED) && (input_trig & (IN_INVENTORY | IN_CANCEL | IN_PAUSE)))
      menu_close();
  } else {
    if (input_trig & (IN_INVENTORY | IN_CANCEL | IN_PAUSE | IN_OK))
      menu_close();
  }
}

static void menu_inventory_draw(void) {
  int i;

  // draw box
  gfx_draw_texrect(&rc_menubox[0], GFX_LAYER_FRONT, INV_BOX_LEFT, INV_BOX_TOP);
  for (i = 1; i < 18; ++i)
    gfx_draw_texrect(&rc_menubox[1], GFX_LAYER_FRONT, INV_BOX_LEFT, INV_BOX_TOP + (i * 8));
  gfx_draw_texrect(&rc_menubox[2], GFX_LAYER_FRONT, INV_BOX_LEFT, INV_BOX_TOP + (i * 8));

  // update title offset
  if (inventory.title_y > INV_BOX_TOP + 8)
    --inventory.title_y;
  // draw titles
  gfx_draw_texrect(&rc_title[0], GFX_LAYER_FRONT, INV_CUR_LEFT, inventory.title_y);
  gfx_draw_texrect(&rc_title[1], GFX_LAYER_FRONT, INV_CUR_LEFT, inventory.title_y + 52);

  ++inventory.flash;

  // draw arms cursor
  const int flash_idx = ((inventory.flash / 2) % 2);
  int cur_idx = (inventory.in_items) ? 1 : flash_idx;
  gfx_draw_texrect(&rc_cursor_arms[cur_idx], GFX_LAYER_FRONT, (inventory.arm_idx * 40) + INV_CUR_LEFT, INV_CUR_TOP);

  // draw arms
  for (i = 0; i < player.num_arms; ++i) {
    // draw icon, slash and Lv
    gfx_draw_texrect(&hud_rc_arms[player.arms[i].id], GFX_LAYER_FRONT, (i * 40) + INV_CUR_LEFT, INV_CUR_TOP);
    gfx_draw_texrect(&hud_rc_ammo[0], GFX_LAYER_FRONT, (i * 40) + INV_CUR_LEFT, INV_CUR_TOP + 32);
    gfx_draw_texrect(&hud_rc_ammo[1], GFX_LAYER_FRONT, (i * 40) + INV_CUR_LEFT, INV_CUR_TOP + 16);
    hud_draw_number(player.arms[i].level, (i * 40) + INV_CUR_LEFT + 24, INV_CUR_TOP + 16);

    // draw ammo (or "--" if infinite)
    if (player.arms[i].max_ammo) {
      hud_draw_number(player.arms[i].ammo, (i * 40) + INV_CUR_LEFT + 24, INV_CUR_TOP + 24);
      hud_draw_number(player.arms[i].max_ammo, (i * 40) + INV_CUR_LEFT + 24, INV_CUR_TOP + 32);
    } else {
      gfx_draw_texrect(&hud_rc_ammo[2], GFX_LAYER_FRONT, (i * 40) + INV_CUR_LEFT + 16, INV_CUR_TOP + 24); // "--"
      gfx_draw_texrect(&hud_rc_ammo[2], GFX_LAYER_FRONT, (i * 40) + INV_CUR_LEFT + 16, INV_CUR_TOP + 32); // "--"
    }
  }

  // draw items cursor
  cur_idx = (inventory.in_items) ? flash_idx : 1;
  int icol = (inventory.item_idx % 6);
  int irow = (inventory.item_idx / 6);
  gfx_draw_texrect(&rc_cursor_items[cur_idx], GFX_LAYER_FRONT, icol * 32 + INV_CUR_LEFT, irow * 16 + INV_ITEM_TOP);

  // draw items
  for (i = 0; i < player.num_items; ++i) {
    const int item_id = player.items[i];
    icol = (i % 6);
    irow = (i / 6);
    gfx_draw_texrect(&hud_rc_item[item_id], GFX_LAYER_FRONT, icol * 32 + INV_CUR_LEFT, irow * 16 + INV_ITEM_TOP);
  }
}

/* minimap */

#define MAP_XSTEP 40
#define MAP_YSTEP 30

static struct {
  gfx_texrect_t texrect;
  u32 mode;
  s32 count;
  u32 title_w;
  s32 player_x;
  s32 player_y;
  u32 wait;
  s32 scale;
} map;

static const u16 map_clut[] = {
  GFX_RGB(0x00, 0x19, 0x00),
  GFX_RGB(0x00, 0x4E, 0x00),
  GFX_RGB(0x00, 0x98, 0x00),
  GFX_RGB(0x00, 0xFF, 0x00),
};

static const int map_scaletab[] = { 4, 3, 2, 2, 1, 1, 1, 1 };

static inline u8 map_get_color(const u8 atrb) {
  // fuck
  switch (atrb) {
    case 0x00:
      return 0;
    case 0x01: case 0x02: case 0x40: case 0x44: case 0x51: case 0x52: case 0x55: case 0x56:
    case 0x60: case 0x71: case 0x72: case 0x75: case 0x76: case 0x80: case 0x81: case 0x82:
    case 0x83: case 0xA0: case 0xA1: case 0xA2: case 0xA3:
      return 1;
    case 0x43: case 0x50: case 0x53: case 0x54: case 0x57: case 0x63: case 0x70: case 0x73:
    case 0x74: case 0x77:
      return 2;
    default:
      return 3;
  }
}

static void menu_map_open(void) {
  map.mode = 0;
  map.count = 0;
  map.title_w = strlen(stage_data->title) * GFX_FONT_WIDTH;
  map.player_x = (TO_INT(player.x) + TILE_SIZE / 2) / TILE_SIZE;
  map.player_y = (TO_INT(player.y) + TILE_SIZE / 2) / TILE_SIZE;

  const int xscale = stage_data->width / MAP_XSTEP;
  const int yscale = stage_data->height / MAP_YSTEP;
  map.scale = map_scaletab[((xscale > yscale) ? xscale : yscale)];

  // unfortunately rendering the map in real time every frame is SLOW
  // even if you render line by line instead of pixel by pixel
  // as such, we just pre-render the motherfucker
  // TODO: maybe pre-render on map load

  // pad with 1 word border to prevent quad cutoff
  // massive alignment required for some reason, or else LoadImage shits itself
  const int aw = ALIGN(stage_data->width + 2, 32);
  const int ah = stage_data->height + 1;
  u8 *data = mem_zeroalloc(aw * (ah + 1));
  u8 *ptr;
  for (u32 ty = 0; ty < stage_data->height; ++ty) {
    ptr = data + ty * aw;
    for (u32 tx = 0; tx < stage_data->width; ++tx, ++ptr)
      *ptr = map_get_color(stage_get_atrb(tx, ty));
  }
  // append CLUT
  memcpy(data + aw * ah, map_clut, sizeof(map_clut));
  // upload for later
  gfx_surf[SURFACE_ID_MAP].tex_x = 0; // let gfx_upload image determine the coords
  gfx_surf[SURFACE_ID_MAP].tex_y = 0;
  gfx_upload_image(data, aw, ah, 1, SURFACE_ID_MAP, TRUE);
  mem_free(data);

  map.texrect.r.left = 0;
  map.texrect.r.top = 0;
  map.texrect.r.right = stage_data->width;
  map.texrect.r.bottom = stage_data->height;
  gfx_set_texrect(&map.texrect, SURFACE_ID_MAP);
}

static void menu_map_act(void) {
  if ((input_trig & (IN_CANCEL | IN_MAP | IN_OK)) && (map.mode | map.count)) {
    map.mode = 3;
    map.count = 0;
  }
}

static void menu_map_draw(void) {
  int xofs, yofs, cw, ch;
  const u8 colors[2][3] = { { 0x00, 0x00, 0x00 }, { 0xFF, 0xFF, 0xFE } };

  // draw map title
  gfx_draw_fillrect(colors[0], GFX_LAYER_FRONT, 0, 16, VID_WIDTH, 16);
  xofs = (VID_WIDTH - map.title_w) / 2;
  gfx_draw_string(stage_data->title, GFX_LAYER_FRONT, xofs, 16 + (16 - GFX_FONT_HEIGHT) / 2);

  // draw map
  switch (map.mode) {
    case 0: // background rect opening
    case 3: // and closing
      cw = map.scale * stage_data->width * map.count / 8;
      ch = map.scale * stage_data->height * map.count / 8;
      if (map.mode == 3) {
        cw = map.scale * stage_data->width - cw;
        ch = map.scale * stage_data->height - ch;
      }
      xofs = (VID_WIDTH - cw) / 2;
      yofs = 32 + ((VID_HEIGHT - 32) - ch) / 2;
      gfx_draw_fillrect(colors[0], GFX_LAYER_FRONT, xofs, yofs, cw, ch);
      if (++map.count > 8) {
        if (map.mode == 3)
          menu_id = 0;
        else
          map.mode = 1;
        map.count = 0;
      }
      break;
    case 1: // map scanning in from the top
      map.count += 2;
      if (map.count >= stage_data->height) {
        map.mode = 2;
        map.count = stage_data->height;
      }
    /* fallthrough */
    case 2: // and being rendered normally
      cw = map.scale * stage_data->width;
      ch = map.scale * stage_data->height;
      xofs = (VID_WIDTH - cw) / 2;
      yofs = 32 + ((VID_HEIGHT - 32) - ch) / 2;
      // draw background
      gfx_draw_fillrect(colors[0], GFX_LAYER_FRONT, xofs - 4, yofs - 4, cw + 8, ch + 8);
      // draw map
      map.texrect.r.h = map.count;
      if (map.scale == 1)
        gfx_draw_texrect_wide(&map.texrect, GFX_LAYER_FRONT, xofs, yofs);
      else
        gfx_draw_texrect_scaled(&map.texrect, GFX_LAYER_FRONT, xofs, yofs, map.scale);
      // draw player
      if ((map.mode == 2) && ((++map.wait / 8) % 2)) {
        xofs += map.player_x * map.scale - map.scale / 2;
        yofs += map.player_y * map.scale - map.scale / 2;
        gfx_draw_fillrect(colors[1], GFX_LAYER_FRONT, xofs, yofs, map.scale, map.scale);
      }
      break;
  }
}

/* stage select */

#define STAGESEL_TOP ((VID_HEIGHT / 2) - 74)

static struct {
  s32 idx;
  s32 flash;
  s32 title_y;
} stagesel;

static void menu_stagesel_open(void) {
  tsc_switch_script(TSC_SCRIPT_STAGE_SELECT);
  stagesel.idx = 0;
  stagesel.title_y = STAGESEL_TOP + 8;
  tsc_start_event(1000 + tele_dest[stagesel.idx].stage_num);
}

static inline void menu_stagesel_controls(void) {
  if (tele_dest_num == 0)
    return;

  if (input_trig & IN_LEFT)
    --stagesel.idx;
  else if (input_trig & IN_RIGHT)
    ++stagesel.idx;

  if (stagesel.idx < 0)
    stagesel.idx = tele_dest_num - 1;
  if (stagesel.idx >= tele_dest_num)
    stagesel.idx = 0;

  if (input_trig & (IN_LEFT | IN_RIGHT)) {
    snd_play_sound(PRIO_HIGH, 1, FALSE);
    tsc_start_event(1000 + tele_dest[stagesel.idx].stage_num);
  }
}

static void menu_stagesel_act(void) {
  menu_stagesel_controls();

  tsc_update();

  if (input_trig & IN_OK) {
    menu_close();
    tsc_start_event(tele_dest[stagesel.idx].event_num);
    game_flags &= ~(GFLAG_INPUT_ENABLED | GFLAG_UPDATE_OBJECTS);
  } else if (input_trig & (IN_CANCEL | IN_PAUSE)) {
    menu_close();
    tsc_start_event(0);
    game_flags &= ~(GFLAG_INPUT_ENABLED | GFLAG_UPDATE_OBJECTS);
  }
}

static void menu_stagesel_draw(void) {
  ++stagesel.flash;

  if (stagesel.title_y > STAGESEL_TOP)
    --stagesel.title_y;

  gfx_draw_texrect(&rc_stagesel_title, GFX_LAYER_FRONT, VID_WIDTH / 2 - 32, stagesel.title_y);

  if (tele_dest_num == 0) return;

  int x = (VID_WIDTH - (tele_dest_num * 40)) / 2;

  gfx_draw_texrect(&rc_cursor_items[stagesel.flash / 2 % 2], GFX_LAYER_FRONT, x + stagesel.idx * 40, STAGESEL_TOP + 18);

  for (int i = 0; i < tele_dest_num; ++i, x += 40) {
    if (tele_dest[i].stage_num == 0)
      break; // likely nothing after this
    gfx_draw_texrect(&rc_stage[tele_dest[i].stage_num], GFX_LAYER_FRONT, x, STAGESEL_TOP + 18);
  }
}

/* save/load menu */

#define SAVELOAD_TOP ((VID_HEIGHT / 2) - 48)

enum saveload_menu_state {
  SLSTATE_SELECT_MEMCARD,
  SLSTATE_SELECT_SAVE,
  SLSTATE_NO_MEMCARDS,
  SLSTATE_ERROR,
  SLSTATE_OVERWRITE,
  SLSTATE_FORMAT,
};

static struct {
  s32 state;
  mcrd_id_t cards[MCRD_CARDS_PER_PORT * MCRD_NUM_PORTS];
  s32 num_cards;
  u32 slot_mask;
  s32 slot_to_save;
  const char *title;
  // short info about each slot
  struct {
    char stage[MAX_STAGE_TITLE];
    s32 arm;
    s32 life;
    s32 max_life;
  } slots[MCRD_MAX_SAVES];
} saveload;

static const char *str_yesno[] = { "cA", "HET" };
static const char *str_ok[] = { "OK" };
static const char *str_memcard[] = { "KAPTA jAMvTg 1", "KAPTA jAMvTg 2" };

static inline void menu_saveload_set_state(const int state, const char *title, const int num_choices) {
  saveload.state = state;
  saveload.title = title;
  main_count = num_choices;
  main_sel = 0;
}

static void menu_saveload_open(void) {
  // this takes a while the first time
  mcrd_start();

  main_tip_y = MAIN_TOOLTIP_OFSY;

  saveload.num_cards = mcrd_cards_available(saveload.cards);

  if (saveload.num_cards) {
    menu_saveload_set_state(SLSTATE_SELECT_MEMCARD, "BraOP KAPTr jAMvTg", saveload.num_cards);
    // skip memcard 1 if we don't have it
    main_choices = &str_memcard[(saveload.cards[0].port == 1)];
  } else {
     menu_saveload_set_state(SLSTATE_NO_MEMCARDS, "KAPTA jAMvTg HE OaHAPkeEHA.", 1);
  }
}

static inline void menu_saveload_close(const bool success) {
  mcrd_stop();

  // close menu
  menu_id = 0;

  // if we're in the intro or ending stage, re-open the main menu
  if (!stage_data || stage_data->id == STAGE_OPENING_ID || stage_data->id == STAGE_CREDITS_ID) {
    if (stage_data->id == STAGE_CREDITS_ID) {
      game_reset(TRUE);
      game_start_intro();
    } else {
      menu_open(MENU_TITLE);
    }
  } else {
    // nuke the old "want to save?" prompt
    tsc_clear_text();
    if (!success) {
      // didn't save at save point; prevent the TSC "game saved" message from printing
      tsc_stop_event();
      player.cond &= ~PLRCOND_USE_BUTTON; // don't re-activate the save point immediately
    }
  }
}

static void menu_saveload_update_slots(void) {
  saveload.slot_mask = mcrd_save_slots_available();
  if (saveload.slot_mask == 0) return;

  profile_t prof;

  for (int i = 0; i < MCRD_MAX_SAVES; ++i) {
    // read beginning part of slot that contains the data that we need
    const mcrd_result_t res = mcrd_save_read_slot(i, &prof, 2 * MCRD_SECSIZE);
    if (res != MCRD_SUCCESS) {
      saveload.slot_mask &= ~(1 << i); // clear so it doesn't draw garbage
      continue;
    }
    // store that shit
    memcpy(saveload.slots[i].stage, prof.save.stage_title, MAX_STAGE_TITLE);
    saveload.slots[i].life = prof.save.player.life;
    saveload.slots[i].max_life = prof.save.player.max_life;
    saveload.slots[i].arm = prof.save.player.arms[prof.save.player.arm].id;
  }
}

static inline void menu_saveload_act_select_memcard(const u32 btn) {
  if (btn == 0)
    return;

  // btn can be either OK or CANCEL at this point
  snd_play_sound(PRIO_HIGH, 18, FALSE);

  if (btn == IN_CANCEL) {
    menu_saveload_close(FALSE);
    return;
  }

  mcrd_result_t res = mcrd_card_open(saveload.cards[main_sel]);

  if (res == MCRD_UNFORMATTED) {
    // card is unformatted; if we're saving, offer to format it
    if (menu_id == MENU_SAVE)
      menu_saveload_set_state(SLSTATE_FORMAT, "HEOaXOcgMO OTlOPMATgPOBATs KAPTk. BrjOiHgTs?", 2);
    else
      menu_saveload_set_state(SLSTATE_ERROR, "HEOaXOcgMO OTlOPMATgPOBATs KAPTk.", 1);
    return;
  }

  if (res != MCRD_SUCCESS) {
    // error detecting card
    menu_saveload_set_state(SLSTATE_ERROR, "OogaKA nTEHgv KAPTr.", 1);
    return;
  }

  // success; try opening save file
  res = mcrd_save_open(PROFILE_FILENAME);
  if (res != MCRD_SUCCESS) {
      // if we're in SAVE mode, create new save file if one doesn't exist
    if (menu_id == MENU_SAVE)
      res = mcrd_save_create(PROFILE_FILENAME);
  }

  if (res != MCRD_SUCCESS) {
    const char *err = (menu_id == MENU_SAVE) ?
      "OogaKA COXPAHEHgv cAHHrX." : "HET COXPAHdHHrX cAHHrX.";
    menu_saveload_set_state(SLSTATE_ERROR, err, 1);
    return;
  }

  menu_saveload_update_slots();

  // can't load anything off of an empty save
  if (menu_id == MENU_LOAD && !saveload.slot_mask) {
    menu_saveload_set_state(SLSTATE_ERROR, "HET COXPAHdHHrX cAHHrX.", 1);
    return;
  }

  menu_saveload_set_state(SLSTATE_SELECT_SAVE, "BraOP lAhiA", MCRD_MAX_SAVES);
}

static inline void menu_saveload_do_load(const int slot) {
  mcrd_result_t res = mcrd_save_read_slot(slot, &profile, sizeof(profile));
  if (res != MCRD_SUCCESS)  {
    // write error; bail
    menu_saveload_set_state(SLSTATE_ERROR, "OogaKA COXPAHEHgv cAHHrX.", 1);
  } else {
    // try to actually load the game
    if (!profile_load(TRUE))
      menu_saveload_set_state(SLSTATE_ERROR, "cAHHrE jOBPEecEHr.", 1);
    else
      menu_saveload_close(TRUE);
  }
}

static inline void menu_saveload_do_save(const int slot) {
  mcrd_result_t res = mcrd_save_write_slot(slot, &profile, sizeof(profile));
  if (res != MCRD_SUCCESS) {
    // write error
    menu_saveload_set_state(SLSTATE_ERROR, "OogaKA COXPAHEHgv cAHHrX.", 1);
  } else {
    // success; let the "Game saved" TSC message play out
    menu_saveload_close(TRUE);
  }
}

static inline void menu_saveload_act_select_save(const u32 btn) {
  if (btn == 0)
    return;

  // btn can be either OK or CANCEL at this point
  snd_play_sound(PRIO_HIGH, 18, FALSE);

  if (btn == IN_CANCEL) {
    // go back to memcard selection
    menu_saveload_set_state(SLSTATE_SELECT_MEMCARD, "BraOP KAPTr jAMvTg", saveload.num_cards);
    // skip memcard 1 if we don't have it
    main_choices = &str_memcard[(saveload.cards[0].port == 1)];
    return;
  }

  // don't allow to load post-game saves or empty saves
  const bool occupied = ((saveload.slot_mask & (1 << main_sel)) != 0) &&
    (saveload.slots[main_sel].stage[0] != '=');

  if (menu_id == MENU_SAVE) {
    if (occupied) {
      // trying to save over a slot; ask the user
      saveload.slot_to_save = main_sel;
      menu_saveload_set_state(SLSTATE_OVERWRITE, "fAMEHgTs cAHHrE?", 2);
    } else {
      // saving over an empty slot; all is well
      menu_saveload_do_save(main_sel);
    }
  } else if (occupied) {
    // can only load if the slot is occupied
    menu_saveload_do_load(main_sel);
  }
}

static void menu_saveload_act(void) {
  const u32 btn = menu_generic_control();

  switch (saveload.state) {
    case SLSTATE_SELECT_MEMCARD:
      menu_saveload_act_select_memcard(btn);
      break;
    case SLSTATE_SELECT_SAVE:
      menu_saveload_act_select_save(btn);
      break;
    case SLSTATE_OVERWRITE:
      if (btn == IN_OK && main_sel == 0) {
        // user pressed yes, save that shit
        menu_saveload_do_save(saveload.slot_to_save);
      } else if (btn == IN_CANCEL || (btn == IN_OK && main_sel == 1)) {
        // go back to save selection
        menu_saveload_set_state(SLSTATE_SELECT_SAVE, "BraOP lAhiA", MCRD_MAX_SAVES);
      }
      break;
    case SLSTATE_FORMAT:
      if (btn == IN_OK && main_sel == 0) {
        // user pressed yes, format that shit
        mcrd_result_t res = mcrd_card_format();
        if (res != MCRD_SUCCESS)
          menu_saveload_set_state(SLSTATE_ERROR, "HE kcAiOCs OTlOPMATgPOBATs KAPTk. ", 1);
        else
          menu_saveload_set_state(SLSTATE_SELECT_SAVE, "BraOP lAhiA", MCRD_MAX_SAVES);
      } else if (btn == IN_CANCEL || (btn == IN_OK && main_sel == 1)) {
        // go back to memcard selection
        menu_saveload_set_state(SLSTATE_SELECT_MEMCARD, "BraOP KAPTr jAMvTg", saveload.num_cards);
        // skip memcard 1 if we don't have it
        main_choices = &str_memcard[(saveload.cards[0].port == 1)];
      }
      break;
    default: // all the other "OK" message prompts, etc
      if (btn) {
        snd_play_sound(PRIO_HIGH, 18, FALSE);
        menu_saveload_close(FALSE);
      }
      break;
  }
}

static inline void menu_saveload_draw_select_save(void) {
  draw_string_centered(saveload.title, main_title_rgb, VID_WIDTH / 2, 36);

  // draw slots using the textbox border
  int yofs = SAVELOAD_TOP;
  char lifetext[16];
  for (int i = 0; i < MCRD_MAX_SAVES; ++i, yofs += 32) {
    gfx_draw_texrect(&rc_menubox[0], GFX_LAYER_FRONT, TEXT_BOX_LEFT, yofs);
    gfx_draw_texrect(&rc_menubox[1], GFX_LAYER_FRONT, TEXT_BOX_LEFT, yofs + 8);
    gfx_draw_texrect(&rc_menubox[1], GFX_LAYER_FRONT, TEXT_BOX_LEFT, yofs + 16);
    gfx_draw_texrect(&rc_menubox[2], GFX_LAYER_FRONT, TEXT_BOX_LEFT, yofs + 24);

    if (!(saveload.slot_mask & (1 << i))) {
      // slot is empty
      draw_string_centered("jkCTO", NULL, VID_WIDTH / 2, yofs + 10);
    } else if (saveload.slots[i].max_life <= 0) {
      // corrupted save?
      draw_string_centered("lAhi jOBPEecdH", NULL, VID_WIDTH / 2, yofs + 10);
    } else {
      // draw stage name
      draw_string_shadow(saveload.slots[i].stage, NULL, TEXT_BOX_LEFT + 30, yofs + 10);
      // draw life at the right edge
      sprintf(lifetext, "%02d/%02d", saveload.slots[i].life, saveload.slots[i].max_life);
      gfx_draw_texrect_16x16(&rc_heart, GFX_LAYER_FRONT, TEXT_BOX_LEFT + 244 - 46, yofs + 7);
      draw_string_shadow(lifetext, NULL, TEXT_BOX_LEFT + 244 - 40 - 6 * GFX_FONT_WIDTH, yofs + 10);
      // draw current weapon (unfortunately weapon list won't fit)
      gfx_draw_texrect_16x16(&hud_rc_arms[saveload.slots[i].arm], GFX_LAYER_FRONT, TEXT_BOX_LEFT + 244 - 24, yofs + 8);
    }
  }

  // draw cursor
  gfx_draw_texrect_16x16(&rc_cursor_menu[++main_tick / 10 % 4], GFX_LAYER_FRONT, TEXT_BOX_LEFT + 8, SAVELOAD_TOP + 32 * main_sel + 8);
}

static void menu_saveload_draw(void) {
  gfx_draw_clear(main_bg_rgb, GFX_LAYER_FRONT);

  int xofs;
  switch (saveload.state) {
    case SLSTATE_SELECT_SAVE:
      menu_saveload_draw_select_save();
      break;
    default:
      main_choices = (main_count == 1) ? str_ok : str_yesno;
      /* fallthrough */
    case SLSTATE_SELECT_MEMCARD:
      xofs = (strlen(main_choices[0]) * GFX_FONT_WIDTH - 24) / 2;
      draw_string_centered(saveload.title, main_title_rgb, VID_WIDTH / 2, VID_HEIGHT / 2 - 48);
      menu_generic_draw(VID_WIDTH / 2 - xofs, MAIN_TOP + 80 - 24);
      break;
  }

  menu_draw_tooltip();
}

/* options menu */

#define OPT_SFX_VOL_STEP ((SFX_MAX_VOLUME + 1) / 8)
#define OPT_ORG_VOL_STEP ((ORG_MAX_VOLUME + 1) / 8)
#define OPT_NAME_XOFS (16 * GFX_FONT_WIDTH)
#define OPT_VAL_XOFS (8 * GFX_FONT_WIDTH)

enum option_type {
  OPT_SEP,
  OPT_SCROLL,
  OPT_BIND,
};

static struct {
  s32 sel;
  s32 count;
  s32 first;
  s32 last;
  u16 *bind;
  u16 *counterbind;
  bool lock;
  s32 vol_sfx;
  s32 vol_org;
} optmenu;

static const struct option {
  s32 type;
  const char *name;
  void *value;
  s32 max;
  s32 def;
} options[] = {
  { OPT_SEP,    "HACTPOhKg fBkKA",         NULL,             0,  0            },
  { OPT_SCROLL, "bPOMKOCTs tllEKTOB",  &optmenu.vol_sfx, 8,  8            },
  { OPT_SCROLL, "bPOMKOCTs MkfrKg",  &optmenu.vol_org, 8,  6            },
  { OPT_SEP,    "kjPABiEHgE",      NULL,             0,  0            },
  { OPT_BIND,   "jPreOK",          &input_binds[4],  0,  PAD_CROSS    },
  { OPT_BIND,   "ObOHs",          &input_binds[5],  0,  PAD_SQUARE   },
  { OPT_BIND,   "jPEc. OPkegE",   &input_binds[6],  0,  PAD_TRIANGLE },
  { OPT_BIND,   "CiEc. OPkegE",   &input_binds[7],  0,  PAD_CIRCLE   },
  { OPT_BIND,   "KHOjKA BraOP", &input_binds[10], 11, PAD_CROSS    },
  { OPT_BIND,   "KHOjKA OTMEHA", &input_binds[11], 10, PAD_CIRCLE   },
};

static void menu_options_open(void) {
  optmenu.bind = optmenu.counterbind = NULL;
  optmenu.count = sizeof(options) / sizeof(*options);
  optmenu.sel = optmenu.first = 1;
  optmenu.last = optmenu.count - 1;
  optmenu.vol_sfx = (snd_sfx_volume + 1) / OPT_SFX_VOL_STEP;
  optmenu.vol_org = (org_get_master_volume() + 1) / OPT_ORG_VOL_STEP;
  optmenu.lock = FALSE;
}

static inline void menu_options_apply(void) {
  snd_set_sfx_volume(optmenu.vol_sfx * OPT_SFX_VOL_STEP);
  org_set_master_volume(optmenu.vol_org * OPT_ORG_VOL_STEP);
}

static void menu_options_act(void) {
  if (optmenu.bind) {
    if (optmenu.lock) {
      // waiting for the user to release all buttons
      if (!input_pad)
        optmenu.lock = FALSE;
      return;
    }
    if (input_pad) {
      // we were waiting for a bind and user pressed a button
      const u16 old_bind = *optmenu.bind;
      switch (input_pad) {
        case PAD_CROSS:
        case PAD_CIRCLE:
        case PAD_SQUARE:
        case PAD_TRIANGLE:
          *optmenu.bind = input_pad;
          // swap binds if necessary
          if (optmenu.counterbind && *optmenu.counterbind == input_pad)
            *optmenu.counterbind = old_bind;
          /* fallthrough */
        case PAD_START: // start just cancels it out
          snd_play_sound(PRIO_HIGH, 18, FALSE);
          optmenu.bind = optmenu.counterbind = NULL;
          input_suppress_trig = TRUE;
          break;
      }
      return;
    }
  }

  if (input_trig & IN_CANCEL) {
    snd_play_sound(PRIO_HIGH, 18, FALSE);
    menu_id = 0;
    menu_open((stage_data && stage_data->id == STAGE_OPENING_ID) ? MENU_TITLE : MENU_PAUSE);
    return;
  }

  bool scroll_changed = FALSE;

  if (input_trig & IN_DOWN) {
    ++optmenu.sel;
    while (optmenu.sel < optmenu.count && options[optmenu.sel].type == OPT_SEP)
      ++optmenu.sel; // skip separators
    if (optmenu.sel > optmenu.last)
      optmenu.sel = optmenu.first;
    snd_play_sound(PRIO_HIGH, 1, FALSE);
  } else if (input_trig & IN_UP) {
    --optmenu.sel;
    while (optmenu.sel >= 0 && options[optmenu.sel].type == OPT_SEP)
      --optmenu.sel; // skip separators
    if (optmenu.sel < optmenu.first)
      optmenu.sel = optmenu.last;
    snd_play_sound(PRIO_HIGH, 1, FALSE);
  }

  if (options[optmenu.sel].type == OPT_SCROLL) {
    s32 *pval = (s32 *)options[optmenu.sel].value;
    if ((input_trig & IN_LEFT) && *pval > 0) {
      --*pval;
      scroll_changed = TRUE;
      snd_play_sound(PRIO_HIGH, 1, FALSE);
    } else if ((input_trig & (IN_RIGHT | IN_OK)) && *pval < options[optmenu.sel].max) {
      ++*pval;
      scroll_changed = TRUE;
      snd_play_sound(PRIO_HIGH, 1, FALSE);
    }
  } else if (options[optmenu.sel].type == OPT_BIND) {
    if (input_trig & IN_OK) {
      snd_play_sound(PRIO_HIGH, 18, FALSE);
      optmenu.bind = options[optmenu.sel].value;
      optmenu.lock = TRUE;
      // HACK: maybe add a field for this or turn `option` into a union
      if (options[optmenu.sel].max)
        optmenu.counterbind = &input_binds[options[optmenu.sel].max];
      else
        optmenu.counterbind = NULL;
      return;
    }
  }

  if (scroll_changed)
    menu_options_apply();
}

static void menu_options_draw(void) {
  gfx_draw_clear(main_bg_rgb, GFX_LAYER_FRONT);

  int yofs = 8;
  const int xname = VID_WIDTH / 2 - OPT_NAME_XOFS;
  const int xval = VID_WIDTH / 2 + OPT_VAL_XOFS;
  const int y = GFX_FONT_HEIGHT;

  static const char bar[] = "||||||||||||||||||||||||||||||||";
  int bar_total, bar_val, btn;

  gfx_draw_texrect_16x16(&rc_cursor_menu[++main_tick / 10 % 4], GFX_LAYER_FRONT, xname - 16 - 8, y + 5 + optmenu.sel * 20);
  for (int i = 0; i < optmenu.count; ++i, yofs += 20) {
    switch (options[i].type) {
      case OPT_SEP:
        draw_string_centered(options[i].name, main_title_rgb, VID_WIDTH / 2, y + yofs);
        break;

      case OPT_SCROLL:
        // name
        draw_string_shadow(options[i].name, NULL, xname, y + yofs);
        // value
        bar_total = (sizeof(bar) - options[i].max - 1);
        bar_val = (sizeof(bar) - *(s32 *)options[i].value - 1);
        draw_string_shadow(bar + bar_total, main_barbg_rgb, xval, y + yofs);
        gfx_draw_string(bar + bar_val, GFX_LAYER_FRONT, xval, y + yofs);
        break;

      case OPT_BIND:
        // name
        draw_string_shadow(options[i].name, NULL, xname, y + yofs);
        // value
        if (optmenu.bind == options[i].value) {
          if (main_tick / 16 % 2)
            draw_string_shadow("...", NULL, xval + GFX_FONT_WIDTH * (8 - 3), y + yofs);
        } else {
          btn = menu_padbutton_index(*(u16 *)options[i].value);
          gfx_draw_texrect(&rc_padbuttons[btn], GFX_LAYER_FRONT, xval + GFX_FONT_WIDTH * 8 - 12, y + yofs);
        }
        break;
    }
  }
}

/* falling island "menu" */

static struct {
  s32 mode; // 0 - fall to the ground, 1 - slow down mid-fall
  s32 wait;
  s32 x;
  s32 y;
} fall;

static void menu_fall_open(void) {
  // texrects for this are only loaded in one specific map, so we init them here
  // HACK: surface is split into 2 160x80 surfaces to counteract alignment issues
  if (!rc_island_sky.tpage)
    gfx_set_texrect(&rc_island_sky, SURFACE_ID_LEVEL_SPRITESET_1);
  if (!rc_island_ground.tpage)
    gfx_set_texrect(&rc_island_ground, SURFACE_ID_LEVEL_SPRITESET_2);
  if (!rc_island_sprite.tpage)
    gfx_set_texrect(&rc_island_sprite, SURFACE_ID_LEVEL_SPRITESET_2);

  fall.mode = (menu_id == MENU_FALLING_ISLAND_1);
  fall.x = TO_FIX(168);
  fall.y = TO_FIX(64);
  fall.wait = 0;
}

static void menu_fall_act(void) {
  ++fall.wait;

  if (fall.wait > 900) {
    menu_id = 0;
    return;
  }

  switch (fall.mode) {
    case 0:
      // fall at constant speed
      fall.y += FIX_SCALE / 10;
      break;
    case 1:
      // slow down gradually
      if (fall.wait < 350)
        fall.y += FIX_SCALE / 10;
      else if (fall.wait < 500)
        fall.y += FIX_SCALE / 20;
      else if (fall.wait < 600)
        fall.y += FIX_SCALE / 40;
      else if (fall.wait == 750)
        fall.wait = 900; // end prematurely
      break;
  }
}

static void menu_fall_draw(void) {
  // draw a clear on the background layer just in case there's nothing on the map
  gfx_draw_clear(main_black_rgb, GFX_LAYER_BACK);
  gfx_draw_clear(main_black_rgb, GFX_LAYER_FRONT);
  gfx_push_cliprect(GFX_LAYER_FRONT, 0, VID_HEIGHT / 2 - 40, VID_WIDTH, 80);
  gfx_draw_texrect(&rc_island_sky, GFX_LAYER_FRONT, VID_WIDTH / 2 - 80, VID_HEIGHT / 2 - 40);
  gfx_draw_texrect(&rc_island_sprite, GFX_LAYER_FRONT, TO_INT(fall.x) - 20, TO_INT(fall.y) - 12);
  gfx_draw_texrect(&rc_island_ground, GFX_LAYER_FRONT, VID_WIDTH / 2 - 80, VID_HEIGHT / 2 + 40 - 32);
  gfx_pop_cliprect(GFX_LAYER_FRONT);
  // draw counter
  if (player.equip & EQUIP_NIKUMARU_COUNTER)
    hud_draw_time(16, 16);
}

/* -------------- */

typedef void (*menu_func_t)(void);

static const struct menu_desc {
  menu_func_t openfunc;
  menu_func_t actfunc;
  menu_func_t drawfunc;
  bool need_tsc;
} menu_desctab[] = {
  { menu_null,           menu_null,          menu_null,           FALSE }, // NONE
  { menu_title_open,     menu_title_act,     menu_title_draw,     FALSE }, // TITLE
  { menu_pause_open,     menu_pause_act,     menu_pause_draw,     FALSE }, // PAUSE
  { menu_inventory_open, menu_inventory_act, menu_inventory_draw, TRUE  }, // INVENTORY
  { menu_map_open,       menu_map_act,       menu_map_draw,       TRUE  }, // MAP
  { menu_stagesel_open,  menu_stagesel_act,  menu_stagesel_draw,  TRUE  }, // STAGESELECT
  { menu_saveload_open,  menu_saveload_act,  menu_saveload_draw,  FALSE }, // SAVE
  { menu_saveload_open,  menu_saveload_act,  menu_saveload_draw,  FALSE }, // LOAD
  { menu_options_open,   menu_options_act,   menu_options_draw,   FALSE }, // OPTIONS
  { menu_fall_open,      menu_fall_act,      menu_fall_draw,      FALSE }, // FALLING_ISLAND_0
  { menu_fall_open,      menu_fall_act,      menu_fall_draw,      FALSE }, // FALLING_ISLAND_1
};

void menu_open(const int menu) {
  menu_id = menu;
  menu_desctab[menu].openfunc();
}

int menu_active(void) {
  return menu_id;
}

void menu_act(void) {
  menu_desctab[menu_id].actfunc();
}

void menu_draw(void) {
  menu_desctab[menu_id].drawfunc();
}

bool menu_uses_tsc(void) {
  return menu_desctab[menu_id].need_tsc;
}

void menu_close(void) {
  tsc_stop_event();
  tsc_switch_script(TSC_SCRIPT_STAGE);
  menu_id = MENU_NONE;
}
