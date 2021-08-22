#include <string.h>

#include "engine/common.h"
#include "engine/graphics.h"
#include "engine/math.h"

#include "game/dmgnum.h"
#include "game/hud.h"

dmgnum_t dmgnum_list[DMGNUM_MAX];

static int dmgnum_last = 0;

void dmgnum_init(void) {
  memset_word(dmgnum_list, 0, sizeof(dmgnum_list));
}

void dmgnum_spawn(int *tgt_x, int *tgt_y, int val) {
  int i;
  for (i = 0; i < DMGNUM_MAX; ++i) {
    const dmgnum_t *tnum = &dmgnum_list[i];
    if (tnum->cond && tnum->tgt_x == tgt_x && m_sign(val) == m_sign(tnum->val))
      break;
  }

  dmgnum_t *dnum;
  if (i == DMGNUM_MAX) {
    // new damage display
    dnum = &dmgnum_list[dmgnum_last++];
    if (dmgnum_last == DMGNUM_MAX)
      dmgnum_last = 0;
    dnum->count = 0;
    dnum->yofs = 0;
    dnum->val = val;
  } else {
    // accumulate into old damage display (e.g. exp values)
    dnum = &dmgnum_list[i];
    dnum->count = 32;
    dnum->val += val;
    val = dnum->val;
  }

  dnum->cond = TRUE;
  dnum->tgt_x = tgt_x;
  dnum->tgt_y = tgt_y;
  dnum->vofs = 0;

  const int div[4] = { 1000, 100, 10, 1 };
  const int widths[4] = { 16, 24, 32, 40 };
  int digits[5] = { 20, 0, 0, 0, 0 };
  int numdigits = 1; // includes sign
  if (val < 0) {
    digits[0] = 21;
    digits[1] = digits[2] = digits[3] = digits[4] = 10;
    val = -val;
  }

  for (int i = 0; i < 4; ++i) {
    int d = val / div[i];
    if (d) {
      digits[numdigits++] += d;
      val -= d * div[i];
    }
  }

  dnum->digits = numdigits;
  dnum->xofs = -TO_FIX(8 * numdigits / 2);

  for (int i = 0; i < numdigits; ++i)
    dnum->texrects[i] = &hud_rc_digit[digits[i]];
}

void dmgnum_act(void) {
  for (int i = 0; i < DMGNUM_MAX; ++i) {
    dmgnum_t *dnum = &dmgnum_list[i];
    if (dnum->cond) {
      if (++dnum->count < 32)
        dnum->yofs -= 0x100;
      if (dnum->count > 80)
        dnum->cond = FALSE;
      else if (dnum->count > 72)
        ++dnum->vofs;
    }
  }
}

void dmgnum_draw(int cam_x, int cam_y) {
  cam_x = TO_INT(cam_x);
  cam_y = TO_INT(cam_y);
  for (int i = 0; i < DMGNUM_MAX; ++i) {
    dmgnum_t *dnum = &dmgnum_list[i];
    if (dnum->cond) {
      int x = TO_INT(*dnum->tgt_x + dnum->xofs) - cam_x;
      const int y = TO_INT(*dnum->tgt_y + dnum->yofs) - cam_y - 4;
      for (int d = 0; d < dnum->digits; ++d, x += 8)
        gfx_draw_texrect_ofs(dnum->texrects[d], GFX_LAYER_FRONT, x, y, 0, dnum->vofs);
    }
  }
}
