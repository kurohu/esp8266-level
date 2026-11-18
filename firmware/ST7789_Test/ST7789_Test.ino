#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>

constexpr uint8_t CS = D3;
constexpr uint8_t DC = D6;
constexpr uint8_t BUZZER = D0;
constexpr int dialRadius = 113;
constexpr int menuRadius = dialRadius - 3;
bool firstFrame = true;
bool menuOpen = false;
bool menuDirty = false;
enum class CalibrationPage { Closed, Choose, Face, Settling, Sampling, Retry, Result };
CalibrationPage calPage = CalibrationPage::Closed;
bool zeroSelected = false;
uint8_t face = 0;
const char* calResult = "";
struct Calibration {
  uint32_t marker;
  float bias[3];
  float matrix[3][3];
  int16_t edgeZero, flatXZero, flatYZero;
};
Calibration calibration = {0xca601502, {}, {{0,-1,0},{-1,0,0},{0,0,-1}}, 0, 0, 0};
constexpr int calibrationAddress = 8;
struct Capture {
  unsigned long started, last;
  uint8_t count;
  int32_t sum[3];
  int16_t low[3], high[3];
};
Capture capture{};
float faceSamples[6][3];
bool displayAsleep = false;
uint8_t menuItem = 0;
unsigned long lastActivity = 0;

struct Settings {
  uint8_t marker, sound, tolerance, snap, sleep;
};
constexpr uint8_t legacySettingsMarker = 0xa7;
constexpr uint8_t toleranceCount = 5;
Settings settings = {0xa8, 1, 4, 1, 1};
constexpr int tolerances[toleranceCount] = {0, 5, 10, 50, 100};
constexpr unsigned long sleepMinutes[] = {1, 3, 5, 0};

struct Button {
  uint8_t pin, active;
  bool raw = false, stable = false, held = false;
  unsigned long changed = 0, pressed = 0;

  uint8_t poll(unsigned long now) {
    const bool down = digitalRead(pin) == active;
    if (down != raw) {
      raw = down;
      changed = now;
    }
    if (raw != stable && now - changed >= 25) {
      stable = raw;
      if (!stable) return held ? 0 : 1;
      pressed = now;
      held = false;
    }
    if (stable && raw && !held && now - pressed >= 700) {
      held = true;
      return 2;
    }
    return 0;
  }
};

Button selectButton{D4, LOW};
Button confirmButton{D8, HIGH};
uint8_t selectEvent = 0;
uint8_t confirmEvent = 0;
bool holdRequested = false;
bool pinRequested = false;

void serviceBuzzer(unsigned long now);

void sampleButtons(unsigned long now) {
  serviceBuzzer(now);
  const uint8_t select = selectButton.poll(now);
  const uint8_t confirm = confirmButton.poll(now);
  if (select) selectEvent = select;
  if (confirm) confirmEvent = confirm;
}

unsigned long beepStarted = 0;
unsigned long beepLength = 0;
bool beeping = false;
bool arrivalSecondPending = false;

void beep(unsigned long now, unsigned long duration = 75);

void serviceBuzzer(unsigned long now) {
  if (beeping && now - beepStarted >= beepLength) {
    analogWrite(BUZZER, 0);
    digitalWrite(BUZZER, HIGH);
    beeping = false;
  }
  if (!settings.sound || menuOpen || displayAsleep) arrivalSecondPending = false;
  if (arrivalSecondPending && !beeping && now - beepStarted >= beepLength + 125) {
    arrivalSecondPending = false;
    beep(now, 200);
  }
}

void beep(unsigned long now, unsigned long duration) {
  if (settings.sound) {
    analogWrite(BUZZER, 128);
    beepStarted = now;
    beepLength = duration;
    beeping = true;
  }
}

void command(uint8_t opcode, const uint8_t* data = nullptr, size_t size = 0);

void command(uint8_t opcode, const uint8_t* data, size_t size) {
  digitalWrite(CS, LOW);
  digitalWrite(DC, LOW);
  SPI.transfer(opcode);
  digitalWrite(DC, HIGH);
  for (size_t i = 0; i < size; ++i) SPI.transfer(data[i]);
  digitalWrite(CS, HIGH);
}

  // Each glyph is five pixels wide, with one blank column between glyphs.
constexpr uint8_t glyphs[][7] = {
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14},
    {0,0,0,0,0,6,6}, {6,9,9,6,0,0,0},
    {0,0,0,14,0,0,0}, {0,0,0,0,0,0,0},
    {17,17,10,4,10,17,17}, {17,17,10,4,4,4,4}, {0,4,4,31,4,4,0}
};

constexpr uint8_t letters[][7] = {
  {14,17,17,31,17,17,17},
  {30,17,17,30,17,17,30},
  {14,17,16,16,16,17,14},
  {30,17,17,17,17,17,30},
  {31,16,16,30,16,16,31},
  {31,16,16,30,16,16,16},
  {14,17,16,23,17,17,15},
  {17,17,17,31,17,17,17},
  {14,4,4,4,4,4,14},
  {7,2,2,2,18,18,12},
  {17,18,20,24,20,18,17},
  {16,16,16,16,16,16,31},
  {17,27,21,21,17,17,17},
  {17,25,21,19,17,17,17},
  {14,17,17,17,17,17,14},
  {30,17,17,30,16,16,16},
  {14,17,17,17,21,18,13},
  {30,17,17,30,20,18,17},
  {15,16,16,14,1,1,30},
  {31,4,4,4,4,4,4},
  {17,17,17,17,17,17,14},
  {17,17,17,17,17,10,4},
  {17,17,17,21,21,21,10},
  {17,17,10,4,10,17,17},
  {17,17,10,4,4,4,4},
  {31,1,2,4,8,16,31},
};

uint8_t menuGlyph(char c, int row) {
  if (c >= 'A' && c <= 'Z') return letters[c - 'A'][row];
  if (c >= '0' && c <= '9') return glyphs[c - '0'][row];
  if (c == '.') return glyphs[10][row];
  if (c == '*') return glyphs[11][row];
  if (c == '~') {
    constexpr uint8_t plusMinus[] = {4,4,31,4,4,0,31};
    return plusMinus[row];
  }
  return 0;
}

