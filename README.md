# CoreS3-Neopixel-easy-control

M5Stack CoreS3 のタッチUIで WS2812 系 NeoPixel ストリップの色と輝度を一括コントロールするツール。

## ハードウェア

| 項目 | 内容 |
|---|---|
| 制御機 | M5Stack CoreS3 (ESP32-S3, 320×240 IPS, 静電容量タッチ) |
| 出力ポート | Port A (内部で `port_a_scl` ピンをデータラインに使用) |
| 制御対象 | WS2812 / WS2812B 系 NeoPixel ストリップ × `LED_COUNT` 個 (デフォルト 60) |
| 給電 | Port A の 5V (`M5.Power.setExtOutput(true)` で供給ON) |
| 開発環境 | PlatformIO + Arduino framework |

## 機能仕様

- **RGB スライダ** (R / G / B) で色を 0–255 で調整
- **輝度 (L) スライダ** で全LED共通の明るさを 0–255 で調整
- **ON/OFF トグルボタン** で点灯/消灯を切替。ON時のみ設定が出力に反映される
- **額縁インジケータ**: 画面周囲 14px の枠領域が現在の RGB×輝度 を表示。スライダ操作中にも追従するので、点灯前に最終色を確認できる
- **ちらつき抑制描画**: PSRAM 上のフルスクリーン `M5Canvas` に描画 → 1回の `pushSprite` で表示。状態が変化したフレームだけ再描画

## UI レイアウト

実際の表示と等寸 (320×240) のモックアップ。額縁色はサンプル値 R=255 G=128 B=0 L=200 を反映した状態。

<p align="center">
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 320 240" width="640" height="480">
  <rect width="320" height="240" fill="#000"/>
  <g fill="#FF6400">
    <rect x="0" y="0" width="320" height="14"/>
    <rect x="0" y="226" width="320" height="14"/>
    <rect x="0" y="0" width="14" height="240"/>
    <rect x="306" y="0" width="14" height="240"/>
  </g>
  <text x="20" y="32" font-family="sans-serif" font-size="14" font-weight="bold" fill="#fff">NeoPixel</text>
  <text x="20" y="46" font-family="sans-serif" font-size="8" fill="#fff">OUTPUT: OFF</text>
  <text x="20" y="68" font-family="sans-serif" font-size="14" fill="#fff">R</text>
  <rect x="42" y="52" width="208" height="24" rx="6" fill="#212121"/>
  <rect x="42" y="52" width="208" height="24" rx="6" fill="#F00"/>
  <rect x="42" y="52" width="208" height="24" rx="6" fill="none" stroke="#fff"/>
  <circle cx="246" cy="64" r="14" fill="#fff" stroke="#000"/>
  <text x="300" y="68" font-family="sans-serif" font-size="14" fill="#fff" text-anchor="end">255</text>
  <text x="20" y="98" font-family="sans-serif" font-size="14" fill="#fff">G</text>
  <rect x="42" y="82" width="208" height="24" rx="6" fill="#212121"/>
  <rect x="42" y="82" width="104" height="24" rx="6" fill="#0F0"/>
  <rect x="42" y="82" width="208" height="24" rx="6" fill="none" stroke="#fff"/>
  <circle cx="146" cy="94" r="14" fill="#fff" stroke="#000"/>
  <text x="300" y="98" font-family="sans-serif" font-size="14" fill="#fff" text-anchor="end">128</text>
  <text x="20" y="128" font-family="sans-serif" font-size="14" fill="#fff">B</text>
  <rect x="42" y="112" width="208" height="24" rx="6" fill="#212121"/>
  <rect x="42" y="112" width="208" height="24" rx="6" fill="none" stroke="#fff"/>
  <circle cx="46" cy="124" r="14" fill="#fff" stroke="#000"/>
  <text x="300" y="128" font-family="sans-serif" font-size="14" fill="#fff" text-anchor="end">0</text>
  <text x="20" y="158" font-family="sans-serif" font-size="14" fill="#fff">L</text>
  <rect x="42" y="142" width="208" height="24" rx="6" fill="#212121"/>
  <rect x="42" y="142" width="163" height="24" rx="6" fill="#fff"/>
  <rect x="42" y="142" width="208" height="24" rx="6" fill="none" stroke="#fff"/>
  <circle cx="205" cy="154" r="14" fill="#fff" stroke="#000"/>
  <text x="300" y="158" font-family="sans-serif" font-size="14" fill="#fff" text-anchor="end">200</text>
  <rect x="204" y="188" width="96" height="32" rx="8" fill="#393939"/>
  <rect x="204" y="188" width="96" height="32" rx="8" fill="none" stroke="#fff"/>
  <text x="252" y="210" font-family="sans-serif" font-size="14" font-weight="bold" fill="#fff" text-anchor="middle">OFF</text>
</svg>
</p>

| 領域 | 配置 | 役割 |
|---|---|---|
| 額縁インジケータ | 画面外周 14px | 現在の RGB×輝度 を表示 |
| タイトル | 左上 | 製品名 + OUTPUT 状態テキスト |
| R/G/B/L スライダ | 中央4段 | 各値を 0–255 で調整。トラック塗りつぶし量で値を視覚化、白丸ノブ付き |
| ON/OFF ボタン | 右下 | LED 出力の有効化トグル。ON時は緑、OFF時はグレー |

## 操作

- スライダはタップした時点で「対象スライダ」がロックされ、押下したまま左右にドラッグして値変更
- 指を離すとロック解除。隣のスライダに移るには一度離して再タップ
- ON/OFF ボタンは押した瞬間にトグル

## 配線

```
CoreS3 Port A         WS2812 strip
  5V    (red)    ---> VCC (+5V)
  GND   (black)  ---> GND
  SCL   (yellow) ---> DIN
```

CoreS3 の Port A 出力は 3.3V↔5V のレベルシフタ経由なので、長尺ストリップで信号が崩れる場合は外部に専用バッファ (74HCT245 等) の挿入を推奨。

## ビルド / 書き込み

```powershell
pio run                 # ビルド
pio run -t upload       # 書き込み (USB シリアル自動検出)
pio device monitor      # シリアルモニタ
```

`platformio.ini` で `upload_port` は未指定 = 自動検出。CoreS3 を USB-C で接続すると `m5stack-cores3` ボード定義の HWID (VID `0x303A` / PID `0x1001`) に一致する USB JTAG/Serial ポートが選ばれる。

## 設定パラメータ

`src/main.cpp` 冒頭の定数で調整:

| 定数 | デフォルト | 説明 |
|---|---|---|
| `LED_COUNT` | 60 | 制御する NeoPixel の個数 |
| `FRAME_W` | 14 | 額縁インジケータの太さ (px) |
| `SCREEN_W` / `SCREEN_H` | 320 / 240 | 画面解像度 |
