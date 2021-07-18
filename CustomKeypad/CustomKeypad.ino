#include <Keypad.h>
#include <Keyboard.h>
#include <FastLED.h>
#include <EEPROM.h>

#define ENABLE_PIN 6
#define RED_PIN 9
#define GREEN_PIN 10
#define BLUE_PIN 11

#define BRIGHTNESS_LEVELS 4
#define MAX_BRIGHTNESS 32

#define EEPROM_INIT 0
#define EEPROM_BRIGHT 1
#define EEPROM_MODE 2

#define ROWS 4
#define COLS 3

char hexaKeys[ROWS][COLS] = {
  {'A','B','C'},
  {'7','8','9'},
  {'4','5','6'},
  {'1','2','3'}
};
byte rowPins[ROWS] = {3, 2, 1, 0};
byte colPins[COLS] = {13, 15, 14};

enum Mode {
  RAINBOW,
  RED,
  GREEN,
  BLUE,
  YELLOW,
  CYAN,
  PURPLE,
  WHITE,

  NUM_MODES
};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

uint8_t spectrum = 0;
uint8_t brightness = BRIGHTNESS_LEVELS - 1;
Mode mode = WHITE;

void setup(){
  pinMode(ENABLE_PIN, INPUT_PULLUP);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  uint8_t eeInit = EEPROM.read(EEPROM_INIT);
  if (eeInit != 1) {
    EEPROM.update(EEPROM_INIT, 1);
    save();
  } else {
    brightness = EEPROM.read(EEPROM_BRIGHT);
    mode = EEPROM.read(EEPROM_MODE);
  }
  
  Serial.begin(9600);
  Keyboard.begin();
}
  
void loop(){
  while (digitalRead(ENABLE_PIN) == HIGH) {
    Serial.println("wait");
    delay(500);
    return;
  }
  
  char key = customKeypad.getKey();
  
  if (key){
    Serial.println(key);

    switch (key) {
      case 'A':
        brightness = (brightness + 1) % BRIGHTNESS_LEVELS;
        save();
        break;

      case 'B':
        // unused
        break;

      case 'C':
        mode = (mode + 1) % NUM_MODES;
        save();
        break;

      default:
        Keyboard.press(KEY_LEFT_CTRL);
        Keyboard.press(KEY_LEFT_SHIFT);
        Keyboard.press(KEY_LEFT_ALT);

        switch (key) {
          case '1': Keyboard.press(KEY_F1); break;
          case '2': Keyboard.press(KEY_F2); break;
          case '3': Keyboard.press(KEY_F3); break;
          case '4': Keyboard.press(KEY_F5); break;
          case '5': Keyboard.press(KEY_F6); break;
          case '6': Keyboard.press(KEY_F7); break;
          case '7': Keyboard.press(KEY_F8); break;
          case '8': Keyboard.press(KEY_F9); break;
          case '9': Keyboard.press(KEY_F10); break;
        }
        
        delay(100);
        Keyboard.releaseAll();
        break;
    }
  }

  EVERY_N_MILLIS (100) {
    CRGB color;
    switch (mode) {
      case RED: color = CRGB::Red; break;
      case GREEN: color = CRGB::Green; break;
      case BLUE: color = CRGB::Blue; break;
      case YELLOW: color = CRGB::Yellow; break;
      case CYAN: color = CRGB::Cyan; break;
      case PURPLE: color = CRGB::Purple; break;
      case WHITE: color = CRGB::White; break;
      case RAINBOW: color = ColorFromPalette(RainbowColors_p, spectrum++, 255, LINEARBLEND); break;
    }

    color %= brightness * (MAX_BRIGHTNESS / (BRIGHTNESS_LEVELS-1));

    analogWrite(RED_PIN, color.r);
    analogWrite(GREEN_PIN, color.g);
    analogWrite(BLUE_PIN, color.b);
  }
}

void save() {
    EEPROM.update(EEPROM_BRIGHT, brightness);
    EEPROM.update(EEPROM_MODE, mode);
}

// https://github.com/FastLED/FastLED/issues/1014
CRGB ColorFromPaletteExtended(const CRGBPalette16 &pal, uint16_t index, uint8_t brightness, TBlendType blendType)
{
    // This fucnction has the same intuitive behavior as the other ColorFromPalette
    // functions, except it provides 8-bit interpolation between palette entries.

    // Extract the four most significant bits of the index as a palette index.
    uint8_t index_4bit = (index >> 12);

    // Calculate the 8-bit offset from the palette index.
    // Throws away the 4 least significant bits and uses the middle 8.
    uint8_t offset = (uint8_t)(index >> 4);

    // Get the palette entry from the 4-bit index
    //  CRGB rgb1 = pal[ hi4];
    const CRGB *entry = &(pal[0]) + index_4bit;
    uint8_t red1 = entry->red;
    uint8_t green1 = entry->green;
    uint8_t blue1 = entry->blue;

    uint8_t blend = offset && (blendType != NOBLEND);

    if (blend)
    {
        // If palette blending is enabled, use the offset to interpolate between
        // the selected palette entry and the next.

        if (index_4bit == 15)
        {
            entry = &(pal[0]);
        }
        else
        {
            entry++;
        }

        // Calculate the scaling factor and scaled values for the lower palette value.

        uint8_t f1 = 255 - offset;

        red1 = scale8_LEAVING_R1_DIRTY(red1, f1);
        green1 = scale8_LEAVING_R1_DIRTY(green1, f1);
        blue1 = scale8_LEAVING_R1_DIRTY(blue1, f1);

        // Calculate the scaled values for the neighboring palette value.
        uint8_t red2 = entry->red;
        uint8_t green2 = entry->green;
        uint8_t blue2 = entry->blue;
        red2 = scale8_LEAVING_R1_DIRTY(red2, offset);
        green2 = scale8_LEAVING_R1_DIRTY(green2, offset);
        blue2 = scale8_LEAVING_R1_DIRTY(blue2, offset);

        cleanup_R1();

        // This was overflowing so I've updated it using qadd8
        red1 = qadd8(red1, red2);
        green1 = qadd8(green1, green2);
        blue1 = qadd8(blue1, blue2);
    }

    if (brightness != 255)
    {
        nscale8x3_video(red1, green1, blue1, brightness);
    }

    return CRGB(red1, green1, blue1);
}