constexpr uint32_t menuIcons[][24] = {
  {
    0x000000, 0x000000, 0x000000, 0x000000, 0x060000, 0x0e1000,
    0x181800, 0x381c00, 0x319f00, 0x739ff8, 0x671ff8, 0x661ff8,
    0x661ff8, 0x661ff8, 0x671ff8, 0x739ff8, 0x319f00, 0x381c00,
    0x181800, 0x0e1000, 0x060000, 0x000000, 0x000000, 0x000000,
  },
  {
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x010000,
    0x038000, 0x01c000, 0x00e000, 0x007000, 0x003800, 0x003c00,
    0x006c00, 0x00ee00, 0x00c700, 0x01c380, 0x0181c0, 0x0180e0,
    0x3ffff0, 0x3fffe0, 0x000000, 0x000000, 0x000000, 0x000000,
  },
  {
    0x000000, 0x000000, 0x003000, 0x003000, 0x003000, 0x003c00,
    0x00ff00, 0x01f380, 0x0300c0, 0x0300c0, 0x061860, 0x063c60,
    0x3f3cfc, 0x3f18fc, 0x0300c0, 0x0300c0, 0x01f380, 0x00ff00,
    0x003c00, 0x003000, 0x003000, 0x003000, 0x000000, 0x000000,
  },
  {
    0x000000, 0x000000, 0x000000, 0x000400, 0x000700, 0x000380,
    0x0003c0, 0x0003e0, 0x0003e0, 0x0003e0, 0x0003f0, 0x0007f0,
    0x0007f0, 0x000ff0, 0x001ff0, 0x107fe0, 0x1fffe0, 0x1fffe0,
    0x0fffc0, 0x07ff80, 0x03ff00, 0x00fc00, 0x000000, 0x000000,
  },
  {
    0x000000, 0x000000, 0x000000, 0x001800, 0x007e00, 0x01ff80,
    0x0399c0, 0x0718e0, 0x061860, 0x0c1830, 0x0c1830, 0x1ffff8,
    0x1ffff8, 0x0c1830, 0x0c1830, 0x061860, 0x0718e0, 0x0399c0,
    0x01ff80, 0x007e00, 0x001800, 0x000000, 0x000000, 0x000000,
  },
  {
    0x000000, 0x000000, 0x000200, 0x000700, 0x000380, 0x0001c0,
    0x0000e0, 0x000070, 0x01fff0, 0x03fff0, 0x0f00e0, 0x1c01c0,
    0x180380, 0x180700, 0x180200, 0x180000, 0x1c0000, 0x0e0000,
    0x07fe00, 0x03fe00, 0x000000, 0x000000, 0x000000, 0x000000,
  },
};

constexpr uint64_t calibrationCube[] = {
  0x000000000000ULL, 0x000001000000ULL, 0x000007c00000ULL, 0x00001ff00000ULL,
  0x00003c780000ULL, 0x0000f01e0000ULL, 0x0003e00f8000ULL, 0x000f8003e000ULL,
  0x003e0000f800ULL, 0x00f800003e00ULL, 0x01e000000f00ULL, 0x0780000003c0ULL,
  0x1f00000001f0ULL, 0x3c0000000078ULL, 0x3f00000001f8ULL, 0x3f80000003f8ULL,
  0x39e000000f38ULL, 0x38f800003e38ULL, 0x383e0000f838ULL, 0x380f8003e038ULL,
  0x3803e00f8038ULL, 0x3800f01e0038ULL, 0x38003c780038ULL, 0x38001ff00038ULL,
  0x380007c00038ULL, 0x380003800038ULL, 0x380003800038ULL, 0x380003800038ULL,
  0x380003800038ULL, 0x380003800038ULL, 0x380003800038ULL, 0x380003800038ULL,
  0x380003800038ULL, 0x380003800038ULL, 0x380003800038ULL, 0x3c0003800078ULL,
  0x1f00038001f0ULL, 0x0780038003c0ULL, 0x01e003800f00ULL, 0x00f803803e00ULL,
  0x003e0380f800ULL, 0x000f8383e000ULL, 0x0003e38f8000ULL, 0x0000f39e0000ULL,
  0x00003ff80000ULL, 0x00001ff00000ULL, 0x000007c00000ULL, 0x000001000000ULL,
};

constexpr uint64_t calibrationCubeFill[] = {
  0x000000000000ULL, 0x000000000000ULL, 0x000001000000ULL, 0x000007c00000ULL,
  0x00001ff00000ULL, 0x00003ff80000ULL, 0x0000fffe0000ULL, 0x0003ffff8000ULL,
  0x000fffffe000ULL, 0x003ffffff800ULL, 0x00fffffffe00ULL, 0x01ffffffff00ULL,
  0x07ffffffffc0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL,
  0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL,
  0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL,
  0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL,
  0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL,
  0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL, 0x1ffffffffff0ULL,
  0x07ffffffffc0ULL, 0x01ffffffff00ULL, 0x00fffffffe00ULL, 0x003ffffff800ULL,
  0x000fffffe000ULL, 0x0003ffff8000ULL, 0x0000fffe0000ULL, 0x00003ff80000ULL,
  0x00001ff00000ULL, 0x000007c00000ULL, 0x000001000000ULL, 0x000000000000ULL,
};

