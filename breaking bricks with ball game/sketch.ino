#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define GREEN_BUTTON_PIN 2
#define RED_BUTTON_PIN 3
#define BLUE_BUTTON_PIN 4
#define PADDLE_POT_PIN A0

#define LED1_PIN 8
#define LED2_PIN 9
#define LED3_PIN 10

#define SEG_A 5
#define SEG_B 6
#define SEG_C 7
#define SEG_D 11
#define SEG_E 12
#define SEG_F 13
#define SEG_G A3

// Numara gösterimleri için segment değerleri (0-9)
int digits[10] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111  // 9
};

struct ExtraLife {
  bool active = false;
  int x, y;
} extraLife;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum GameState {
  MENU,
  PLAYING,
  EXITED
};

enum MenuSelection {
  START,
  EXIT
};

GameState gameState = MENU;
MenuSelection menuSelection = START;
int ballX = SCREEN_WIDTH / 2;
int ballY = SCREEN_HEIGHT / 2;
float ballDirX = 1.0;
float ballDirY = -1.0;
int paddleX = SCREEN_WIDTH / 2;
int paddleWidth = 20;
const int paddleY = SCREEN_HEIGHT - 10;
const int brickWidth = SCREEN_WIDTH / 10;
const int brickHeight = 5;
bool bricks[10][4];
int level = 1;
float speedMultiplier = 1.0;
int lives = 3;
int score = 0;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup() {
  pinMode(GREEN_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BLUE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PADDLE_POT_PIN, INPUT);
  pinMode(LED1_PIN, OUTPUT);
  digitalWrite(LED1_PIN, HIGH);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED2_PIN, HIGH);
  pinMode(LED3_PIN, OUTPUT);
  digitalWrite(LED3_PIN, HIGH);

  pinMode(SEG_A, OUTPUT);
  pinMode(SEG_B, OUTPUT);
  pinMode(SEG_C, OUTPUT);
  pinMode(SEG_D, OUTPUT);
  pinMode(SEG_E, OUTPUT);
  pinMode(SEG_F, OUTPUT);
  pinMode(SEG_G, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;); // Donanım hatası durumunda döngü
  }

  display.display();
  delay(2000);
  display.clearDisplay();
  resetGame();
  updateDisplay();
}

void loop() {
  int greenButtonState = digitalRead(GREEN_BUTTON_PIN);
  int redButtonState = digitalRead(RED_BUTTON_PIN);
  int blueButtonState = digitalRead(BLUE_BUTTON_PIN);

  if (millis() - lastDebounceTime > debounceDelay) {
    if (greenButtonState == LOW) {
      lastDebounceTime = millis();
      gameState = MENU;
      menuSelection = START;
    } else if (redButtonState == LOW) {
      lastDebounceTime = millis();
      if (gameState == MENU) {
        menuSelection = EXIT;
      }
    } else if (blueButtonState == LOW) {
      lastDebounceTime = millis();
      if (gameState == MENU && menuSelection == START) {
        gameState = PLAYING;
        resetGame();
      } else if (menuSelection == EXIT) {
        gameState = EXITED;
      }
    }
  }

  if (gameState == PLAYING) {
    moveBall();
    movePaddle();
    checkLevelCompletion();
    if (extraLife.active) {
      extraLife.y += 1;
      if (extraLife.y > SCREEN_HEIGHT) {
        extraLife.active = false;
      }
      if (extraLife.y >= paddleY && extraLife.x >= paddleX && extraLife.x <= paddleX + paddleWidth) {
        if (lives < 3) {
          lives++;
        }
        extraLife.active = false;
        updateLivesDisplay(false);
      }
    }

    if (lives <= 0) {
      gameOver();
    }
  } else if (gameState == MENU) {
    updateLivesDisplay(true);
  } else if (gameState == EXITED) {
    // EXITED durumunda özel bir işlem yok
  }

  updateDisplay();
}

void moveBall() {
  ballX += ballDirX * speedMultiplier;
  ballY += ballDirY * speedMultiplier;

  if (ballX <= 0 || ballX >= SCREEN_WIDTH) ballDirX *= -1;
  if (ballY <= 0) ballDirY *= -1;

  if (ballY >= SCREEN_HEIGHT) {
    lives--;
    updateLivesDisplay(false);
    if (lives > 0) {
      resetBallPosition();
    } else {
      gameState = EXITED;
    }
  }

  if (ballY == paddleY && ballX >= paddleX && ballX <= paddleX + paddleWidth) ballDirY *= -1;

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 4; j++) {
      if (bricks[i][j]) {
        if (ballX >= i * brickWidth && ballX <= (i + 1) * brickWidth &&
            ballY >= j * brickHeight && ballY <= (j + 1) * brickHeight) {
          bricks[i][j] = false;
          ballDirY *= -1;
          score += 1;
          updateScoreDisplay();

          if (!extraLife.active && random(10) < 1) {
            extraLife.active = true;
            extraLife.x = i * brickWidth + brickWidth / 2;
            extraLife.y = j * brickHeight;
          }
        }
      }
    }
  }
}

