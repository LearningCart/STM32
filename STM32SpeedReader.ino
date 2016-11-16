/*
CONNECTIONS:
------------
STM32:
For programming via serial:
Tools/Board set to Generic STM32F103C
Tools/Upload set to Serial
Top jumper set to 1, press the button before uploading

  PA9 /TX to PC RX (VIOLET)
  PA10/RX to PC TX (GREY)
  3V3               (RED)
  GND              (BLUE)

 STM32 SPI1 pins:
  PA4 CS1
  PA5 SCK1
  PA6 MISO1
  PA7 MOSI1

  PA11 RST
  PA12 DC

TFT2.2 ILI9341 from top left:
  MISO  PA6
  LED   +3.3V
  SCK   PA5
  MOSI  PA7
  DC    PA12
  RST   PA11 or +3V3
  CS    PA4
  GND   GND
  VCC   +3.3V

*/
#include <Arduino.h>
#include "SPI.h"

#include <Adafruit_GFX_AS.h>
#include <Adafruit_ILI9341_STM.h>

#define ILI9341_VSCRDEF  0x33
#define ILI9341_VSCRSADD 0x37

int xp = 0;
int yp = 0;
uint16_t bg = ILI9341_BLACK;
uint16_t fg = ILI9341_WHITE;
int screenWd = 240;
int screenHt = 320;
int wrap = 0;
int bold = 0;
int sx = 1;
int sy = 1;
int horizontal = -1;
int scrollMode = 1;

enum ReadingMode {DAY, NIGHT};
enum ReadingMode ReadingMode = DAY;

#define WRAP_PIN    PB9
#define HORIZ_PIN   PB8
#define TFT_CS      PA4                  
#define TFT_DC      PA12              
#define TFT_RST     PA11 
Adafruit_ILI9341_STM tft = Adafruit_ILI9341_STM(TFT_CS, TFT_DC, TFT_RST); // Use hardware SPI


const uint16_t Background[] =
                        {
                            ILI9341_WHITE, /* Day   */
                            ILI9341_BLACK  /* Night */
                        };

const uint16_t LineColor[] = 
                       {
                           ILI9341_LIGHTGREY, /* Day */
                           ILI9341_DARKGREY  /* Night */
                       };

const uint16_t LogoTextColor[] = 
                      {
                             ILI9341_OLIVE,       /* Day */
                             ILI9341_GREENYELLOW /* Night */
                      };

const uint16_t ReadingColor[] = 
                      {
                          ILI9341_BLACK,   /* Day */
                          ILI9341_WHITE    /* Night */
                      };

const uint16_t ReadingPivotColor[] = 
                      {
                          ILI9341_RED,   /* Day */
                          ILI9341_BLUE    /* Night */
                      };

char *peBook = "Success Poem by Berton Barley. If you want a thing bad enough, To go out and fight for it, Work day and night for it, Give up your time and your peace and your sleep for it. If only desire of it Makes you quite mad enough, Never to tire of it, Makes you hold all other things tawdry and cheap for it If life seems all empty and useless without it And all that you scheme and you dream is about it, If gladly you'll sweat for it, Fret for it, Plan for it, Lose all your terror of God or man for it, If you'll simply go after that thing that you want. With all your capacity, Strength and sagacity, Faith, hope and confidence, stern pertinacity, If neither cold poverty, famished and gaunt, Nor sickness nor pain Of body or brain Can turn you away from the thing that you want, If dogged and grim you besiege and beset it, You'll get it! ― Berton Braley";
char *src;

#define MAXWORDLEN      (40)

char extword[MAXWORDLEN];


void mymemset(char *buff, uint8_t character, uint8_t len)
{
  uint8_t count;
  if(0 == len) return;

  for(count =  0; count < len; count++)
  {
      buff[count] = character;
  }
}

char *GetNexWord(char *line, uint8_t *len)
{
  uint8_t lenofword;
  mymemset(extword, '\0', MAXWORDLEN);


  if(NULL == line || '\0' == *line || NULL == len )
  {
    *len = 0;
    return NULL; 
  }

  lenofword = 0;
  while(NULL != line && '\0' != *line)
  {
    if(' ' == *line  || '\t' == *line)
    {
        break;
    }
   
    extword[lenofword++] = *line;
    line++;
  }
  lenofword ++;
  *len = lenofword;

  return extword;
}