void showMenu() {
  const char* names[] = {"SOUND", "TOLERANCE", "SNAP", "SLEEP", "CAL", "EXIT"};
  const char* tolerance[toleranceCount] = {"0*", "~0.05*", "~0.1*", "~0.5*", "~1.0*"};
  const char* sleep[] = {"1 MIN", "3 MIN", "5 MIN", "NEVER"};
  const char* values[] = {settings.sound ? "ON" : "OFF", tolerance[settings.tolerance],
                          settings.snap ? "ON" : "OFF", sleep[settings.sleep],
                          "", ""};
  const bool calibrating = calPage != CalibrationPage::Closed;
  const char* title = names[menuItem];
  const char* value = values[menuItem];
  constexpr const char* faces[] = {"BOTTOM DOWN", "TOP DOWN", "LEFT DOWN",
                                   "RIGHT DOWN", "SCREEN UP", "SCREEN DOWN"};
  if (calibrating) {
    title = "CALIBRATION";
    switch (calPage) {
      case CalibrationPage::Choose: value = zeroSelected ? "ZERO CURRENT" : "SIX FACES"; break;
      case CalibrationPage::Face: value = faces[face]; break;
      case CalibrationPage::Settling: value = "SETTLING"; break;
      case CalibrationPage::Sampling: value = "HOLD STILL"; break;
      case CalibrationPage::Retry: value = calResult; break;
      case CalibrationPage::Result: value = calResult; break;
      default: break;
    }
  }
  const char* instruction = "";
  const char* controls = "";
  if (calPage == CalibrationPage::Choose) {
    instruction = zeroSelected ? "USE LEVEL SURFACE" : "SET ON SIX SIDES";
    controls = "DOWN NEXT UP GO";
  } else if (calPage == CalibrationPage::Face) {
    controls = "UP TO CAPTURE";
  } else if (calPage == CalibrationPage::Retry) {
    instruction = zeroSelected ? "USE LEVEL SURFACE" : faces[face];
    controls = "UP TO RETRY";
  } else if (calPage == CalibrationPage::Result) {
    controls = "UP TO CONTINUE";
  }
  const bool showProgress = calibrating && !zeroSelected && calPage != CalibrationPage::Choose;
  const int completed = face;
  const int titleY = value[0] ? -21 : -7;
  const int valueY = calibrating ? -17 : 0;
  const int valueScale = calibrating ? (strlen(value) <= 10 ? 3 : 2) :
      (strlen(value) > 5 ? 3 : 4);
  constexpr uint16_t selectedGreen = 0x6fd2;
  constexpr uint16_t sectorBlue = 0x11cd;
  constexpr uint16_t centerBlack = 0x0022;
  constexpr uint16_t calibrationBlue = 0x32bf;
  const bool calibrationError = calPage == CalibrationPage::Retry;
  const int centerRadius = calibrating ? menuRadius : 69;
  constexpr int directionX[] = {-87, 0, 87, 87, 0, -87};
  constexpr int directionY[] = {-50, -100, -50, 50, 100, 50};
  constexpr int iconX[] = {-77, 0, 77, 77, 0, -77};
  constexpr int iconY[] = {-45, -89, -45, 45, 89, 45};
  const uint8_t columns[] = {0, 0, 1, 63};
  const uint8_t rows[] = {0, 0, 0, 239};
  uint8_t pixels[640];
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  command(0x2a, columns, sizeof(columns));
  command(0x2b, rows, sizeof(rows));
  digitalWrite(CS, LOW);
  digitalWrite(DC, LOW);
  SPI.transfer(0x2c);
  digitalWrite(DC, HIGH);
  for (int y = 0; y < 240; ++y) {
    const int cy = y - 120;
    const bool titleRow = !calibrating && cy >= titleY && cy < titleY + 14;
    int textScale = titleRow ? 2 : valueScale;
    int textY = titleRow ? cy - titleY : cy - valueY;
    const char* label = titleRow ? title : value;
    uint16_t textColor = calibrationError ? 0xfcc8 : (titleRow ? 0xffff : selectedGreen);
    if (calibrating && cy >= 11) {
      const bool controlsRow = !instruction[0] || cy >= 34;
      label = controlsRow ? controls : instruction;
      textY = cy - (controlsRow && instruction[0] ? 34 : 11);
      textScale = 2;
      textColor = controlsRow ? 0xaf7f : 0xffff;
    }
    const int length = strlen(label);
    const int halfWidth = (length * 6 - 1) * textScale / 2;
    for (int x = 0; x < 320; ++x) {
      const int cx = x - 160;
      const int distance = cx * cx + cy * cy;
      uint16_t color = 0;
      if (distance <= centerRadius * centerRadius) {
        color = centerBlack;
        if (calibrating) {
          if (distance >= (menuRadius - 3) * (menuRadius - 3)) color = calibrationBlue;
          if (cx >= -24 && cx < 24 && cy >= -81 && cy < -33) {
            const int row = cy + 81;
            const uint64_t pixel = 1ULL << (cx + 24);
            if (calibrationCubeFill[row] & pixel) {
              const bool topFace = row * 20 <= 480 - 11 * abs(cx);
              color = topFace ? 0x447f : (cx < 0 ? 0x229b : 0x1952);
              if (topFace && calPage == CalibrationPage::Sampling) color = 0x27b1;
            }
            if (calibrationCube[row] & pixel) color = 0xcf5f;
          }
          if (showProgress && cy >= 49 && cy <= 61 && cx >= -61 && cx <= 61) {
            const int dot = (cx + 66) / 22;
            const int dx = cx - (22 * dot - 55);
            const int dy = cy - 55;
            if (dot < 6 && dx * dx + dy * dy <= 36) {
              color = dot < completed ? selectedGreen : sectorBlue;
              if (dot == completed) {
                color = dx * dx + dy * dy >= 16 ? calibrationBlue : centerBlack;
              }
            }
          }
        }
        const int tx = cx + halfWidth;
        if (length && textY >= 0 && textY < 7 * textScale &&
            tx >= 0 && tx < 2 * halfWidth) {
          const int column = (tx / textScale) % 6;
          if (column < 5 &&
              (menuGlyph(label[tx / (6 * textScale)], textY / textScale) & (16 >> column))) {
            color = textColor;
          }
        }
      } else if (distance <= menuRadius * menuRadius) {
        int best = -12000;
        int second = best;
        int sector = 0;
        for (int i = 0; i < 6; ++i) {
          const int projection = cx * directionX[i] + cy * directionY[i];
          if (projection > best) {
            second = best;
            best = projection;
            sector = i;
          } else if (projection > second) {
            second = projection;
          }
        }
        if (best - second > 350) {
          color = sector == menuItem ? selectedGreen : sectorBlue;
          const int ix = cx - iconX[sector] + 12;
          const int iy = cy - iconY[sector] + 12;
          if (ix >= 0 && ix < 24 && iy >= 0 && iy < 24 &&
              (menuIcons[sector][iy] & (1UL << ix))) {
            color = sector == menuItem ? 0x0842 : 0xffff;
          }
        }
      }
      pixels[x * 2] = color >> 8;
      pixels[x * 2 + 1] = color;
    }
    SPI.writeBytes(pixels, sizeof(pixels));
    sampleButtons(millis());
  }
  digitalWrite(CS, HIGH);
  SPI.endTransaction();
}