void movePaddle() {
  int potValue = analogRead(PADDLE_POT_PIN) / 4;
  paddleX = map(potValue, 0, 255, 0, SCREEN_WIDTH - paddleWidth);
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  switch (gameState) {
    case MENU:
      if (menuSelection == START) {
        display.drawRect(0, 0, 128, 16, SSD1306_WHITE);
      } else if (menuSelection == EXIT) {
        display.drawRect(0, 16, 128, 16, SSD1306_WHITE);
      }
      display.setCursor(0, 0);
      display.println(F("Baslat"));
      display.setCursor(0, 16);
      display.println(F("Cikis"));
      break;
    case PLAYING:
      drawGame();
      break;
    case EXITED:
      display.setCursor(0, 0);
      display.println(F("Oyunumuza gosterdiginiz ilgi icin tesekkurler"));
      break;
  }
  display.display();
}

void drawGame() {
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 4; j++) {
      if (bricks[i][j]) {
        display.drawRect(i * brickWidth, j * brickHeight, brickWidth, brickHeight, SSD1306_WHITE);
      }
    }
  }

  if (extraLife.active) {
    display.fillRect(extraLife.x - 2, extraLife.y - 2, 4, 4, SSD1306_WHITE);
  }
  display.fillRect(paddleX, paddleY, paddleWidth, 2, SSD1306_WHITE);
  display.fillCircle(ballX, ballY, 2, SSD1306_WHITE);
}

void updateLivesDisplay(bool forceOff) {
  digitalWrite(LED1_PIN, (lives >= 1 && !forceOff) ? HIGH : LOW);
  digitalWrite(LED2_PIN, (lives >= 2 && !forceOff) ? HIGH : LOW);
  digitalWrite(LED3_PIN, (lives >= 3 && !forceOff) ? HIGH : LOW);
}

void resetBallPosition() {
  ballX = paddleX + paddleWidth / 2;
  ballY = paddleY - 3;
  ballDirY = -1;
}

void resetGame() {
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;
  ballDirX = 1.0;
  ballDirY = -1.0;
  paddleX = SCREEN_WIDTH / 2;

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 4; j++) {
      bricks[i][j] = true;
    }
  }

  resetBallPosition();
  updateLivesDisplay(false);
}

void nextLevel() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  level++; // Seviye her durumda artırılacak
  speedMultiplier *= 1.2; // Hız çarpanı her seviye yükseldiğinde artırılıyor
  ballDirX = (ballDirX > 0 ? 1 : -1) * speedMultiplier; // Hız çarpanıyla topun X yönü hızı güncelleniyor
  ballDirY = (ballDirY > 0 ? 1 : -1) * speedMultiplier; // Hız çarpanıyla topun Y yönü hızı güncelleniyor

  if (level > 5) {
    display.println(F("Tebrikler! Oyunu bitirdiniz."));
    display.display();
    delay(5000);
    gameState = MENU;
    level = 1;
    speedMultiplier = 1.0;
  } else {
    display.println(F("Yeni bolum yukleniyor..."));
    display.display();
    delay(5000);
    resetGame();
    changeBrickPattern();
  }
}

void changeBrickPattern() {
for (int i = 0; i < 10; i++) {
for (int j = 0; j < 4; j++) {
bricks[i][j] = random(2);
}
}
}

void checkLevelCompletion() {
bool allBricksCleared = true;
for (int i = 0; i < 10; i++) {
for (int j = 0; j < 4; j++) {
if (bricks[i][j]) {
allBricksCleared = false;
break;
}
}
if (!allBricksCleared) break;
}

if (allBricksCleared) {
nextLevel();
}
}

void gameOver() {
display.clearDisplay();
display.setTextSize(1);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0, 0);
display.print(F("Skor: "));
display.println(score);
display.display();
delay(5000); // 5 saniye bekle
lives = 3; // Can sayısını sıfırla
score = 0; // Skoru sıfırla
level = 1; // Seviyeyi sıfırla
speedMultiplier = 1.0; // Hız çarpanını sıfırla
gameState = MENU;
menuSelection = START;
}

// Seven Segment Display'de skoru gösterme fonksiyonu

void updateScoreDisplay() {
    int digit = score % 10; // Sadece bir basamak gösterilecek
    int segments = digits[digit];

    digitalWrite(SEG_A, !(segments & 0b00000001));
    digitalWrite(SEG_B, !(segments & 0b00000010));
    digitalWrite(SEG_C, !(segments & 0b00000100));
    digitalWrite(SEG_D, !(segments & 0b00001000));
    digitalWrite(SEG_E, !(segments & 0b00010000));
    digitalWrite(SEG_F, !(segments & 0b00100000));
    digitalWrite(SEG_G, !(segments & 0b01000000));
}
