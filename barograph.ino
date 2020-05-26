#include <EEPROM.h>

#include <SD.h>

#include <Adafruit_GFX.h>
#include <Adafruit_BMP280.h>
#include <stdio.h>

#if defined(_GFXFONT_H_)           //are we using the new library?
#include <Fonts/FreeMono9pt7b.h>
#define ADJ_BASELINE 11            //new fonts setCursor to bottom of letter
#else
#define ADJ_BASELINE 0             //legacy setCursor to top of letter
#endif
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

// Colour Defines
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define RGB(r, g, b) (((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3))

#define GREY      RGB(127, 127, 127)
#define DARKGREY  RGB(64, 64, 64)
#define TURQUOISE RGB(0, 128, 128)
#define PINK      RGB(255, 128, 192)
#define OLIVE     RGB(128, 128, 0)
#define PURPLE    RGB(128, 0, 128)
#define AZURE     RGB(0, 128, 255)
#define ORANGE    RGB(255,128,64)
#define DARKGREEN RGB(0,128,0)
#define LIGHTGREEN RGB(0,255,128)

Adafruit_BMP280 bmp; // I2C



uint16_t ID;

uint16_t HEIGHT = 319;
uint16_t WIDTH = 479;
uint16_t GRAPH_WIDTH = 479 - 6 - 6;
uint16_t GRAPH_HEIGHT = 319 - 64 - 6;
uint16_t TOP_GRAPH = 64;
uint16_t GRADULE = 64;
uint16_t BOTTOM_GRAPH= 310;


// Data Defines
#define FIVE_MINUTES 30000 //0   // milliseconds
#define POINTS_PER_DAY 256
#define MAX_DAYS 7
#define BARO_ARRAY_SIZE (POINTS_PER_DAY * MAX_DAYS) // 7 days data
#define POINTS_PER_DAY  256

// Baro Array Define
uint16_t m_baroDataArray[BARO_ARRAY_SIZE];
uint16_t m_baroDataHead = 0;
uint16_t m_numDays = 1;
uint16_t m_numDataPoints = m_numDays * POINTS_PER_DAY;

// baro filter
#define FILTER_SIZE 8
uint16_t m_baroFilter[FILTER_SIZE]= {0};
uint16_t m_yPosFilter[FILTER_SIZE]= {0};



void setup() 
{
    // put your setup code here, to run once:
    Serial.begin(38400);

    // Reset the TFT
    tft.reset();
    ID = tft.readID();
    if (ID == 0xD3D3) ID = 0x9486; // write-only shield
    tft.begin(ID);
    tft.setRotation(1);
    tft.fillScreen(BLACK);

#if defined(_GFXFONT_H_)
    tft.setFont(&FreeMono9pt7b);
#endif

    // Set up the Barometer chip
    /* Default settings from datasheet. */
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

    if (!bmp.begin())
    {
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    }
}


void DrawInitScreen()
{
    // put a box on the screen
    tft.fillRect (0, 0 , 479 , 5 , GREEN);
    tft.fillRect (0, 315 , 479 , 5 , GREEN);
    tft.fillRect (0 , 5 , 5 , 319 , GREEN);
    tft.fillRect (474 , 0 , 5 , 319 , GREEN);

    tft.setCursor (1, 20);
    tft.setTextColor(YELLOW);
    tft.setTextSize(1);


    tft.setCursor (10, 30);
    tft.print("Baro         Position           SOG    Kts");
    tft.setCursor (210, 60);
    tft.print("COG");
}

void UpdatePosition (char* latitude , char* longitude)
{
    tft.setCursor (1, 20);
    tft.setTextColor(CYAN);
    tft.setTextSize(1);
    tft.setCursor (60, 30);
}

void UpdateBaro (char* baro)
{
    static char lastBaro[10] = {"1013.4"};
    tft.setCursor (1, 20);
    tft.setTextColor(CYAN , BLACK);
    tft.setTextSize(1);
    int16_t x, y, x1, y1;
    uint16_t w , h;
    x = 60;
    y = 30;
    tft.getTextBounds (lastBaro , x , y , &x1, &y1 , &w , &h);
    tft.fillRect (x1, y1, w, h, BLACK);
    tft.setCursor (x, y);
    tft.print(baro);

    strcpy (baro , lastBaro);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void GetHighLowRange (uint16_t& high , uint16_t &low , uint16_t &range)
{
    // loop the data and find the high and low
    high = 0;
    low = 0xFFFF ; 
    range = 0;
    int16_t offset = m_baroDataHead - m_numDataPoints;
    if (offset < 0) offset += BARO_ARRAY_SIZE;
    
    
    for (uint16_t i = 0 ; i < m_numDataPoints ; i++)
    {
        if (m_baroDataArray[offset] < low) low = m_baroDataArray[offset];
        else if (m_baroDataArray[offset] > high) high = m_baroDataArray[offset];        

        // check range  
        if (++offset >= BARO_ARRAY_SIZE) offset = 0;
    }
    range = high - low;
    Serial.print ("GetHighLowRange() Range ");
    Serial.print (range , DEC);
    Serial.print (" High ");
    Serial.print (high , DEC);
    Serial.print (" Low ");
    Serial.println (low , DEC);
}

void ScaleHighLowRange (uint16_t& high , uint16_t &low , uint16_t &range)
{
    if (range < 10) range = 10;
    else if (range < 20) range = 20;
    else if (range < 50) range = 50;
    else if (range < 100) range = 100;
    else if (range < 200) range = 200;
    else if (range < 500) range = 500;
    else range =  1000; 

    high = uint16_t((high / 10)+1)*10;
    low = uint16_t(low / 10)*10;
    Serial.print ("ScaleHighLowRange() Range ");
    Serial.print (range , DEC);
    Serial.print (" High ");
    Serial.print (high , DEC);
    Serial.print (" Low ");
    Serial.println (low , DEC);
}

//----------------------------------------
//
//----------------------------------------
int16_t Interpolate (int16_t value , int16_t fromLow , int16_t fromHigh , int16_t toLow , int16_t toHigh)
{
    float ffromLow = fromLow;
    float ffromHigh = fromHigh;
    float ftoLow = toLow;
    float ftoHigh = toHigh;
    float fvalue = value;
    float result = (fvalue - ffromLow ) * (ftoHigh - ftoLow) / (ffromHigh - ffromLow) + ftoLow;
    return (int16_t) result;
}
//----------------------------------------
//
//----------------------------------------
uint16_t FilterDisplay (uint16_t yPos)
{
    static int head = 0;
    // init the filter
    if (m_yPosFilter[0] == 0)
    {
        for (int i = 0 ; i < FILTER_SIZE ; i++)
        {
            m_yPosFilter[i] = yPos;
        }
        return yPos;
    }
    else 
    {
        m_yPosFilter[head++] = yPos;
        if (head == FILTER_SIZE)
            head = 0;
    }
    uint32_t total = 0; 
    for (int i = 0 ; i < FILTER_SIZE ; i++)
    {
        total += m_yPosFilter[i];
    }
    return total / FILTER_SIZE;
}



//----------------------------------------
//
//----------------------------------------
void DrawBaro (uint16_t baro)
{
    // Add the new value
    m_baroDataArray[m_baroDataHead] = baro;
    
    // get the high low and range
    uint16_t high , low , range;
    GetHighLowRange (high , low , range);
    ScaleHighLowRange(high, low, range);
    

    uint16_t offset = m_baroDataHead - m_numDataPoints;
    if (offset < 0) offset += BARO_ARRAY_SIZE;
    uint16_t lastX = 0;
    uint16_t lastY = 0;

    // reset yPos filter
    for (int i=0; i < FILTER_SIZE ; i++)
    {
        m_yPosFilter[i] = 0;
    }
    
    for (int i = 0 ; i < m_numDataPoints ; i++)
    {
        // need to draw the pixel
        // scale between 950 - 1050

        baro = m_baroDataArray[offset];

        uint16_t yPos = Interpolate (baro , high , low , TOP_GRAPH , BOTTOM_GRAPH);
        yPos = FilterDisplay(yPos);

        if (yPos < 70) yPos = 70;
        if (yPos > 310) yPos = 310;
        
        uint16_t xPos = Interpolate (i , 0 , m_numDataPoints , 6 , 470);
        
        if (lastY != 0)
        {
            // Over draw the previous colours
            for (int x = lastX ; x <= xPos ; x++)
            {
                if (x%GRADULE == 0 )//|| x%32 == 0)
                    tft.drawFastVLine (x , TOP_GRAPH , GRAPH_HEIGHT , DARKGREEN);
                else
                    tft.drawFastVLine (x , TOP_GRAPH , GRAPH_HEIGHT , BLACK);
            }

            // draw a line
            tft.drawLine (lastX , lastY , xPos , yPos , LIGHTGREEN);
            tft.drawLine (lastX , lastY+1 , xPos , yPos+1 , LIGHTGREEN);
        }
        for (int y = TOP_GRAPH ; y < HEIGHT - 6 ; y += GRADULE)
        {
            tft.drawFastHLine (0 , y , GRAPH_WIDTH , DARKGREEN);
        }

        lastX = xPos;
        lastY = yPos;
        
        // check range  
        if (++offset >= BARO_ARRAY_SIZE) offset = 0;
    }
        // reset the array head if necessary
    if (++m_baroDataHead > BARO_ARRAY_SIZE)
        m_baroDataHead = 0;

}

//----------------------------------------
//
//----------------------------------------
void DrawTitles()
{
    int16_t mBar = 1030;
    tft.setTextSize(1);
    for (int y = 64 ; y < HEIGHT; y += 32)
    {
        tft.setCursor (2 , y);
        tft.print (mBar);
        mBar -= 5;
    }
}

//----------------------------------------
//
//----------------------------------------
void DrawGraph ()
{
    for (int y = TOP_GRAPH ; y < HEIGHT; y += GRADULE)
    {
        tft.drawFastHLine (0 , y , WIDTH , DARKGREEN);
    }
    for (int x = 0 ; x < WIDTH; x += GRADULE)
    {
        tft.drawFastVLine (x , TOP_GRAPH , HEIGHT , DARKGREEN);
    }
}

//----------------------------------------
// Test Code - will load from eprom
//----------------------------------------
void LoadBaroArray ()
{
    int16_t initVal = 10134;
    int16_t delta = 0;
    for (int16_t i = 0 ; i < BARO_ARRAY_SIZE ; i++)
    {
        int rn = random (10);
        if (rn < 2)
            delta = 1;
        if (rn > 8)
            delta = -2;
        m_baroDataArray[i] = (initVal += delta);
    }
}

//----------------------------------------
//
//----------------------------------------
uint16_t FilterBaro (uint16_t baro)
{
    static int head = 0;
    // init the filter
    if (m_baroFilter[0] == 0)
    {
        for (int i = 0 ; i < FILTER_SIZE ; i++)
        {
            m_baroFilter[i] = baro;
        }
        return baro;
    }
    else 
    {
        m_baroFilter[head++] = baro;
        if (head == FILTER_SIZE)
            head = 0;
    }
    uint32_t total = 0; 
    for (int i = 0 ; i < FILTER_SIZE ; i++)
    {
        total += m_baroFilter[i];
    }
    return total / FILTER_SIZE;
}

//----------------------------------------
//
//----------------------------------------
void loop()
{
    // draw the screen
    DrawGraph();
    DrawInitScreen ();
    LoadBaroArray();
    
    m_baroDataHead = 256;
    
    // Local variables
    float pressure = 0.0;

    uint32_t lastReadTime = 0;
    uint32_t lastUpdateTime = 0;

    int16_t lastPressure = 0;
    int16_t int_pressure = 0;
    char buf[10];

    while (true)
    {
        if (lastReadTime == 0 || millis() - lastReadTime > 3000)
        {
            lastReadTime = millis();
            int_pressure = (int16_t)(bmp.readPressure() / 10.0);
            int_pressure = FilterBaro (int_pressure);
        }        
      
        if (lastUpdateTime == 0 || millis() - lastUpdateTime > FIVE_MINUTES)
        {
            lastUpdateTime = millis();
            
            // update the graph
            DrawBaro (int_pressure);

            // Update the value only if its changed
            if (lastPressure != int_pressure)
            {
                lastPressure = int_pressure;
                sprintf (buf , "%d.%d" , int_pressure / 10 , int_pressure % 10);
                UpdateBaro (buf);
            }
        }      
        delay(100);
    }
}
