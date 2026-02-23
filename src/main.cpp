#include <GxEPD2_BW.h>
#include <Adafruit_GFX.h>
#include "bitmap.h"

// classic ones
extern const unsigned char dice_1[] PROGMEM;
extern const unsigned char dice_2[] PROGMEM;
extern const unsigned char dice_3[] PROGMEM;
extern const unsigned char dice_4[] PROGMEM;
extern const unsigned char dice_5[] PROGMEM;
extern const unsigned char dice_6[] PROGMEM;

// menu stuff
extern const unsigned char intro[] PROGMEM;
extern const unsigned char epd_bitmap_waiting_for_input_screen[] PROGMEM;
extern const unsigned char epd_bitmap_flag_screen[] PROGMEM;
extern const unsigned char epd_bitmap_choose_menu_1[] PROGMEM;
extern const unsigned char epd_bitmap_choose_menu_2[] PROGMEM;
extern const unsigned char epd_bitmap_welcome_to_classic_mode[] PROGMEM;
extern const unsigned char epd_bitmap_welcome_to_custom_mode[] PROGMEM;
extern const unsigned char epd_bitmap_choose_menu_settings [] PROGMEM;
extern const unsigned char epd_bitmap_Welcome_to_settings [] PROGMEM;
extern const unsigned char epd_bitmap_Sound_Off [] PROGMEM;
extern const unsigned char epd_bitmap_sound_on [] PROGMEM;