void setPanelAwake(bool on) {
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  command(on ? 0x29 : 0x28);
  SPI.endTransaction();
  displayAsleep = !on;
}

void handleButtons(unsigned long now) {
  sampleButtons(now);
  const uint8_t select = selectEvent;
  const uint8_t confirm = confirmEvent;
  selectEvent = confirmEvent = 0;
  if (!select && !confirm) return;
  lastActivity = now;
  if (displayAsleep) {
    setPanelAwake(true);
    return;
  }
  if (calPage != CalibrationPage::Closed) {
    if (confirm == 2) {
      calPage = CalibrationPage::Closed;
    } else if (calPage == CalibrationPage::Choose && select == 1) {
      zeroSelected = !zeroSelected;
    } else if (confirm == 1) {
      if (calPage == CalibrationPage::Choose && !zeroSelected) {
        face = 0;
        calPage = CalibrationPage::Face;
      } else if (calPage == CalibrationPage::Choose || calPage == CalibrationPage::Face ||
                 calPage == CalibrationPage::Retry) {
        capture = {};
        capture.started = now;
        calPage = CalibrationPage::Settling;
      } else if (calPage == CalibrationPage::Result) {
        calPage = CalibrationPage::Choose;
      }
    }
    menuDirty = true;
    return;
  }
  if (confirm == 2) {
    menuOpen = !menuOpen;
    menuDirty = menuOpen;
    firstFrame = !menuOpen;
    if (menuOpen) {
      menuItem = 0;

    }
  } else if (menuOpen && select == 1) {
    menuItem = (menuItem + 1) % 6;
    menuDirty = true;
  } else if (menuOpen && confirm == 1) {
    switch (menuItem) {
      case 0: settings.sound ^= 1; break;
      case 1: settings.tolerance = (settings.tolerance + 1) % toleranceCount; break;
      case 2: settings.snap ^= 1; break;
      case 3: settings.sleep = (settings.sleep + 1) % 4; break;
      case 4: zeroSelected = false; calPage = CalibrationPage::Choose; break;
      case 5: menuOpen = false; firstFrame = true; break;
    }
    if (menuItem < 4) {
      EEPROM.put(0, settings);
      EEPROM.commit();
    }
    menuDirty = menuOpen;
  } else {
    holdRequested = confirm == 1;
    pinRequested = select == 1;
    return;
  }
  Serial.printf("menu=%d item=%u sound=%u tolerance=%u snap=%u sleep=%u\n",
                menuOpen, menuItem, settings.sound, settings.tolerance,
                settings.snap, settings.sleep);
  beep(now);
}

