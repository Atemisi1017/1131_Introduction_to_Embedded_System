#include <Wire.h>
#include <U8g2lib.h>

// 定義蜂鳴器和PWM通道
#define BUZZER_PIN 16
#define LEDC_CHANNEL 0
#define LEDC_TIMER 0

// 定義按鈕引腳
#define BUTTON_PIN 32
#define PAUSE_BUTTON_PIN 33  // 暫停按鈕
#define SPEED_BUTTON_PIN 27  // 加速按鈕

// 定義 8 個獨立 LED 引腳
const int LED_PINS[] = {25, 26, 18, 19, 13, 23, 17, 2};  // 請根據實際連接的引腳調整
#define NUM_LEDS 8

// OLED螢幕設定
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// 定義音符
#define NOTE_C4 262  // Do
#define NOTE_D4 294  // Re
#define NOTE_E4 330  // Mi
#define NOTE_F4 349  // Fa
#define NOTE_G4 392  // So
#define NOTE_A4 440  // La
#define NOTE_B4 494  // Ti
#define NOTE_C5 523  // High Do
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_B5 988
#define NOTE_REST 0  // 休止符

float speedFactor = 1.5; // 播放速度調整因子

// Jingle Bells - Complete version with correct pitch
const int jingleBellsNotes[] = {
    // First verse
    NOTE_E4, NOTE_E4, NOTE_E4,
    NOTE_E4, NOTE_E4, NOTE_E4,
    NOTE_E4, NOTE_G4, NOTE_C4, NOTE_D4,
    NOTE_E4,

    // Second part
    NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4,
    NOTE_F4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4,
    NOTE_E4, NOTE_D4, NOTE_D4, NOTE_E4, NOTE_D4,
    NOTE_G4,

    // Repeat first part
    NOTE_E4, NOTE_E4, NOTE_E4,
    NOTE_E4, NOTE_E4, NOTE_E4,
    NOTE_E4, NOTE_G4, NOTE_C4, NOTE_D4,
    NOTE_E4
};

const int jingleBellsDurations[] = {
    // First verse
    250, 250, 500,
    250, 250, 500,
    250, 250, 250, 250,
    1000,

    // Second part
    250, 250, 250, 250,
    250, 250, 250, 250, 250,
    250, 250, 250, 500, 500,
    1000,

    // Repeat first part
    250, 250, 500,
    250, 250, 500,
    250, 250, 250, 250,
    1000
};

// Santa Claus Is Coming To Town - Complete version with correct pitch
const int santaClausNotes[] = {
    // First phrase
    NOTE_G4, NOTE_E4, NOTE_F4, NOTE_G4,
    NOTE_G4, NOTE_G4, NOTE_A4, NOTE_G4,
    NOTE_F4, NOTE_E4, NOTE_D4,

    // Second phrase
    NOTE_G4, NOTE_G4, NOTE_G4,
    NOTE_A4, NOTE_G4, NOTE_F4,
    NOTE_E4, NOTE_D4,

    // Final phrase
    NOTE_G4, NOTE_A4, NOTE_B4,
    NOTE_C5, NOTE_G4, NOTE_E4,
    NOTE_D4, NOTE_C4
};

const int santaClausDurations[] = {
    // First phrase
    250, 250, 250, 500,
    250, 250, 500, 250,
    250, 250, 1000,

    // Second phrase
    250, 250, 250,
    500, 250, 250,
    250, 1000,

    // Final phrase
    250, 250, 500,
    250, 250, 500,
    250, 1000
};

// We Wish You a Merry Christmas
const int merryChristmasNotes[] = {
  NOTE_G4, NOTE_C5, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_B4, NOTE_A4, NOTE_A4,
  NOTE_D5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_B4, NOTE_B4, NOTE_B4, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5,
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_D5,
  NOTE_G4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5, NOTE_A4, NOTE_C5, NOTE_D5, NOTE_E5
};

