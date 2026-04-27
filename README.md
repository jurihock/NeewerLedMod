# Neewer LED panel modification

Homemade Arduino Nano based driver for the [Neewer BP300](https://neewer.com/products/neewer-bp300-2-pack-led-video-light-panel-lighting-kit-66606584) LED panel.

## Features

- Single controller to sync two LED panels
- Color balance between cool and warm
- Brightness balance between two LED panels
- Total brightness adjustment

## Implementation

- Basically PWM followed by the the IRLZ44N logic-level MOSFET
- Utilizing timers 1 and 2 of the [ATmega328](https://en.wikipedia.org/wiki/ATmega328) chip to generate 2x2 PWM signals up to ~60kHz common frequency and individual duty cycles

### See also

- [The MOSFET as Switch](https://wolles-elektronikkiste.de/en/the-mosfet-as-switch)

## Details

### PSU

- [LEICKE ULL 5V 8A](https://www.leicke.de/shop-artikel/buero/leicke-ull-netzteil-40w-5v-8a_106522_67199) or [6A](https://www.leicke.de/shop-artikel/buero/leicke-ull-netzteil-30w-5v-6a_106544_67223)
- Connector 5.5x2.5mm (+)
- Actually 5.5V **not** 5V
- [Meanwell GST60A05-P1J 5V 6A](https://www.welectron.com/Meanwell-GST60A05-P1J-Desktop-Power-Supply-5V-6A) as possible alternative

### MCU

- [Arduino Nano compatible clone](https://www.christians-shop.de/nano-v3-0-atmega328p-development-board-without-soldering-christians-technikshop)
- VIN between 5V and 12V
- [Pololu S7V8F5](https://www.pololu.com/product/2123) as possible extension

### Wiring

- Extension cable [LiYY 3x0.34mm²](https://www.ebay.de/itm/325896583886)
- Cable connector [M13 5A](https://www.ebay.de/itm/323694504483)

| Label  | Native wire | Extension wire | Connector pin |
|--------|-------------|----------------|---------------|
| W-     | Yellow      | Green          | 1             |
| C-     | Black       | White          | 2             |
| + (5V) | Red         | Brown          | 3             |

### LED indicators

| Color | Resistor |
|-------|----------|
| G     | 470      |
| R     | 2k       |
| B     | 10k      |

### Pots

- 3x10k

### Fuses

- 2x3A slow or fast

### Housing

- [TEKO 362](https://www.teko.it/en/products/family/PU/series/pult-36) or similar