void showAngle(int hundredths, int rollDegrees, bool flat, int xAngle, int yAngle,
               int batteryBars, int textDegrees, bool flatTextFlipped, bool held, bool pin, bool pinOnTarget) {
  constexpr int radius = dialRadius;
  constexpr int centerRadius = 75;
  constexpr int scale = 4;

  const int left = firstFrame ? 0 : 160 - radius;
  const int right = firstFrame ? 319 : 160 + radius;
  const int top = firstFrame ? 0 : 120 - radius;
  const int bottom = firstFrame ? 239 : 120 + radius;
  const uint8_t columns[] = {uint8_t(left >> 8), uint8_t(left),
                             uint8_t(right >> 8), uint8_t(right)};
  const uint8_t rows[] = {0, uint8_t(top), 0, uint8_t(bottom)};
  firstFrame = false;
  uint8_t pixels[640];
  uint8_t characters[] = {13, 12, 10, 12, 12, 11};
  if (hundredths >= 0) {
    characters[0] = hundredths >= 1000 ? hundredths / 1000 : 13;
    characters[1] = (hundredths / 100) % 10;
    characters[3] = (hundredths / 10) % 10;
    characters[4] = hundredths % 10;
  }
  uint8_t labels[2][8];
  const int angles[] = {xAngle, yAngle};
  for (int i = 0; i < 2; ++i) {
    const int magnitude = abs(angles[i]);
    const uint8_t label[] = {
      uint8_t(14 + i), uint8_t(angles[i] < 0 ? 12 : 16),
      uint8_t(magnitude >= 1000 ? magnitude / 1000 : 13),
      uint8_t(magnitude / 100 % 10), 10, uint8_t(magnitude / 10 % 10),
      uint8_t(magnitude % 10), 11
    };
    memcpy(labels[i], label, sizeof(label));
  }
  float bubbleX = xAngle * 0.10f;
  float bubbleY = -yAngle * 0.10f;
  const float travel = hypotf(bubbleX, bubbleY);
  if (travel > 57) {
    bubbleX *= 57 / travel;
    bubbleY *= 57 / travel;
  }
  const int bx = lroundf(bubbleX);
  const int by = lroundf(bubbleY);
  const int firstCharacter = characters[0] == 13 ? 1 : 0;
  const int textWidth = (6 - firstCharacter) * 6 * scale;
  const int textHeight = 7 * scale;
  const char* badge = held ? (pin ? "HOLD PIN" : "HOLD") : (pin ? "PIN" : "");
  constexpr int badgeScale = 2;
  const int badgeWidth = strlen(badge) * 6 * badgeScale;
  constexpr int badgeLeft = -81;
  constexpr int badgeTop = -60;
  const float roll = rollDegrees * (PI / 180.0f);
  const int lineX = lroundf(sinf(roll) * 256);
  const int lineY = lroundf(cosf(roll) * 256);
  const float textRadians = textDegrees * (PI / 180.0f);
  const int textSin = lroundf(sinf(textRadians) * 256);
  const int textCos = lroundf(cosf(textRadians) * 256);

  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  command(0x2a, columns, sizeof(columns));
  command(0x2b, rows, sizeof(rows));
  digitalWrite(CS, LOW);
  digitalWrite(DC, LOW);
  SPI.transfer(0x2c);
  digitalWrite(DC, HIGH);
  for (int y = top - 120; y <= bottom - 120; ++y) {
    for (int col = left; col <= right; ++col) {
      const int x = col - 160;
      const int distance = x * x + y * y;
      uint16_t color = 0x0000;
      if (distance <= centerRadius * centerRadius) {
        color = hundredths >= 0 || flat ? 0x2ed1 : 0x9ce7;
        if (pin) color = pinOnTarget ? 0x39f5 : 0x21ab;
        if (flat) {
          if (x == 0 || y == 0 || (distance >= 21 * 21 && distance <= 23 * 23)) {
            color = 0x1469;
          }
          const int bubbleDistance = (x - bx) * (x - bx) + (y - by) * (y - by);
          if (bubbleDistance <= 15 * 15) {
            color = bubbleDistance >= 12 * 12 ? 0x0842 : 0xcffb;
          }
        } else {
          const int textX = ((x * textCos + y * textSin + 128) >> 8) + textWidth / 2;
          const int textY = ((-x * textSin + y * textCos + 128) >> 8) + textHeight / 2;
          if (textY >= 0 && textY < textHeight && textX >= 0 && textX < textWidth) {
            const int pixelColumn = (textX / scale) % 6;
            const uint8_t glyph = characters[firstCharacter + textX / (6 * scale)];
            if (pixelColumn < 5 && (glyphs[glyph][textY / scale] & (16 >> pixelColumn))) {
              color = pin ? 0xffff : 0x0842;
            }
          }
        }
      } else if (distance <= radius * radius) {
        color = 0x1105;
        if (distance < 108 * 108) {
          const int ax = abs(x);
          const int ay = abs(y);
          // The ring changes color at 15, 45 and 75 degrees in each quadrant.
          const int diagonal = ax - ay;
          const int shallow = 256 * ay - 69 * ax;
          const int steep = 256 * ax - 69 * ay;
          if (abs(diagonal) > 2 && abs(shallow) > 430 && abs(steep) > 430) {
            const bool blue = (shallow < 0) || (diagonal < 0 && steep >= 0);
            color = blue ? 0x39f5 : 0x21ab;
          }
          const int across = abs(y * lineY - x * lineX);
          if (pin && !flat && hundredths >= 0) {
            // The outer marks reference the captured angle. The inner arcs
            // track the live deviation until the target is reached.
            if (distance >= 94 * 94 && abs(y) <= 2) color = pinOnTarget ? 0x5f75 : 0x2dff;
            if (!pinOnTarget && distance >= 79 * 79 && distance <= 87 * 87) {
              const int along = abs(x * lineY + y * lineX);
              if (across * 256 <= along * 46) color = 0xdf7f;
            }
          } else if ((flat || hundredths >= 0) && across <= 640) {
            color = 0x5f75;
          }
        }
      }
      if (flat) {
        constexpr int labelLeft = -47;
        constexpr int top[] = {-94, 81};
        for (int i = 0; i < 2; ++i) {
          const int tx = (flatTextFlipped ? -x : x) - labelLeft;
          const int ty = (flatTextFlipped ? -y : y) - top[i];
          if (tx >= 0 && tx < 96 && ty >= 0 && ty < 14) {
            const uint8_t glyph = labels[i][tx / 12];
            if ((tx / 2) % 6 < 5 && (glyphs[glyph][ty / 2] & (16 >> ((tx / 2) % 6)))) {
              color = 0xffff;
            }
          }
        }
      }
      if (badgeWidth && x >= badgeLeft - 3 && x < badgeLeft + badgeWidth + 2 &&
          y >= badgeTop - 3 && y < badgeTop + 7 * badgeScale + 3) {
        color = 0x0022;
      }
      if (badgeWidth && x >= badgeLeft && x < badgeLeft + badgeWidth &&
          y >= badgeTop && y < badgeTop + 7 * badgeScale) {
        const int bx = (x - badgeLeft) / badgeScale;
        const int by = (y - badgeTop) / badgeScale;
        if (bx % 6 < 5 && (menuGlyph(badge[bx / 6], by) & (16 >> (bx % 6)))) {
          color = 0xffff;
        }
      }
      if (batteryBars && x >= 62 && x <= 86 && y >= -64 && y <= -52) {
        const int bx = x - 62;
        const int by = y + 64;
        const uint16_t fill = batteryBars == 1 ? 0xfd20 : 0x6fd2;
        color = 0x0022;
        if (bx <= 21 && (bx == 0 || bx == 21 || by == 0 || by == 12)) color = 0xffff;
        if (bx >= 22 && by >= 4 && by <= 8) color = 0xffff;
        if (bx >= 3 && bx < 3 + 6 * batteryBars && (bx - 3) % 6 < 4 &&
            by >= 3 && by <= 9) color = fill;
      }
      pixels[2 * (col - left)] = color >> 8;
      pixels[2 * (col - left) + 1] = color & 0xff;
    }
    SPI.writeBytes(pixels, 2 * (right - left + 1));
    sampleButtons(millis());
  }
  digitalWrite(CS, HIGH);
  SPI.endTransaction();
}

constexpr uint8_t MPU = 0x68;

bool writeSensor(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readAcceleration(int16_t (&axes)[3]) {
  Wire.beginTransmission(MPU);
  Wire.write(0x3b);
  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(MPU, uint8_t(6)) != 6) return false;
  for (int16_t& axis : axes) {
    const uint16_t high = Wire.read();
    const uint16_t low = Wire.read();
    axis = static_cast<int16_t>((high << 8) | low);
  }
  return true;
}
void calibratedAxes(const float raw[3], float output[3]) {
  for (int axis = 0; axis < 3; ++axis) {
    output[axis] = 0;
    for (int input = 0; input < 3; ++input) {
      output[axis] += calibration.matrix[axis][input] * (raw[input] - calibration.bias[input]);
    }
  }
}

