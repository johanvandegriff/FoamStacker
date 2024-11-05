/*
   This sketch is for the ArduinOLED board. for more info, visit:
   https://jjv.sh/ArduinOLED

   However, I am running it on a different board, just using the library, more info here:
   https://jjv.sh/FoamStacker
*/

#include <Arduino.h>
#include <U8g2lib.h> //library to control the display in graphics mode
#include <pitches.h> //list of pitches for the buzzer
#include <ArduinOLED.h> //library to read the buttons and joysticks
#include <EEPROM.h>

#define STACKER_HIGHSCORE_EEPROM_ADDR 0
#define LED_PIN 6
//#define BUTTON_PIN 2 //see isButtonPressed() below for more info

bool isButtonPressed() {
  //note: the ArduinOLED library expects a button matrix,
  //but I just attached a button between pin 2 and GND instead,
  //and when it's pressed it shows up as multiple buttons, including BTN_D
  return ArduinOLED.isPressed(BTN_D);
}
void waitForButtonPress() {
  while(!isButtonPressed()) {
//    delay(1);
  }
}
void waitForButtonRelease() {
  while(isButtonPressed()) {
//    delay(1);
  }
}


//set up the display in page mode, which will draw part of the screen at a time
//https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
//there is a 128 byte (6.25% of 2048 bytes) difference between the 128 and 256 byte page sizes
//there is a 768 byte (37.5% of 2048 bytes) difference between the 256 and 1024 byte page sizes
//there is a 896 byte (43.75% of 2048 bytes) difference between the 128 and 1024 byte page sizes
//U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //128 byte page size
//U8G2_SSD1306_128X64_NONAME_2_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //256 byte page size
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //1024 byte page size (full page)

//This updates the display multiple times to display the whole image
void render(void (*f)(void)) {
  display.firstPage();
  do {
    f();
  } while ( display.nextPage() );
}

uint32_t highscore;
//to display anything on the screen, put it inside a function and call render(functionName)
void setupScreen() {
  EEPROM.get(STACKER_HIGHSCORE_EEPROM_ADDR, highscore);
  char text[32];
  sprintf(text, "Highscore: %lu", highscore);
  
  //display the info text
  display.setCursor(0, 0 * 10);
  display.print(F("     Foam Stacker"));
  display.setCursor(0, 1 * 10 + 5);
  display.print(F("    https://jjv.sh"));
  display.setCursor(0, 2 * 10 + 5);
  display.print(F("     /FoamStacker"));
  display.setCursor(0, 4 * 10);
  display.print(text);
  display.setCursor(0, 5 * 10);
//  display.print(F("  Hit any button :-]"));
  display.print(F(" Squeeze to start :-]"));
}

void resetScreen() {
  display.setCursor(0, 1 * 10 + 5);
  display.print(F("    Highscore has"));
  display.setCursor(0, 2 * 10 + 5);
  display.print(F("     been reset."));
  display.setCursor(0, 5 * 10);
//  display.print(F("  Hit any button :-]"));
  display.print(F(" Squeeze to start :-]"));
}

bool ledState;
void ledOn() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
}
void ledOff() {
  ledState = false;
  digitalWrite(LED_PIN, LOW);
}
void ledToggle() {
  ledState = !ledState;
  if (ledState) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  ledOn();
  ArduinOLED.begin(); //start the ArduinOLED library, which controls the buttons and joystick
  display.begin(); //start the u8g2 library, which controls the display

  display.setFont(u8g2_font_6x10_tf); //set the font
  display.setFontRefHeightExtendedText();
  display.setDrawColor(1);
  display.setFontPosTop();
  display.setFontDirection(0);

  if (isButtonPressed()) {
    highscore = 0;
    EEPROM.put(STACKER_HIGHSCORE_EEPROM_ADDR, highscore);
    render(resetScreen);
    waitForButtonRelease();
    waitForButtonPress();
    waitForButtonRelease();
  }

  render(setupScreen); //display the menu
  waitForButtonPress();
  waitForButtonRelease();
  display.setFont(u8g2_font_unifont_t_symbols); //set the font to allow unicode later
}

#define BLOCK_SIZE 16 //8
#define BLOCK_GAP 2 //1
#define WIDTH 8 //16
#define HEIGHT 4-1 //8-2
#define PLAY_AREA_OFFSET_X BLOCK_GAP / 2 //even out the gap on either side
#define PLAY_AREA_OFFSET_Y (BLOCK_GAP - 1) + (1 * BLOCK_SIZE) //rest the blocks on the bottom border and leave 1 row at the top for text
int8_t shakeOffsets[] = {-2, 1};
int8_t offsetX = 0; //used for shake
int8_t offsetY = 0; //used for smoothly moving down
int8_t fallingOffsetY = 0; //used to smoothly move down just the falling blocks

uint8_t board[HEIGHT][WIDTH];
uint8_t fall[WIDTH];
uint8_t stackY;
uint32_t score;
bool isGameOver;
bool gotHighscore = false;

