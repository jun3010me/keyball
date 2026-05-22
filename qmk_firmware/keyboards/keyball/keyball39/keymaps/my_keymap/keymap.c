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

// Store tapping term at the last 2 bytes of EEPROM (address 1022-1023).
// This avoids the eeconfig block (0-36) and is far from VIA keymap (41-712)
// and macro storage (713+), so it survives eeconfig_init() and VIA resets.
#define TAPPING_TERM_EEPROM_ADDR ((uint16_t *)1022)

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

#if defined(DYNAMIC_TAPPING_TERM_ENABLE) && defined(SPLIT_KEYBOARD)
    if (is_keyboard_master()) {
        static uint16_t last_synced = 0;
        static uint16_t last_saved  = 0;
        if (g_tapping_term != last_synced) {
            if (transaction_rpc_send(SYNC_TAPPING_TERM, sizeof(g_tapping_term), &g_tapping_term)) {
                last_synced = g_tapping_term;
            }
        }
        if (g_tapping_term != last_saved) {
            eeprom_update_word(TAPPING_TERM_EEPROM_ADDR, g_tapping_term);
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
    g_tapping_term = *(const uint16_t *)in_data;
#ifdef DYNAMIC_TAPPING_TERM_ENABLE
    eeprom_update_word(TAPPING_TERM_EEPROM_ADDR, g_tapping_term);
#endif
}

void keyboard_post_init_user(void) {
    transaction_register_rpc(SYNC_TAPPING_TERM, sync_tapping_term_slave_handler);
    set_auto_mouse_layer(1);
    set_auto_mouse_enable(true);
#ifdef DYNAMIC_TAPPING_TERM_ENABLE
    uint16_t saved = eeprom_read_word(TAPPING_TERM_EEPROM_ADDR);
    if (saved >= 50 && saved <= 2000) {
        g_tapping_term = saved;
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

// Right-justify n into a 4-char field, space-padded.
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

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}

// Override the logo display on the secondary (slave) OLED with
// keyboard control information that does not fit on the primary OLED.
void oledkit_render_logo_user(void) {
    // Row 1: keyboard model identifier
    oled_write_P(PSTR(" [Keyball 39] "), false);
    oled_advance_page(false);

    // Row 2: tapping term — adjustable at runtime via DYNAMIC_TAPPING_TERM
    oled_write_P(PSTR("Tap:"), false);
#ifdef DYNAMIC_TAPPING_TERM_ENABLE
    oled_write_num4(g_tapping_term);
#else
    oled_write_num4(TAPPING_TERM);
#endif
    oled_write_P(PSTR("ms"), false);
    oled_advance_page(false);

    // Row 3: RGB hue (changes per layer) + current brightness
#ifdef RGBLIGHT_ENABLE
    oled_write_P(PSTR("RGB h"), false);
    oled_write_num4(rgblight_get_hue());
    oled_write_P(PSTR(" v"), false);
    oled_write_num4(rgblight_get_val());
#else
    oled_write_P(PSTR("RGB: N/A       "), false);
#endif
    oled_advance_page(false);

    // Row 4: scroll divider + auto mouse layer activation timeout
    oled_write_P(PSTR("Div:"), false);
    oled_write_char('0' + keyball_get_scroll_div(), false);
#ifdef POINTING_DEVICE_AUTO_MOUSE_ENABLE
    oled_write_P(PSTR("  AML:"), false);
    oled_write_num4(keyball.auto_mouse_layer_timeout);
#endif
    oled_advance_page(false);
}
#endif