int angleHundredths(float rise, float base) {
  return lroundf(atan2f(rise, base) * (18000.0f / PI));
}

bool saveCalibration(const Calibration& updated) {
  EEPROM.put(calibrationAddress, updated);
  if (!EEPROM.commit()) {
    EEPROM.put(calibrationAddress, calibration);
    return false;
  }
  calibration = updated;
  return true;
}

bool fitSixFaces(Calibration& fitted) {
  // The columns describe the raw change along enclosure right, top and front.
  constexpr uint8_t positive[] = {2, 0, 4};
  constexpr uint8_t negative[] = {3, 1, 5};
  float columns[3][3];
  for (int axis = 0; axis < 3; ++axis) {
    for (int i = 0; i < 3; ++i) {
      const float plus = faceSamples[positive[axis]][i];
      const float minus = faceSamples[negative[axis]][i];
      if (axis == 0) fitted.bias[i] = 0;
      fitted.bias[i] += (plus + minus) / 6.0f;
      columns[axis][i] = (plus - minus) / 32768.0f;
    }
  }
  for (int axis = 0; axis < 3; ++axis) {
    const float* u = columns[(axis + 1) % 3];
    const float* v = columns[(axis + 2) % 3];
    float* inverse = fitted.matrix[axis];
    inverse[0] = u[1] * v[2] - u[2] * v[1];
    inverse[1] = u[2] * v[0] - u[0] * v[2];
    inverse[2] = u[0] * v[1] - u[1] * v[0];
    const float* w = columns[axis];
    const float determinant = w[0] * inverse[0] + w[1] * inverse[1] + w[2] * inverse[2];
    if (fabsf(determinant) < 0.65f || fabsf(determinant) > 1.45f) return false;
    for (int i = 0; i < 3; ++i) inverse[i] /= determinant;
  }
  // Reject repeated, non-opposing or substantially tilted captures.
  for (int axis = 0; axis < 3; ++axis) {
    for (int sign : {-1, 1}) {
      const float* raw = faceSamples[sign > 0 ? positive[axis] : negative[axis]];
      for (int j = 0; j < 3; ++j) {
        float actual = 0;
        for (int i = 0; i < 3; ++i) actual += fitted.matrix[j][i] * (raw[i] - fitted.bias[i]);
        if (fabsf(actual - (j == axis ? sign * 16384 : 0)) > 1800) return false;
      }
    }
  }
  fitted.edgeZero = fitted.flatXZero = fitted.flatYZero = 0;
  return true;
}

void finishCapture(const char* result, bool saved = false);

void finishCapture(const char* result, bool saved) {
  calResult = result;
  calPage = saved ? CalibrationPage::Result : CalibrationPage::Retry;
  menuDirty = true;
  Serial.printf("calibration: %s\n", result);
}

void processCapture(bool valid, const int16_t axes[3]) {
  const unsigned long now = millis();
  if (calPage == CalibrationPage::Settling) {
    if (now - capture.started < 2000) return;
    capture.started = now;
    calPage = CalibrationPage::Sampling;
    menuDirty = true;
  }
  if (calPage != CalibrationPage::Sampling) return;
  if (!valid || now - capture.started > 4000) {
    finishCapture("SENSOR ERROR");
    return;
  }
  if (capture.count && now - capture.last < 20) return;
  capture.last = now;
  for (int i = 0; i < 3; ++i) {
    if (!capture.count) capture.low[i] = capture.high[i] = axes[i];
    capture.low[i] = min(capture.low[i], axes[i]);
    capture.high[i] = max(capture.high[i], axes[i]);
    capture.sum[i] += axes[i];
    if (capture.high[i] - capture.low[i] > 1200) {
      finishCapture("MOVED RETRY");
      return;
    }
  }
  if (++capture.count < 60) return;
  float raw[3];
  for (int i = 0; i < 3; ++i) raw[i] = capture.sum[i] / 60.0f;
  const float magnitude = hypotf(hypotf(raw[0], raw[1]), raw[2]);
  if (magnitude < 12500 || magnitude > 20000) {
    finishCapture("FACE RETRY");
    return;
  }
  Calibration updated = calibration;
  if (zeroSelected) {
    float gravity[3];
    calibratedAxes(raw, gravity);
    const float right = gravity[0], top = gravity[1], front = gravity[2];
    if (front > 1.25f * hypotf(right, top)) {
      updated.flatXZero = angleHundredths(right, front);
      updated.flatYZero = angleHundredths(top, front);
    } else if (fabsf(top) > 1.25f * hypotf(right, front)) {
      updated.edgeZero = angleHundredths(right, fabsf(top));
    } else {
      finishCapture("SET LEVEL");
      return;
    }
  } else {
    memcpy(faceSamples[face], raw, sizeof(raw));
    if (face < 5) {
      ++face;
      calPage = CalibrationPage::Face;
      menuDirty = true;
      return;
    }
    if (!fitSixFaces(updated)) {
      finishCapture("FACE RETRY");
      return;
    }
  }
  const bool saved = saveCalibration(updated);
  if (saved && !zeroSelected) ++face;
  finishCapture(saved ? "SAVED" : "SAVE ERROR", saved);
}