const int merryChristmasDurations[] = {
  240, 240, 240, 240, 240, 240, 480, 480,
  240, 240, 240, 240, 240, 240, 480, 480, 240, 240, 240, 240, 240, 480,
  240, 240, 240, 240, 240, 240, 240, 240, 480,
  240, 240, 240, 240, 240, 240, 240, 240, 480
};

// 歌曲列表
const int* songs[] = {jingleBellsNotes, santaClausNotes, merryChristmasNotes};
const int* durations[] = {jingleBellsDurations, santaClausDurations, merryChristmasDurations};
const int songLengths[] = {
  sizeof(jingleBellsNotes) / sizeof(int),
  sizeof(santaClausNotes) / sizeof(int),
  sizeof(merryChristmasNotes) / sizeof(int)
};
const int totalSongs = 3;

int currentSong = 2;
int songIndex = 0;
unsigned long songTimer = 0;
bool isPaused = false;  // 暫停狀態變數

void setup() {
  Serial.begin(115200);
  
  // 初始化OLED
  u8g2.begin();
  u8g2.setFont(u8g2_font_michaelmouse_tu);
  
  // 初始化蜂鳴器
  ledcSetup(LEDC_CHANNEL, 5000, 7);
  ledcAttachPin(BUZZER_PIN, LEDC_CHANNEL);
  
  // 初始化按鈕
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PAUSE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPEED_BUTTON_PIN, INPUT_PULLUP);
  
  // 初始化 LED 引腳
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // 處理切換歌曲按鈕
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(300);
    currentSong = (currentSong + 1) % totalSongs;
    songIndex = 0;
    songTimer = currentMillis;
    ledcWriteTone(LEDC_CHANNEL, 0);
    
    // 更新OLED顯示新歌曲
    u8g2.firstPage();
    do {
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.setCursor(0, 10);
        switch(currentSong) {
          case 0: u8g2.print("Jingle Bells"); break;
          case 1: u8g2.print("Santa Claus"); break;
          case 2: u8g2.print("Merry Xmas"); break;
        }
        u8g2.setFont(u8g2_font_inb24_mr); 
        u8g2.setCursor(0, 50);
        u8g2.print("Switch");
    } while (u8g2.nextPage());
  }

  // 處理播放/暫停按鈕
  if (digitalRead(PAUSE_BUTTON_PIN) == LOW) {
    delay(300);
    isPaused = !isPaused;
    if (isPaused) {
      ledcWriteTone(LEDC_CHANNEL, 0);
      turnOffLeds();
      
      // 更新OLED顯示暫停狀態
      u8g2.firstPage();
      do {
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.setCursor(0, 10);
        switch(currentSong) {
          case 0: u8g2.print("Jingle Bells"); break;
          case 1: u8g2.print("Santa Claus"); break;
          case 2: u8g2.print("Merry Xmas"); break;
        }
        u8g2.setFont(u8g2_font_inb24_mr); 
        u8g2.setCursor(0, 50);
        u8g2.print("Paused");
      } while (u8g2.nextPage());
    }
  }

  // 處理加速按鈕
  if (digitalRead(SPEED_BUTTON_PIN) == LOW) {
    delay(300);
    speedFactor *= 0.75;
    if (speedFactor < 0.5) speedFactor = 1.5;
  }

  // 如果未暫停，繼續播放歌曲
  if (!isPaused && (currentMillis - songTimer >= durations[currentSong][songIndex] * speedFactor)) {
    int note = songs[currentSong][songIndex];
    playNoteAndLeds(note);
    songTimer = currentMillis;
    songIndex = (songIndex + 1) % songLengths[currentSong];
  }
}

