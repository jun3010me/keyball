/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H
#include "quantum.h"
#include "transactions.h"
#include <avr/eeprom.h>

// Custom EEPROM layout for tapping term (outside eeconfig/VIA/keymap areas).
// Bytes 1020-1021: saved tapping term value (uint16_t)
// Bytes 1022-1023: magic sentinel — 0xBEEF confirms we wrote these bytes
// config.h sets DYNAMIC_KEYMAP_EEPROM_MAX_ADDR 1019 so VIA macro reset never touches them.
#define TT_EEPROM_VAL   ((uint16_t*)1020)
#define TT_EEPROM_MAGIC ((uint16_t*)1022)
#define TT_MAGIC_VALUE  0xBEEF

static void tt_save(uint16_t val) {
    eeprom_update_word(TT_EEPROM_VAL, val);
    eeprom_update_word(TT_EEPROM_MAGIC, TT_MAGIC_VALUE);
}

// Set to true when tt_load() successfully reads a valid value from EEPROM.
// Propagated to slave via sync packet so the slave OLED can show S/D.
static bool tt_from_eeprom = false;

static uint16_t tt_load(void) {
    if (eeprom_read_word(TT_EEPROM_MAGIC) == TT_MAGIC_VALUE) {
        uint16_t v = eeprom_read_word(TT_EEPROM_VAL);
        if (v >= 50 && v <= 2000) {
            tt_from_eeprom = true;
            return v;
        }
    }
    tt_from_eeprom = false;
    return TAPPING_TERM;
}

// Sync payload: tapping term + modifier state + EEPROM-load flag (master → slave).
typedef struct { uint16_t tapping_term; uint8_t mods; uint8_t from_eeprom; } sync_state_t;
static uint8_t synced_mods = 0;

// Custom keycodes for escaping the auto mouse layer.
//   ESC_ML      - exit AML only
//   ESC_ML_LNG2 - exit AML then tap LNG2 (英数, switch to English input)
//   ESC_ML_LNG1 - exit AML then tap LNG1 (かな, switch to Japanese input)
enum my_keycodes {
    ESC_ML = KEYBALL_SAFE_RANGE,
    ESC_ML_LNG2,
    ESC_ML_LNG1,
};

// Deferred LNG key: sent on the next matrix scan after AML exit so that
// the layer change is fully committed before the HID report is generated.
static uint8_t pending_lang_key = 0;

void matrix_scan_user(void) {
    if (pending_lang_key) {
        tap_code(pending_lang_key);
        pending_lang_key = 0;
    }

#if defined(SPLIT_KEYBOARD)
    if (is_keyboard_master()) {
        static uint16_t last_term = 0;
        static uint8_t  last_mods = 0xff;
        static uint16_t last_saved = TAPPING_TERM;
        uint8_t cur_mods = get_mods() | get_oneshot_mods();
        if (g_tapping_term != last_term || cur_mods != last_mods) {
            sync_state_t d = {g_tapping_term, cur_mods, (uint8_t)tt_from_eeprom};
            if (transaction_rpc_send(SYNC_TAPPING_TERM, sizeof(d), &d)) {
                last_term = g_tapping_term;
                last_mods = cur_mods;
            }
        }
        if (g_tapping_term != last_saved) {
            tt_save(g_tapping_term);
            last_saved = g_tapping_term;
        }
    }
#endif
}


bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case ESC_ML:
            if (record->event.pressed) {
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
                keyball_escape_mouse_layer();
#endif
            }
            return false;
        case ESC_ML_LNG2:
            if (record->event.pressed) {
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
                keyball_escape_mouse_layer();
#endif
                pending_lang_key = KC_LNG2;
            }
            return false;
        case ESC_ML_LNG1:
            if (record->event.pressed) {
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
                keyball_escape_mouse_layer();
#endif
                pending_lang_key = KC_LNG1;
            }
            return false;
    }
    return true;
}

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

  [0] = LAYOUT_universal(
    KC_Q     , KC_W     , KC_E     , KC_R     , KC_T     ,                            KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     ,
    KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                            KC_H     , KC_J     , KC_K     , KC_L     , KC_SCLN  ,
    KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     ,                            KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  ,
    KC_LCTL  , KC_LGUI  , KC_LALT  , LSFT_T(KC_LNG2), LT(5,KC_SPC), LT(3,KC_LNG1),  KC_BSPC  , LT(2,KC_ENT), LSFT_T(KC_LNG2), KC_RALT, KC_RGUI, KC_RCTL
  ),

  [1] = LAYOUT_universal(
    LGUI(KC_C), LGUI(KC_V), LGUI(KC_X), KC_DEL  , LT(3,KC_NO),                       KC_NO    , KC_BTN1  , LALT(KC_W), KC_ESC   , TG(0)    ,
    KC_NO     , LCTL(KC_C), LCTL(KC_UP), LCTL(KC_V), LCTL(KC_LEFT),                  LCTL(KC_RGHT), KC_BTN1, KC_UP   , KC_BTN2  , TG(1)    ,
    KC_NO     , LCTL(KC_X), KC_NO     , KC_NO    , LCTL(KC_DOWN),                     KC_NO    , KC_LEFT  , KC_DOWN  , KC_RGHT  , TG(1)    ,
    KC_LCTL   , KC_NO     , KC_NO     , ESC_ML_LNG2, LT(3,KC_NO), ESC_ML_LNG1,        KC_NO    , LT(1,KC_NO), KC_NO  , KC_RALT  , KC_RGUI  , KC_LCTL
  ),

  [2] = LAYOUT_universal(
    LGUI(KC_C), KC_UP     , LGUI(KC_V), LCTL(KC_PGUP), LGUI(KC_X),                   LALT(KC_GRV), LGUI(KC_W), LCTL(KC_PGDN), KC_TAB  , LGUI(KC_T),
    LGUI(KC_F), LCTL(KC_LEFT), LGUI(KC_DOWN), LCTL(KC_RGHT), LSFT(KC_W),             LCTL(KC_LNG1), KC_PGUP  , KC_LNG1  , KC_4    , LSFT_T(KC_NO),
    LCTL(KC_C), LCTL(KC_UP), LCTL(KC_V), LCTL(KC_LEFT), LGUI(KC_LBRC),               KC_PGDN  , LCTL(KC_X), KC_NO    , LGUI(LSFT(KC_5)), LGUI(KC_MINS),
    KC_LCTL   , KC_LGUI   , KC_NO     , KC_DEL   , LT(2,KC_NO), KC_BSPC ,             KC_NO    , KC_NO     , KC_NO   , KC_NO    , LGUI(KC_0), KC_NO
  ),

  [3] = LAYOUT_universal(
    RGB_TOG  , AML_TO   , AML_I50  , AML_D50  , KC_NO    ,                            DT_DOWN  , DT_UP    , SSNP_HOR , SSNP_VRT , SSNP_FRE ,
    RGB_MOD  , RGB_HUI  , RGB_SAI  , RGB_VAI  , KBC_SAVE ,                            KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,
    RGB_RMOD , RGB_HUD  , RGB_SAD  , RGB_VAD  , KC_NO    ,                            CPI_D1K  , CPI_D100 , CPI_I100 , CPI_I1K  , KBC_SAVE ,
    QK_BOOT  , KBC_RST  , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,      KC_LNG2  , KC_LNG1  , KC_NO    , KC_NO    , KBC_RST  , QK_BOOT
  ),

  [4] = LAYOUT_universal(
    KC_NO    , KC_F3    , KC_NO    , KC_F4    , KC_NO    ,                            KC_NO    , KC_F7    , KC_NO    , KC_NO    , KC_NO    ,
    KC_F1    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                            KC_F8    , KC_NO    , KC_F9    , KC_NO    , KC_NO    ,
    KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                            KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_F11   ,
    KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,      KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO
  ),

  [5] = LAYOUT_universal(
    KC_EXLM  , KC_AT    , KC_HASH  , KC_DLR   , KC_PERC  ,                            KC_CIRC  , KC_AMPR  , KC_ASTR  , KC_LPRN  , KC_RPRN  ,
    KC_1     , KC_2     , KC_3     , KC_4     , KC_5     ,                            KC_6     , KC_7     , KC_8     , KC_9     , KC_0     ,
    KC_BSLS  , KC_LBRC  , KC_RBRC  , KC_MINS  , KC_EQL   ,                            KC_GRV   , KC_QUOT  , KC_DQUO  , KC_TILD  , KC_NO    ,
    KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,      KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO
  ),

  [6] = LAYOUT_universal(
    KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  ,                            KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  ,
    KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  ,                            KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  ,
    KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  ,                            KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  ,
    KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  ,      KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS  , KC_TRNS
  ),
};
// clang-format on