// custom choices menu
extern const unsigned char epd_bitmap_choose_dice_1[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_2[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_3[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_4[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_5[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_6[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_7[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_8[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_9[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_10[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_11[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_12[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_13[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_14[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_15[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_16[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_17[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_18[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_19[] PROGMEM;
extern const unsigned char epd_bitmap_choose_dice_20[] PROGMEM;

// A array to choose the proper number from 1 to 20
const unsigned char* const diceBitmaps[] PROGMEM = 
{

  epd_bitmap_choose_dice_1, epd_bitmap_choose_dice_2, epd_bitmap_choose_dice_3, epd_bitmap_choose_dice_4,
  epd_bitmap_choose_dice_5, epd_bitmap_choose_dice_6, epd_bitmap_choose_dice_7, epd_bitmap_choose_dice_8,
  epd_bitmap_choose_dice_9, epd_bitmap_choose_dice_10, epd_bitmap_choose_dice_11, epd_bitmap_choose_dice_12,
  epd_bitmap_choose_dice_13, epd_bitmap_choose_dice_14, epd_bitmap_choose_dice_15, epd_bitmap_choose_dice_16,
  epd_bitmap_choose_dice_17, epd_bitmap_choose_dice_18, epd_bitmap_choose_dice_19, epd_bitmap_choose_dice_20

};

// ESP32 DevKit pins
#define CS 5     //D5
#define DC 17    //D17
#define RST 16   //D16
#define BUSY 4   //D4
#define BUTTON_PIN 25
#define ENCODER_A 32
#define ENCODER_B 33
#define ENCODER_BTN 26
#define BUZZER_PIN 12 //D12

volatile int encoderPos = 0;
int lastEncoded = 0;
int curr_index = 0;   // global bitmap index
int total_index = 19; // total bitmaps from 0
int currentMenuIndex = 0;
int lastMenuIndex = currentMenuIndex;
unsigned long lastMoveTime = 0;          // track last processed move
const unsigned long debounceDelay = 3000;
int lastMenuStep = 0;
bool customJustEntered = false;
bool sound_on = true;
bool settingsJustEntered = true;

// Create the display object
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
  GxEPD2_154_D67(CS, DC, RST, BUSY)
);

// Function declarations
void rolldice_num(int upperlim); // implemented but add custom images for each number
void classicRoll();              // a classic roll
void display_once(const unsigned char* bitmap, int w, int h);
void displayNumberCentered(int number); // currently for rolldice_num
void initEncoder();
void updateEncoder();
void scroll(int move2);
void update_display_choice();
void doNothing();  // placeholder for settings
void dingDong();

struct menuitem // global menu so that each menu has a intro , menu page and a function
{

  const unsigned char* image;
  const unsigned char* opening_image;
  void (*action)();

};

enum AppState 
{
  MAIN_MENU,
  CUSTOM_SELECT,
  SETTINGS
};

AppState currentState = MAIN_MENU;

 menuitem menu[] = 
     {
         { epd_bitmap_choose_menu_1,epd_bitmap_welcome_to_classic_mode,classicRoll },
         { epd_bitmap_choose_menu_2,epd_bitmap_welcome_to_custom_mode,[](){ currentState = CUSTOM_SELECT;customJustEntered = true; lastMenuIndex = -1; } },
         { epd_bitmap_choose_menu_settings,epd_bitmap_Welcome_to_settings,[](){currentState = SETTINGS;}}
     };

void setup() 
{

  display.init();
  display.setRotation(1);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // internal pull-up for button
  initEncoder();
  pinMode(BUZZER_PIN, OUTPUT);
  display_once(intro, 200, 200);
  delay(500);
  display_once(epd_bitmap_choose_menu_1, 200, 200);

}

void loop() 
{
  if (currentState == MAIN_MENU)
  {
    int menuStep = encoderPos / 4;

     // keep currentMenuIndex in range
     if (currentMenuIndex >= (sizeof(menu)/sizeof(menu[0]))) currentMenuIndex = 0;
     if (currentMenuIndex < 0) currentMenuIndex = sizeof(menu)/sizeof(menu[0]) - 1;

     // display menu if changed
     if (currentMenuIndex != lastMenuIndex) 
     {
         display_once(menu[currentMenuIndex].image, 200, 200);
         lastMenuIndex = currentMenuIndex;
     }

     // button press
     if (digitalRead(ENCODER_BTN) == LOW) 
     {
       display_once(menu[currentMenuIndex].opening_image, 200, 200);
       delay(300);
       menu[currentMenuIndex].action();
       lastMenuIndex = -1;       // force refresh after action
       delay(500);               // debouncing
     }

     // encoder handling
     unsigned long now = millis();

     // only process if step changed AND debounce time passed
     if (menuStep != lastMenuStep && (now - lastMoveTime > debounceDelay)) {
     if (menuStep > lastMenuStep) currentMenuIndex++;
     else currentMenuIndex--;

     // wrap menu
     if (currentMenuIndex < 0) currentMenuIndex = sizeof(menu)/sizeof(menu[0]) - 1;
     if (currentMenuIndex >= sizeof(menu)/sizeof(menu[0])) currentMenuIndex = 0;

     lastMenuStep = menuStep;  // save last processed step
     lastMoveTime = now;
   }
  
  }
  
  if (currentState == CUSTOM_SELECT) {

    if (customJustEntered) {
        encoderPos = 0;
        lastMenuStep = 0;
        curr_index = 0;

        update_display_choice();  // show first dice option
        customJustEntered = false;
    }

    int menuStep = encoderPos / 4;

    if (menuStep != lastMenuStep) {
        if (menuStep > lastMenuStep) curr_index++;
        else curr_index--;

        if (curr_index < 0) curr_index = total_index;
        if (curr_index > total_index) curr_index = 0;

        lastMenuStep = menuStep;

        update_display_choice();
    }

    if (digitalRead(ENCODER_BTN) == LOW) 
    {
        delay(300);
        
        rolldice_num(curr_index + 1);

        if (sound_on){dingDong();}
        
        delay(1000);

        currentState = MAIN_MENU;
        lastMenuIndex = -1;
    }
}
  
  if (currentState == SETTINGS)
{
    if (settingsJustEntered)
    {
        if (sound_on)
            display_once(epd_bitmap_sound_on,200,200);
        else
            display_once(epd_bitmap_Sound_Off,200,200);

        settingsJustEntered = false;
    }

    if (digitalRead(ENCODER_BTN) == LOW)
    {
        delay(300);

        sound_on = !sound_on;

        if (sound_on)
            display_once(epd_bitmap_sound_on,200,200);
        else
            display_once(epd_bitmap_Sound_Off,200,200);

        delay(300);

        currentState = MAIN_MENU;
        settingsJustEntered = true;
        lastMenuIndex = -1;
    }
}
  
}


void classicRoll() // perfect code no work needed here!
{

  int dice = random(1, 7);

  switch(dice) 
  {

    case 1: display_once(dice_1, 200, 200); if (sound_on){dingDong();}; break;
    case 2: display_once(dice_2, 200, 200); if (sound_on){dingDong();}; break;
    case 3: display_once(dice_3, 200, 200); if (sound_on){dingDong();}; break;
    case 4: display_once(dice_4, 200, 200); if (sound_on){dingDong();}; break;
    case 5: display_once(dice_5, 200, 200); if (sound_on){dingDong();}; break;
    case 6: display_once(dice_6, 200, 200); if (sound_on){dingDong();}; break;
  
  }

  delay(500);

}

// helper function to display a bitmap once
void display_once(const unsigned char* bitmap, int w, int h) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, bitmap, w, h, GxEPD_BLACK);
  } while (display.nextPage());
}

// helper to write text with coords and text size feature
void display_text(const char* text, int x = 0, int y = 0, int textSize = 1) {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.setTextColor(GxEPD_BLACK);
    display.setTextSize(textSize);
    display.print(text);
  } while (display.nextPage());
}

// helper that rolls the dice using an upper limit
void rolldice_num(int upperlim) 
{

  int dice = random(1, (upperlim + 1));
  displayNumberCentered(dice);

}

// encoder stuff
void initEncoder()
{
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), updateEncoder, CHANGE);
}

// updates the encoder
void updateEncoder() {
  int MSB = digitalRead(ENCODER_A);
  int LSB = digitalRead(ENCODER_B);
  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderPos++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderPos--;

  lastEncoded = encoded;
}

// the main display function
void displayNumberCentered(int number) {
  int textSize = 10;
  int charWidth = 6 * textSize;
  int charHeight = 8 * textSize;
  int numChars = String(number).length();
  int x = (200 - (charWidth * numChars)) / 2;
  int y = (200 - charHeight) / 2;
  display_text(String(number).c_str(), x, y, textSize);
}

// scrolling function
void scroll(int move2) {
  curr_index += move2;
  if (curr_index < 0) curr_index = total_index;
  else if(curr_index > total_index) curr_index = 0;
}

// displays the current choice
void update_display_choice() {
  int bitmapIndex = curr_index;
  const unsigned char* bmp = (const unsigned char*)pgm_read_ptr(&diceBitmaps[bitmapIndex]);
  display_once(bmp, 200, 200);
}

// test
void dingDong() 
{
  tone(BUZZER_PIN, 1200);
  delay(180);
  noTone(BUZZER_PIN);
  delay(100);
  tone(BUZZER_PIN, 800);
  delay(250);
  noTone(BUZZER_PIN);
}