void setup() {
  Serial.begin(115200);
  pinMode(D4, INPUT_PULLUP);
  pinMode(D8, INPUT);
  digitalWrite(BUZZER, HIGH);
  pinMode(BUZZER, OUTPUT);
  analogWriteRange(255);
  analogWriteFreq(1600);
  EEPROM.begin(calibrationAddress + sizeof(Calibration));
  Calibration savedCalibration;
  EEPROM.get(calibrationAddress, savedCalibration);
  if (savedCalibration.marker == calibration.marker) calibration = savedCalibration;
  Settings saved;
  EEPROM.get(0, saved);
  if (saved.marker == legacySettingsMarker && saved.tolerance < 3) {
    saved.marker = settings.marker;
    constexpr uint8_t legacyToleranceIndex[] = {3, 3, 4};
    saved.tolerance = legacyToleranceIndex[saved.tolerance];
  }
  if (saved.marker == settings.marker && saved.sound <= 1 && saved.tolerance < toleranceCount &&
      saved.snap <= 1 && saved.sleep < 4) settings = saved;
  lastActivity = millis();
  Wire.begin(D2, D1);
  Wire.setClock(100000);
  digitalWrite(CS, HIGH);
  pinMode(CS, OUTPUT);
  SPI.begin();
  // The display is write-only. Reclaim the default MISO pin for DC.
  pinMode(DC, OUTPUT);
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  command(0x01);
  delay(150);
  command(0x11);
  delay(120);
  const uint8_t pixelFormat = 0x55;
  const uint8_t orientation = 0xA0;  // MADCTL MY | MV, clockwise landscape.
  command(0x3a, &pixelFormat, 1);
  command(0x36, &orientation, 1);
  command(0x21);
  command(0x13);
  command(0x29);
  SPI.endTransaction();
}

void guideToLevel(bool valid, bool flat, int x, int y, int target, int tolerance) {
  static bool reference = false;
  static bool previousFlat = false;
  static bool moving = false;
  static bool targetLatched = false;
  static int previousTarget = 0;
  static int referenceX, referenceY;
  static unsigned long lastMove, lastPulse;
  constexpr int motionThreshold = 60; // In hundredths of a degree.
  const int guideRange = flat ? 1200 : 1000;
  const unsigned long now = millis();
  const int error = flat ? max(abs(x), abs(y)) : abs(abs(x) - target);

  if (!valid || displayAsleep || !settings.sound || menuOpen) {
    reference = false;
    return;
  }
  if (!reference || flat != previousFlat) {
    reference = true;
    previousFlat = flat;
    previousTarget = target;
    referenceX = x;
    referenceY = y;
    moving = false;
    targetLatched = error <= tolerance;
    return;
  }
  if (abs(x - referenceX) >= motionThreshold || abs(y - referenceY) >= motionThreshold) {
    referenceX = x;
    referenceY = y;
    if (!moving) lastPulse = now - 650;
    moving = true;
    lastMove = now;
  }

  if (target != previousTarget || error > tolerance + 30) targetLatched = false;
  previousTarget = target;
  if (moving && now - lastMove >= 1600) moving = false;
  if (error <= tolerance) {
    if (moving && !targetLatched && !arrivalSecondPending) {
      beep(now, 170);
      arrivalSecondPending = true;
    }
    targetLatched = true;
    return;
  }
  if (targetLatched || !moving || error > guideRange || beeping || arrivalSecondPending) return;

  const unsigned long interval = map(error, tolerance, guideRange, 125, 650);
  if (now - lastPulse >= interval) {
    beep(now, 35);
    lastPulse = now;
  }
}

int wrappedPinError(int angle, int target) {
  int difference = angle - target;
  if (difference > 18000) difference -= 36000;
  if (difference < -18000) difference += 36000;
  return difference;
}