void gameScreen() {
  //draw a border around the edge of the screen
  display.drawFrame(0+offsetX, 0, display.getDisplayWidth()+offsetX, display.getDisplayHeight());

  for(int x=0; x<WIDTH; x++) {
    for(int y=0; y<HEIGHT; y++) {
      if(board[y][x]) {
        display.drawBox(
          x * BLOCK_SIZE + PLAY_AREA_OFFSET_X + offsetX,
          y * BLOCK_SIZE + PLAY_AREA_OFFSET_Y + offsetY + (y == stackY && fall[x] ? fallingOffsetY : 0),
          BLOCK_SIZE - BLOCK_GAP,
          BLOCK_SIZE - BLOCK_GAP
        );
      }
    }
  }
  char text[32];
  sprintf(text, "Score: %lu", score);
  display.drawStr(3+offsetX, 2, text);
  if (gotHighscore) {
    display.setDrawColor(0);
    display.drawBox(0, 40-3, 128, 32);
    display.setDrawColor(1);
    display.drawStr(0+offsetX, 40, "Beat the hiscore");
    sprintf(text, "of %lu!", highscore);
    display.drawStr(0+offsetX, 52, text);
  } else if (isGameOver) {
    display.setDrawColor(0);
    display.drawBox(30-3+offsetX, 40-3, 78, 20);
    display.setDrawColor(1);
    display.drawStr(30+offsetX, 40, "Game Over");
  }
}

void shake(void (*f)(void)) {
  for (uint8_t i=0; i<sizeof(shakeOffsets); i++) {
//    ledToggle();
    offsetX = shakeOffsets[i];
    render(f);
  }
//  ledToggle();
  offsetX = 0;
  render(f);
//  ledOn();
}

void clearBoard() {
  for(int x=0; x<WIDTH; x++) {
    for(int y=0; y<HEIGHT; y++) {
      board[y][x] = 0;
    }
  }
}

uint32_t timer;
int8_t barWidth;
int8_t startingDirection;
int8_t direction;
uint8_t barStart;
uint8_t moveDelay;
bool dropped;

void loop() {
  //new game starts here
  clearBoard();
  score = 0;
  isGameOver = false;
  stackY = HEIGHT-1;
  barWidth = 5;
  startingDirection = 1;
  moveDelay = 250;
  while (!isGameOver) {
    dropped = false;
    direction = startingDirection;
    barStart = (direction == 1) ? 0 : WIDTH - barWidth;
    startingDirection *= -1;
    moveDelay *= 0.85;
    while (!dropped) {
      uint8_t barEnd = barStart+barWidth;
      for(int x=0; x<WIDTH; x++) {
        board[stackY][x] = (x >= barStart && x < barEnd);
      }
      render(gameScreen);

      timer = millis() + moveDelay;
      while (millis() <= timer && !dropped) {
        if (isButtonPressed()) {
          score++;
          //shake horizontally
          shake(gameScreen);

          waitForButtonRelease();
          //remove ones that are not supported and recalculate width, game over if 0
          uint8_t firstFallCount = 0;
          //if we are not on the first layer
          for (uint8_t y=stackY; y<HEIGHT-1; y++) {
            uint8_t fallCount = 0;
            for(int x=0; x<WIDTH; x++) {
              fall[x] = board[y][x] && ! board[y+1][x];
              if (fall[x]) fallCount++;
            }
            if (firstFallCount == 0) firstFallCount = fallCount;
            if (fallCount == 0) break;

            //animate the unsupported ones falling smoothly
            for(fallingOffsetY=0; fallingOffsetY<BLOCK_SIZE; fallingOffsetY+=8) {
              ledToggle();
              render(gameScreen);
            }
            fallingOffsetY = 0;
            for(int x=0; x<WIDTH; x++) {
              if (fall[x]) {
                board[y][x] = 0;
                board[y+1][x] = 1; //the block moved to the spot below
              }
            }

//            //make the falling ones blink
//            for(int z=0; z<4; z++) {
//              for(int x=0; x<WIDTH; x++) {
//                if (fall[x]) board[y][x] = 1;
//              }
//              render(gameScreen);
//              for(int x=0; x<WIDTH; x++) {
//                if (fall[x]) board[y][x] = 0;
//              }
//              render(gameScreen);
//            }
          }
          ledOn();
          render(gameScreen);
          barWidth -= firstFallCount;
          if (barWidth <= 0) {
            isGameOver = true;
            render(gameScreen);
            waitForButtonPress();
            waitForButtonRelease();
            EEPROM.get(STACKER_HIGHSCORE_EEPROM_ADDR, highscore);
            if (score > highscore) {
              EEPROM.put(STACKER_HIGHSCORE_EEPROM_ADDR, score);
              gotHighscore = true;
              shake(gameScreen);
              shake(gameScreen);
              shake(gameScreen);
              shake(gameScreen);
              shake(gameScreen);
              gotHighscore = false;
              highscore = score;
              waitForButtonPress();
              waitForButtonRelease();
            }
          }
          dropped = true;
          if (isGameOver) break;
          if (stackY > 0) {
            stackY--;
          } else {
            //animate the board moving down smoothly (or is it the camera panning up? :)
            for(offsetY=0; offsetY<BLOCK_SIZE; offsetY+=4) {
              render(gameScreen);
            }
            offsetY = 0;
            //shift entire board down by 1
            for(int x=0; x<WIDTH; x++) {
              for(int stackY=HEIGHT-2; stackY>=0; stackY--) { //start on the 2nd to last row
                board[stackY+1][x] = board[stackY][x];
              }
              board[0][x] = 0; //clear the top row
            }
            render(gameScreen);
          }
        }
//        delay(1);
      }
  
      barStart += direction;
      if ((direction < 0 && barStart <= 0) || (direction > 0 && barEnd >= WIDTH-1)) {
        direction *= -1;
      }
    }
  }
}