// ---------------------------------------------------------------------------
// Split sync: push g_tapping_term from master to slave each time it changes
// ---------------------------------------------------------------------------

static void sync_tapping_term_slave_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    const sync_state_t *d = (const sync_state_t *)in_data;
    g_tapping_term = d->tapping_term;
    synced_mods    = d->mods;
    tt_from_eeprom = d->from_eeprom;
}

void keyboard_post_init_user(void) {
    transaction_register_rpc(SYNC_TAPPING_TERM, sync_tapping_term_slave_handler);
    set_auto_mouse_layer(1);
    set_auto_mouse_enable(true);
#ifdef DYNAMIC_TAPPING_TERM_ENABLE
    if (is_keyboard_master()) {
        g_tapping_term = tt_load();
    }
#endif
}


layer_state_t layer_state_set_user(layer_state_t state) {
  #ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
    keyball_handle_auto_mouse_layer_change(state); // ← これを追加
  #endif
    // Layer 3 のときスクロールモード ON
    keyball_set_scroll_mode(get_highest_layer(state) == 3);

    // 現在の明るさを維持しつつレイヤーで色を変える
    uint8_t current_val = rgblight_get_val();
    uint8_t hue = 0;

    switch (get_highest_layer(state)) {
        case 0: hue =  85; break; // 蛍光グリーン（デフォルト）
        case 1: hue = 128; break; // 蛍光シアン（マウス・AML）
        case 2: hue =  43; break; // 蛍光イエロー
        case 3: hue =  21; break; // 蛍光オレンジ（設定・スクロール）
        case 4: hue = 213; break; // 蛍光ピンク（ファンクション）
        case 5: hue = 192; break; // 蛍光パープル（記号・数字）
        case 6: hue =   0; break; // 蛍光レッド（未使用）
        default: hue =  85; break;
    }
    rgblight_sethsv_noeeprom(hue, 255, current_val);

    return state;
}

#ifdef OLED_ENABLE
#    include "lib/oledkit/oledkit.h"

// Right-justify unsigned 16-bit value in a 4-char space-padded field.
static void oled_write_num4(uint16_t n) {
    char    buf[5] = "    ";
    uint8_t i      = 4;
    buf[i]         = '\0';
    if (n == 0) {
        buf[--i] = '0';
    } else {
        while (n > 0 && i > 0) {
            buf[--i] = '0' + (n % 10);
            n /= 10;
        }
    }
    oled_write(buf, false);
}

// Lower nibble of x as a hex digit.
static char nibble_hex(uint8_t x) {
    x &= 0x0f;
    return x < 10 ? x + '0' : x + 'a' - 10;
}

// Both OLEDs mounted vertically, connector pins at the bottom.
// OLED_ROTATION_270 renders text top-to-bottom in portrait orientation.
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_270;
}

// ── Master OLED – key / ball / layer (portrait 5 chars/row) ──────────────────
// Note: writing exactly 5 chars auto-wraps to the next row via oled_advance_char().
// Do NOT call oled_advance_page() after a 5-char write — it would skip an extra row.