void playNoteAndLeds(int note) {
  if (note > 0) {
    ledcWriteTone(LEDC_CHANNEL, note);
    changeLedsByNote(note);
    
    // 更新OLED顯示
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.setCursor(0, 10);
      switch(currentSong) {
        case 0: u8g2.print("Jingle Bells"); break;
        case 1: u8g2.print("Santa Claus"); break;
        case 2: u8g2.print("Merry Xmas"); break;
      }
      
      u8g2.setFont(u8g2_font_inb24_mr);
      
      if (note >= NOTE_C5) {
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.setCursor(35, 35);
        u8g2.print("High");
        u8g2.setFont(u8g2_font_inb24_mr);
      }
      
      u8g2.setCursor(45, 50);
      switch(note) {
        case NOTE_C4: u8g2.print("Do"); break;
        case NOTE_D4: u8g2.print("Re"); break;
        case NOTE_E4: u8g2.print("Mi"); break;
        case NOTE_F4: u8g2.print("Fa"); break;
        case NOTE_G4: u8g2.print("So"); break;
        case NOTE_A4: u8g2.print("La"); break;
        case NOTE_B4: u8g2.print("Ti"); break;
        case NOTE_C5: u8g2.print("Do"); break;
        case NOTE_D5: u8g2.print("Re"); break;
        case NOTE_E5: u8g2.print("Mi"); break;
        case NOTE_F5: u8g2.print("Fa"); break;
        case NOTE_G5: u8g2.print("So"); break;
        case NOTE_A5: u8g2.print("La"); break;
        case NOTE_B5: u8g2.print("Ti"); break;
        default: u8g2.print("-"); break;
      }
    } while (u8g2.nextPage());
  } else {
    ledcWriteTone(LEDC_CHANNEL, 0);
    turnOffLeds();
    
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_inb24_mr);
      u8g2.setCursor(20, 40);
      u8g2.print("Pause");
    } while (u8g2.nextPage());
  }
}

void changeLedsByNote(int note) {
  bool pattern1, pattern2;

  // 根據音符範圍決定 LED 閃爍模式
  switch(note) {
    case NOTE_C4:  // Do
      pattern1 = HIGH;
      pattern2 = LOW;
      break;
    case NOTE_D4:  // Re
      pattern1 = LOW;
      pattern2 = HIGH;
      break;
    case NOTE_E4:  // Mi
      pattern1 = HIGH;
      pattern2 = HIGH;
      break;
    case NOTE_F4:  // Fa
      pattern1 = HIGH;
      pattern2 = LOW;
      break;
    case NOTE_G4:  // So
      pattern1 = LOW;
      pattern2 = HIGH;
      break;
    case NOTE_A4:  // La
      pattern1 = HIGH;
      pattern2 = HIGH;
      break;
    case NOTE_B4:  // Ti
      pattern1 = HIGH;
      pattern2 = LOW;
      break;
    case NOTE_C5:  // High Do
      pattern1 = LOW;
      pattern2 = HIGH;
      break;
    case NOTE_D5:  // High Re
      pattern1 = HIGH;
      pattern2 = HIGH;
      break;
    case NOTE_E5:  // High Mi
      pattern1 = HIGH;
      pattern2 = LOW;
      break;
    case NOTE_F5:  // High Fa
      pattern1 = LOW;
      pattern2 = HIGH;
      break;
    case NOTE_G5:  // High So
      pattern1 = HIGH;
      pattern2 = HIGH;
      break;
    case NOTE_A5:  // High La
      pattern1 = HIGH;
      pattern2 = LOW;
      break;
    case NOTE_B5:  // High Ti
      pattern1 = LOW;
      pattern2 = HIGH;
      break;
    default:  // 休止符
      pattern1 = LOW;
      pattern2 = LOW;
      break;
  }

  // 設置 LED 狀態
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i % 2 == 0) {
      digitalWrite(LED_PINS[i], pattern1);
    } else {
      digitalWrite(LED_PINS[i], pattern2);
    }
  }
}

void turnOffLeds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LED_PINS[i], LOW);
  }
}