void setBookPtr(void)
{
    src = peBook;
}



void drawSplashScreen(void)
{
  tft.begin();
  tft.setRotation(3);
  tft.setTextSize(2);
  tft.setCursor(60, 20);
  
  tft.fillScreen(Background[ReadingMode]);
  tft.setTextColor(LogoTextColor[ReadingMode]);
  tft.println("Jatin Gandhi eReader");
  tft.setTextSize(3);
  tft.drawLine(10, 79, 300, 79,    LineColor[ReadingMode]);
  tft.drawLine(10, 80, 300, 80,    LineColor[ReadingMode]);
  tft.drawLine(10, 160, 300, 160,  LineColor[ReadingMode]);
  tft.drawLine(10, 161, 300, 161,  LineColor[ReadingMode]);
  tft.drawLine(160, 80, 160, 90,   LineColor[ReadingMode]);
  tft.drawLine(161, 80, 161, 90,   LineColor[ReadingMode]);
  tft.drawLine(160, 150, 160, 160, LineColor[ReadingMode]);
  tft.drawLine(161, 150, 161, 160, LineColor[ReadingMode]);
}

void ClearDrawArea(void)
{
  tft.fillRect(10, 90, 300, 60, ILI9341_BLACK);
}


void setup() {
  Serial.begin(115200);

  setBookPtr();
  ReadingMode = DAY;
  
  drawSplashScreen();
  tft.setTextColor(ReadingColor[ReadingMode]); // Optimized form loop..,
}


int GetPivot (int dwordlen)
{
  int pivot;

    switch(dwordlen)
    {
        case 0:
        case 1:
            pivot = 0;
        break;

        case 2:
        case 3:
        case 4:
        case 5:
            pivot = 1;
        break;
        case 6:
        case 7:
        case 8:
        case 9:
            pivot = 2;
        break;

        case 10:
        case 11:
        case 12:
        case 13:
            pivot = 3;
        break;

        default:
        pivot = 4;
        };

  return pivot;
  
}



/* (320 - (2 * MARGIN(10 pixel))) / 2*/
#define CENTER_OF_SCREEN  (160)
#define AVERAGE_CHAR_WIDTH_INPIXEL  (25)

// Note: instead of text intent we are having avg. widht 25.
// support max word len 12 characters.

void  CenterAdjustCursor(int wordlen,  int pivotpoint)
{
  int beforepivot, afterpivot;
  int centerxpos;
  
  afterpivot = wordlen - pivotpoint;
  beforepivot = wordlen - afterpivot;

  centerxpos = CENTER_OF_SCREEN - ( AVERAGE_CHAR_WIDTH_INPIXEL * beforepivot);

  tft.setCursor(centerxpos, 110);
  
}

void loop(void)
{
    uint8_t offset;
    char *disword;
      int pivot,index;
    int wordlen;
    offset = 0;

    disword = GetNexWord(src, &offset);
  
    if(0 != offset && NULL != disword && '\0' != *disword)
    {
      
        ClearDrawArea();
        
        // tft.setCursor(80, 100);
        wordlen = strlen(disword);
        pivot = GetPivot (wordlen);
        CenterAdjustCursor(wordlen, pivot);
        
        // tft.setTextColor(ILI9341_WHITE);
        for(index = 0; index < pivot; index ++)
        {
            tft.print(disword[index]);
        }
        tft.setTextColor(ReadingPivotColor[ReadingMode]);
        tft.print(disword[pivot]);
        tft.setTextColor(ReadingColor[ReadingMode]);
        for(index = pivot + 1; index < wordlen; index++)
        {
            tft.print(disword[index]);
        }
        //  printString(disword);
        delay(70 * wordlen); // 70 ms per character

        if((NULL != strchr(disword,'.'))||  (NULL != strchr(disword,',')))
        {
          /* Add additional delay for end of line or comma */
		      delay(200);
        }
        
        src += offset;
        if(src > (peBook + strlen(peBook)))
        {
          setBookPtr();  // restart the book
        }
    }
}