void oledkit_render_info_user(void) {
    // Row 0: key position  "R?C? "
    oled_write_char('R', false);
    oled_write_char(nibble_hex(keyball.last_pos.row), false);
    oled_write_char('C', false);
    oled_write_char(nibble_hex(keyball.last_pos.col), false);
    oled_write_char(' ', false);

    // Row 1: keycode  "K??  "
    oled_write_char('K', false);
    oled_write_char(nibble_hex(keyball.last_kc >> 4), false);
    oled_write_char(nibble_hex(keyball.last_kc), false);
    oled_write_P(PSTR("  "), false);

    // Row 2: pressing keys (always exactly 5 chars)
    for (uint8_t i = 0; i < 5; i++) oled_write_char(keyball.pressing_keys[i], false);

    // Rows 3-4: ball x, y  "x NNN" / "y NNN"
#define WB(label, val) do { \
    int8_t _v = (val); \
    oled_write_char((label), false); \
    oled_write_num4((uint16_t)(_v < 0 ? -(int)_v : (int)_v)); \
} while (0)
    WB('x', keyball.last_mouse.x);
    WB('y', keyball.last_mouse.y);
#undef WB

    // Row 5: CPI  "CXX00"
    {
        uint8_t c = keyball_get_cpi();
        oled_write_char('C', false);
        oled_write_char(c >= 10 ? '0' + (c / 10) : ' ', false);
        oled_write_char('0' + (c % 10), false);
        oled_write_P(PSTR("00"), false);
    }

    // Row 6: layer  "L:N  "
    oled_write_char('L', false);
    oled_write_char(':', false);
    oled_write_char('0' + get_highest_layer(layer_state), false);
    oled_write_P(PSTR("  "), false);

    // Rows 7-8: AML status and timeout
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
    if (!get_auto_mouse_enable()) {
        oled_write_P(PSTR("AmOff"), false);
    } else if (layer_state_is(AUTO_MOUSE_DEFAULT_LAYER)) {
        oled_write_P(PSTR("Am:AC"), false);
    } else {
        oled_write_P(PSTR("Am:On"), false);
    }
    oled_write_char('a', false);
    oled_write_num4(keyball.auto_mouse_layer_timeout);
#endif
}

// ── Slave OLED – settings + modifier display (portrait 5 chars/row) ──────────

void oledkit_render_logo_user(void) {
    // Row 0: keyboard label
    oled_write_P(PSTR(" K39 "), false);

    // Row 1: tapping term label
    oled_write_P(PSTR(" Tap "), false);

    // Row 2: tapping term  " NNNS" (S=loaded from EEPROM, D=firmware default)
    oled_write_num4(g_tapping_term);
    oled_write_char(tt_from_eeprom ? 'S' : 'D', false);

    // Rows 3-5: RGB info
#ifdef RGBLIGHT_ENABLE
    oled_write_P(PSTR(" RGB "), false);
    oled_write_char('h', false);
    oled_write_num4(rgblight_get_hue());
    oled_write_char('v', false);
    oled_write_num4(rgblight_get_val());
#else
    oled_write_P(PSTR(" RGB "), false);
    oled_write_P(PSTR(" N/A "), false);
    oled_write_P(PSTR("     "), false);
#endif

    // Rows 6-9: modifier keys – inverted when pressed
    oled_write_P(PSTR("Ctrl "), (bool)(synced_mods & MOD_MASK_CTRL));
    oled_write_P(PSTR("Shft "), (bool)(synced_mods & MOD_MASK_SHIFT));
    oled_write_P(PSTR(" Alt "), (bool)(synced_mods & MOD_MASK_ALT));
    oled_write_P(PSTR(" GUI "), (bool)(synced_mods & MOD_MASK_GUI));

    // Row 10: scroll divisor
    oled_write_P(PSTR("Div:"), false);
    oled_write_char('0' + keyball_get_scroll_div(), false);
}
#endif