void loop() {
  static bool hold = false;
  static bool pin = false;
  static bool pinFlat = false;
  static int pinX = 0, pinY = 0;
  static bool heldFlat = false, heldOnTarget = false;
  static int heldHundredths = -1, heldX = 0, heldY = 0, heldRoll = 0;
  static bool lastHold = false, lastPin = false, lastPinOnTarget = false;
  const unsigned long now = millis();
  handleButtons(now);
  static int16_t lastMotionAxes[3];
  static bool motionReference = false;
  static bool awake = false;
  static float filtered[3];
  static int lastShown = -2;
  static int lastRoll = 0;
  static int textDegrees = 0;
  static int lastTextDegrees = 0;
  static bool flatTextFlipped = false;
  static bool lastFlatTextFlipped = false;
  static int lastY = 0;
  static bool flat = false;
  static bool lastFlat = false;
  static unsigned long switchSince = 0;
  static unsigned long lastLog = 0;
  static unsigned long lastBatterySample = 0;
  static int batteryAverageQ8 = 0;
  static int batteryBars = 0;
  static int lastBatteryBars = -1;
  if (now - lastBatterySample >= 500) {
    lastBatterySample = now;
    const int reading = analogRead(A0);
    if (reading < 20 || reading > 55) {
      batteryAverageQ8 = 0;
      batteryBars = 0;
    } else {
      const int sampleQ8 = reading * 256;
      batteryAverageQ8 = batteryAverageQ8 ?
          (7 * batteryAverageQ8 + sampleQ8) / 8 : sampleQ8;
      // Estimated from 4.165 V at ADC 42. Recalibrate with a second measured point.
      constexpr int middle = 36 * 256;
      constexpr int high = 39 * 256;
      constexpr int hysteresis = 128;
      if (!batteryBars) {
        batteryBars = batteryAverageQ8 >= high ? 3 : (batteryAverageQ8 >= middle ? 2 : 1);
      } else if (batteryBars == 3 && batteryAverageQ8 < high - hysteresis) {
        batteryBars = 2;
      } else if (batteryBars == 2 && batteryAverageQ8 >= high + hysteresis) {
        batteryBars = 3;
      } else if (batteryBars == 2 && batteryAverageQ8 < middle - hysteresis) {
        batteryBars = 1;
      } else if (batteryBars == 1 && batteryAverageQ8 >= middle + hysteresis) {
        batteryBars = 2;
      }
    }
  }
  int16_t axes[3]{};
  const bool firstSample = !awake;
  if (firstSample) {
    awake = writeSensor(0x6b, 0x01) && writeSensor(0x1c, 0x00);
    if (awake) delay(100);
  }

  int hundredths = -1;
  int rollDegrees = 0;
  int xAngle = 0;
  int yAngle = 0;
  int fullEdgeAngle = 0;
  const bool valid = awake && readAcceleration(axes);
  processCapture(valid, axes);
  if (valid) {
    for (int i = 0; i < 3; ++i) {
      filtered[i] = firstSample ? axes[i] : 0.45f * filtered[i] + 0.55f * axes[i];
    }
    // Board header toward the bottom, component side toward the back.
    // Confirm the in-plane axis signs on the assembled unit.
    float gravity[3];
    calibratedAxes(filtered, gravity);
    const float right = gravity[0];
    const float top = gravity[1];
    fullEdgeAngle = angleHundredths(right, top);
    const float front = gravity[2];
    const float normal = fabsf(front);
    const float inPlane = hypotf(right, top);
    const bool wantsSwitch = flat ? normal < 0.8f * inPlane : front > 1.25f * inPlane;
    if (pin || pinRequested) {
      switchSince = 0;
    } else if (!wantsSwitch) {
      switchSince = 0;
    } else if (switchSince == 0) {
      switchSince = millis();
    } else if (millis() - switchSince >= 300) {
      flat = !flat;
      switchSince = 0;
    }
    if (!flat && inPlane > 0.8f * normal) {
      textDegrees = lroundf(atan2f(right, top) * (180.0f / PI));
      if (fabsf(top) > 0.25f * inPlane) flatTextFlipped = top < 0;
    }
    if (flat) {
      xAngle = constrain(angleHundredths(right, normal) - calibration.flatXZero, -9000, 9000);
      yAngle = constrain(angleHundredths(top, normal) - calibration.flatYZero, -9000, 9000);
    } else {
      xAngle = constrain(angleHundredths(right, fabsf(top)) - calibration.edgeZero, -9000, 9000);
      hundredths = abs(xAngle);
      rollDegrees = lroundf(-xAngle / 100.0f);
    }
  } else {
    awake = false;
    flat = pin && pinFlat;
    switchSince = 0;
  }

  if (menuOpen) {
    guideToLevel(false, false, 0, 0, 0, 0);
    if (menuDirty) {
      showMenu();
      menuDirty = false;
    }
    lastActivity = millis();
    delay(10);
    return;
  }

  if (awake) {
    bool moved = !motionReference;
    for (int i = 0; i < 3; ++i) {
      if (abs(axes[i] - lastMotionAxes[i]) > 1000) moved = true;
    }
    if (moved) {
      memcpy(lastMotionAxes, axes, sizeof(axes));
      motionReference = true;
      lastActivity = millis();
      if (displayAsleep) setPanelAwake(true);
    }
  }

  const bool changeHold = holdRequested;
  const bool changePin = pinRequested;
  holdRequested = pinRequested = false;
  if (changePin && (pin || valid)) {
    hold = false;
    pin = !pin;
    pinFlat = flat;
    pinX = flat ? xAngle : fullEdgeAngle;
    pinY = yAngle;
    guideToLevel(false, false, 0, 0, 0, 0);
    beep(now);
  }
  const int tolerance = tolerances[settings.tolerance];
  int guideX = xAngle;
  int guideY = yAngle;
  if (pin && valid && pinFlat == flat) {
    guideX = flat ? xAngle - pinX : wrappedPinError(fullEdgeAngle, pinX);
    guideY = flat ? yAngle - pinY : 0;
    if (!flat) rollDegrees = lroundf(-guideX / 100.0f);
  }
  const int target = !pin && abs(xAngle) > 4500 ? 9000 : 0;
  const bool onTarget = valid && (flat
      ? abs(guideX) <= tolerance && abs(guideY) <= tolerance
      : abs(abs(guideX) - target) <= tolerance);
  guideToLevel(valid && !hold && !changeHold, flat, guideX, guideY,
               flat ? 0 : target, tolerance);
  if (!pin && settings.snap && onTarget) {
    if (flat) {
      xAngle = 0;
      yAngle = 0;
    } else {
      hundredths = target;
      rollDegrees = target == 0 ? 0 : (xAngle < 0 ? 90 : -90);
    }
  }
  const unsigned long idleLimit = sleepMinutes[settings.sleep] * 60000UL;
  if (!displayAsleep && idleLimit && millis() - lastActivity >= idleLimit) {
    setPanelAwake(false);
  }
  if (changeHold && (hold || valid)) {
    hold = !hold;
    if (hold) {
      heldFlat = flat;
      heldOnTarget = onTarget;
      heldHundredths = hundredths;
      heldX = xAngle;
      heldY = yAngle;
      heldRoll = rollDegrees;
    }
    beep(now);
  }
  const bool frameFlat = hold ? heldFlat : flat;
  const int frameHundredths = hold ? heldHundredths : hundredths;
  const int frameX = hold ? heldX : xAngle;
  const int frameY = hold ? heldY : yAngle;
  const int frameRoll = hold ? heldRoll : rollDegrees;
  const bool pinOnTarget = pin && (hold ? heldOnTarget : onTarget);
  const int shown = frameFlat ? frameX : frameHundredths;
  if (!displayAsleep && (firstFrame || shown != lastShown || frameRoll != lastRoll ||
                         frameY != lastY || frameFlat != lastFlat || batteryBars != lastBatteryBars ||
                         hold != lastHold || pin != lastPin || pinOnTarget != lastPinOnTarget ||
                         (!frameFlat && textDegrees != lastTextDegrees) ||
                         (frameFlat && flatTextFlipped != lastFlatTextFlipped))) {
    showAngle(frameHundredths, frameRoll, frameFlat, frameX, frameY, batteryBars,
              textDegrees, flatTextFlipped, hold, pin, pinOnTarget);
    lastHold = hold;
    lastPin = pin;
    lastPinOnTarget = pinOnTarget;
    lastTextDegrees = textDegrees;
    lastFlatTextFlipped = flatTextFlipped;
    lastBatteryBars = batteryBars;
    lastShown = shown;
    lastY = frameY;
    lastFlat = frameFlat;
    lastRoll = frameRoll;
  }
  if (millis() - lastLog >= 500) {
    lastLog = millis();
    Serial.printf("BAT ADC=%.1f bars=%d\n", batteryAverageQ8 / 256.0, batteryBars);
    if (awake) {
      Serial.printf("X=%d Y=%d Z=%d mode=%s x=%+.2f y=%+.2f deg\n",
                    axes[0], axes[1], axes[2], flat ? "flat" : "edge", xAngle / 100.0, yAngle / 100.0);
    } else {
      Serial.println("MPU6050 read failed");
    }
  }
  delay(10);
}
