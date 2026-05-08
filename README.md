# Keyball39 カスタムキーマップ (`my_keymap`)

[Keyball39](https://shirogane-lab.net/items/64b8f8693ee3fd0045280190)（分割 + 39キー + トラックボール）向けに、日常使いを徹底的に最適化したカスタムファームウェアです。

本リポジトリは [Yowkees/keyball](https://github.com/Yowkees/keyball) のフォークです。カスタムキーマップは `qmk_firmware/keyboards/keyball/keyball39/keymaps/my_keymap/` に配置しています。

---

## 主な工夫

### 1. Auto Mouse Layer（AML）の大幅カスタマイズ

QMK 標準の AML をベースに、実用上の問題点を全て改善しています。

| 機能 | 標準AML | このキーマップ |
|------|---------|---------------|
| クリックでマウスレイヤーを抜けるか | 抜ける | **抜けない**（ドラッグ操作が自然に） |
| マウスレイヤーの離脱方法 | タイムアウト or キー入力 | **`ESC_ML`キーによる明示的離脱のみ** |
| 誤爆（軽い触れでAML発火）| 起きやすい | **累積移動量がしきい値(50)を超えるまで発火しない** |
| しきい値のリセット | なし | **500ms無動作でリセット（ゆっくりした繰り返し触れも防止）** |
| ESC_ML後の即再発火 | — | **600msクールダウンで再発火を抑制** |
| マウスレイヤーの保持時間 | 短い | **30秒（`AUTO_MOUSE_LAYER_KEEP_TIME 30000`）** |

**仕組みの概要：**
- トラックボールを動かすと Layer 1（マウスレイヤー）に自動遷移
- クリックしてもレイヤーは維持されるので、ドラッグや連続クリックがスムーズ
- `ESC_ML` キーを押すと強制的にマウスレイヤーを抜け、600ms は再発火しない
- `ESC_ML_LNG2` / `ESC_ML_LNG1` を押すと、マウスレイヤーを抜けつつ英数 / かなキーも同時送出

```c
// keymap.c より
enum my_keycodes {
    ESC_ML      = KEYBALL_SAFE_RANGE,  // マウスレイヤーを抜けるだけ
    ESC_ML_LNG2,                        // 抜ける + 英数（KC_LNG2）
    ESC_ML_LNG1,                        // 抜ける + かな（KC_LNG1）
};
```

LNG キーはレイヤー変更が完了してから送出する必要があるため、`process_record_user` 内でフラグを立て、`matrix_scan_user` で次のスキャンサイクルに送出する方式を採用しています。

---

### 2. レイヤー連動 RGB 発光

現在のアクティブレイヤーを LED の色で直感的に把握できます。輝度はそのまま維持しつつ色相だけを切り替えます。

| レイヤー | 色 | 用途 |
|---------|-----|------|
| 0 | 蛍光グリーン | デフォルト（QWERTY） |
| 1 | 蛍光シアン | マウス / AML |
| 2 | 蛍光イエロー | 編集 / ナビゲーション |
| 3 | 蛍光オレンジ | 設定 / スクロール |
| 4 | 蛍光ピンク | ファンクションキー |
| 5 | 蛍光パープル | 記号 / 数字 |

---

### 3. ホームポジション重視のレイヤー構成（7レイヤー）

親指キーのホールドでレイヤーに入るため、手を動かさずにほぼ全操作が完結します。

#### Layer 0 — デフォルト（QWERTY）

```
Q        W        E        R        T                Y        U        I        O        P
A        S        D        F        G                H        J        K        L        ;
Z        X        C        V        B                N        M        ,        .        /
LCtrl    LGui     LAlt     ⇧/英数   Spc/[5]  かな/[3]  BS       Ent/[2]  ⇧/英数   RAlt     RGui     RCtrl
```

- `⇧/英数`（`LSFT_T(KC_LNG2)`）: タップで英数キー、ホールドで Shift
- `Spc/[5]`（`LT(5,KC_SPC)`）: タップでスペース、ホールドで Layer 5（記号/数字）
- `かな/[3]`（`LT(3,KC_LNG1)`）: タップでかなキー、ホールドで Layer 3（設定/スクロール）
- `Ent/[2]`（`LT(2,KC_ENT)`）: タップでEnter、ホールドで Layer 2（編集/ナビゲーション）

#### Layer 1 — マウス / AML（トラックボール操作中に自動遷移）

```
Cmd+C    Cmd+V    Cmd+X    Del      [3]              —        BTN1     Alt+W    Esc      TG(0)
—        Ctrl+C   Ctrl+↑   Ctrl+V   Ctrl+←           Ctrl+→   BTN1     ↑        BTN2     TG(1)
—        Ctrl+X   —        —        Ctrl+↓           —        ←        ↓        →        TG(1)
LCtrl    —        —        英数脱出  [3]     かな脱出  —        [1]      —        RAlt     RGui     LCtrl
```

- クリック系（BTN1/BTN2）の操作後もこのレイヤーにとどまる
- `TG(0)` / `TG(1)` でトグル離脱
- `英数脱出`（`ESC_ML_LNG2`）: マウスレイヤーを抜けて英数モードへ
- `かな脱出`（`ESC_ML_LNG1`）: マウスレイヤーを抜けてかなモードへ

#### Layer 2 — 編集 / ナビゲーション（Enter ホールドで遷移）

```
Cmd+C    ↑        Cmd+V    Ctrl+PgUp Cmd+X           Alt+`    Cmd+W    Ctrl+PgDn Tab     Cmd+T
Cmd+F    Ctrl+←   Cmd+↓    Ctrl+→    Shift+W         Ctrl+英数 PgUp     英数       4       ⇧
Ctrl+C   Ctrl+↑   Ctrl+V   Ctrl+←    Cmd+[           PgDn     Ctrl+X   —          Cmd+⇧+5 Cmd+-
LCtrl    LGui     —        Del       [2]     BS       —        —        —          —       Cmd+0   —
```

#### Layer 3 — 設定 / スクロールモード（かなキー ホールドで遷移）

このレイヤーではトラックボールがスクロール操作に切り替わります。

```
RGB切替  AML切替  AML+50ms AML-50ms —                —        —        スナップ水平 スナップ垂直 スナップフリー
RGB順送  色相+    彩度+    輝度+    KBC保存           —        —        —        —        —
RGB逆送  色相-    彩度-    輝度-    —                CPI-1K   CPI-100  CPI+100  CPI+1K   KBC保存
QK_BOOT  KBC_RST  —        —        —        —        英数     かな      —        —        KBC_RST  QK_BOOT
```

#### Layer 4 — ファンクションキー

F1, F3, F4, F7, F8, F9, F11 を配置。

#### Layer 5 — 記号 / 数字（スペースキー ホールドで遷移）

```
!        @        #        $        %                ^        &        *        (        )
1        2        3        4        5                6        7        8        9        0
\        [        ]        -        =                `        '        "        ~        —
```

---

### 4. OLED 表示

キー情報・トラックボール情報・レイヤー情報をリアルタイム表示します。

```c
void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}
```

---

### 5. VIA / Remap 対応

`VIA_ENABLE = yes` が有効なので、[Remap](https://remap-keys.app/) でキーを GUI から書き換えられます。

カスタムキーコードは Remap の「SPECIAL」→「User」カテゴリに以下の番号で対応しています。

| User 番号 | キーコード | 動作 |
|-----------|-----------|------|
| User 0 | `ESC_ML` | マウスレイヤーを抜けるだけ |
| User 1 | `ESC_ML_LNG2` | マウスレイヤーを抜ける + 英数 |
| User 2 | `ESC_ML_LNG1` | マウスレイヤーを抜ける + かな |

---

## ビルド方法

QMK Firmware の環境構築後、以下のコマンドでビルドできます。

```bash
qmk compile -kb keyball/keyball39 -km my_keymap
```

または QMK MSYS / QMK CLI でビルド後、[QMK Toolbox](https://github.com/qmk/qmk_toolbox) でキーボードに書き込んでください。

---

## ファイル構成

```
qmk_firmware/keyboards/keyball/keyball39/keymaps/my_keymap/
├── keymap.c   # レイヤー定義・AML 処理・RGB 発光制御
├── config.h   # AML タイムアウト・レイヤー数などの設定
└── rules.mk   # RGBLIGHT / OLED / VIA の有効化
```

AML のコア実装（`auto_mouse_activation`, `keyball_handle_auto_mouse_layer_change`, `keyball_escape_mouse_layer`）は `qmk_firmware/keyboards/keyball/lib/keyball/keyball.c` に追加しています。

---

## クレジット

- 元リポジトリ: [Yowkees/keyball](https://github.com/Yowkees/keyball)
- ハードウェア設計: [@Yowkees](https://github.com/Yowkees)
- ファームウェア: [@kaoriya (KoRoN)](https://github.com/koron)
- カスタムキーマップ: Jun Uozumi

---

## オリジナルの Keyball シリーズについて

購入・ビルドガイドなど、ハードウェアに関する情報はオリジナルリポジトリを参照してください。

- [Keyball39 ビルドガイド（日本語）](./keyball39/doc/rev1/buildguide_jp.md)
- [Keyball39 Build Guide (English)](./keyball39/doc/rev1/buildguide_en.md)
- [遊舎工房（Yushakobo）](https://shop.yushakobo.jp/products/5357)
- [白金ラボ（Shirogane Lab）](https://shirogane-lab.net/items/64b8f8693ee3fd0045280190)
