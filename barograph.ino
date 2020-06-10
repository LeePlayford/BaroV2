#include <EEPROM.h>


#include <Adafruit_GFX.h>
#include <Adafruit_BMP280.h>
#include <stdio.h>

#if defined(_GFXFONT_H_)           //are we using the new library?
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#define ADJ_BASELINE 11            //new fonts setCursor to bottom of letter
#else
#define ADJ_BASELINE 0             //legacy setCursor to top of letter
#endif
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;

// Colour Defines
#define BLACK   0x0000
//#define BLUE    0x001F
//#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
//#define WHITE   0xFFFF

#define RGB(r, g, b) (((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3))

//#define GREY      RGB(127, 127, 127)
#define DARKGREY  RGB(64, 64, 64)
//#define TURQUOISE RGB(0, 128, 128)
//#define PINK      RGB(255, 128, 192)
//#define OLIVE     RGB(128, 128, 0)
//#define PURPLE    RGB(128, 0, 128)
//#define AZURE     RGB(0, 128, 255)
//#define ORANGE    RGB(255,128,64)
#define DARKGREEN RGB(0,128,0)
#define LIGHTGREEN RGB(0,255,128)

Adafruit_BMP280 bmp; // I2C



uint16_t ID;

uint16_t HEIGHT = 319;
uint16_t WIDTH = 479;
uint16_t GRAPH_WIDTH = 400;
uint16_t GRAPH_HEIGHT = 245;
uint16_t TOP_GRAPH = 70;
uint16_t GRADULE = 66;
uint16_t BOTTOM_GRAPH= 310;
uint16_t LEFT_GRAPH = 70;


// Data Defines
uint32_t SAMPLE_TIME = 86400/4*10;
#define POINTS_PER_DAY 400
#define MAX_DAYS 1
#define BARO_ARRAY_SIZE (POINTS_PER_DAY * MAX_DAYS) // 7 days data

// Baro Array Define
uint16_t m_baroDataArray[BARO_ARRAY_SIZE];
uint16_t m_baroDataHead = 0;

// baro filter
#define FILTER_SIZE 8
uint16_t m_baroFilter[FILTER_SIZE]= {0};
uint16_t m_yPosFilter[FILTER_SIZE]= {0};


//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void setup() 
{
    // put your setup code here, to run once:
    //Serial.begin(38400);

    // Reset the TFT
    tft.reset();
    ID = tft.readID();
    if (ID == 0xD3D3) ID = 0x9486; // write-only shield
    tft.begin(ID);
    tft.setRotation(1);
    tft.fillScreen(BLACK);

#if defined(_GFXFONT_H_)
    tft.setFont(&FreeSans9pt7b);
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
    //    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    }
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void DrawInitScreen()
{
    // put a box on the screen
    tft.fillRect (0, 0 , 479 , 4 , GREEN);      // Top Line
    tft.fillRect (0, 316 , 479 , 4 , GREEN);    // Bottom Line
    tft.fillRect (0 , 4 , 4 , 319 , GREEN);     // Left Line
    tft.fillRect (475 , 0 , 4 , 319 , GREEN);   // Right Line
    tft.fillRect (0, 66 , 475 , 4 , GREEN);     // Horiz Divider
    tft.fillRect (66 , 70 , 2 , 250 , GREEN);   // Vertical Divider

    tft.setCursor (1, 20);
    tft.setTextColor(YELLOW);
    tft.setTextSize(1);


    tft.setCursor (10, 30);
    tft.print("Baro");
    tft.setCursor (140, 30);
    tft.print("High");
    tft.setCursor (10, 60);
    tft.print("Trend");
    tft.setCursor (260, 30);
    tft.print("Low");
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void UpdateDelta (char* delta)
{
    static char lastDelta[6] = {"10.0"};

    tft.setFont(&FreeSans12pt7b);
    int16_t x, y, x1, y1;

    x = 310;
    y = 60;

    tft.setTextColor(CYAN , BLACK);
    tft.setTextSize(1);
    uint16_t w , h;
    tft.getTextBounds (lastDelta , x , y , &x1, &y1 , &w , &h);
    tft.fillRect (x1, y1, w, h, BLACK);
    tft.setCursor (x, y);
    tft.print(delta);

    strcpy (lastDelta , delta);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void UpdateLow (char* low)
{
    static char lastLow[7] = {"1013.5"};

    tft.setFont(&FreeSans12pt7b);
    int16_t x, y, x1, y1;

    x = 310;
    y = 30;

    tft.setTextColor(CYAN , BLACK);
    tft.setTextSize(1);
    uint16_t w , h;
    tft.getTextBounds (lastLow , x , y , &x1, &y1 , &w , &h);
    tft.fillRect (x1, y1, w, h, BLACK);
    tft.setCursor (x, y);
    tft.print(low);

    strcpy (lastLow , low);
}


//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void UpdateHigh (char* high)
{
    static char lastHigh[7] = {"1013.6"};
    tft.setFont(&FreeSans12pt7b);

    tft.setTextColor(CYAN , BLACK);
    tft.setTextSize(1);
    int16_t x, y, x1, y1;
    uint16_t w , h;
    x = 180;
    y = 30;
    tft.getTextBounds (lastHigh , x , y , &x1, &y1 , &w , &h);
    tft.fillRect (x1, y1, w, h, BLACK);
    tft.setCursor (x, y);
    tft.print(high);

    strcpy (lastHigh , high);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void UpdateTrend (int16_t baro)
{
    // get last update and 3 hours before
    int16_t offset = m_baroDataHead - POINTS_PER_DAY / 8;
    if (offset < 0) offset += POINTS_PER_DAY; 
    int16_t threeHours = m_baroDataArray[offset] ;

    int16_t diff = baro - threeHours;
    char buf[6];
    if ( diff < 0)
        sprintf (buf , "-%d.%d" , abs(diff / 10) , abs(diff % 10));
    else
        sprintf (buf , "%d.%d" , abs(diff / 10) , abs(diff % 10));
    
    UpdateDelta(buf);
    const int8_t BUF_SIZE = 18;
    char f_r [BUF_SIZE];
    memset (f_r , 0x0 , BUF_SIZE);
    if (abs(diff) < 1)
    {
        strcpy (f_r , "Steady ");
    }
    else
    {
        if (diff > 0)
            strcpy (f_r , "Rising ");
        else
            strcpy (f_r , "Falling ");
         
        if (abs(diff) < 15)
            strcat (f_r , "Slowly"); 
        else if (abs(diff) < 35)
            strcat (f_r , ""); 
        else if (abs(diff) < 60)
            strcat (f_r , "Quickly"); 
        else
            strcat (f_r , "Rapidly");
    } 

    static char lastTrend[BUF_SIZE] = {"Steady"};
    tft.setFont(&FreeSans12pt7b);

    int16_t x, y, x1, y1;
    x = 80;
    y = 60;
    
    tft.setCursor (x,y);
    tft.setTextColor(MAGENTA , BLACK);
    tft.setTextSize(1);
    uint16_t w , h;
    tft.getTextBounds (lastTrend , x , y , &x1, &y1 , &w , &h);
    tft.fillRect (x1, y1, w, h, BLACK);
    tft.setCursor (x, y);
    tft.print(f_r);

    strcpy (lastTrend , f_r);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void FormatBaro(int16_t baro , char * buffer)
{
    if (baro < 10000)
        sprintf (buffer , "  %d.%d" , baro / 10 , baro % 10);
    else
        sprintf (buffer , "%d.%d" , baro / 10 , baro % 10);
    
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void UpdateBaro (int16_t baro)
{
    UpdateTrend (baro);
    char buf[7];
  
    uint16_t high , low , range;
    GetHighLowRange(high , low , range);
    //ScaleHighLowRange(high, low, range);

    FormatBaro (high , buf);
    UpdateHigh (buf);
    FormatBaro (low , buf);
    UpdateLow (buf);

    FormatBaro (baro , buf);
  
    static char lastBaro[7] = {"1013.3"};

    tft.setFont(&FreeSans12pt7b);
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
    tft.print(buf);

    strcpy (lastBaro , buf);
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
    
    for (uint16_t i = 0 ; i < BARO_ARRAY_SIZE ; i++)
    {
        if (m_baroDataArray[i] < low) 
            low = m_baroDataArray[i];
        else if (m_baroDataArray[i] > high) 
            high = m_baroDataArray[i];        
    }
    
    range = high - low;
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
uint16_t GetRange (uint16_t range)
{
    if (range < 10) range = 10;
    else if (range < 20) range = 20;
    else if (range < 50) range = 50;
    else if (range < 100) range = 100;
    else if (range < 200) range = 200;
    else if (range < 500) range = 500;
    else range =  1000; 
    return range;
    
}
//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void ScaleHighLowRange (uint16_t& high , uint16_t &low , uint16_t &range)
{
    range = GetRange (range);
    high = uint16_t((high / 10)+1)*10;

    if (low < high - range)
    {
        range = GetRange (range + 10);
    }
    low = high - range;
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
void AddScale (uint16_t baro , uint16_t stepVal)
{
    // Clear out box
    tft.fillRect (5,70,62,245 , BLACK);
    uint16_t yPos = 312;

    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(CYAN , BLACK);
    tft.setTextSize(1);
    char buf[8];

    for (int i = 0 ; i <= 5 ; i++)
    {
        tft.setCursor (8, yPos);
        FormatBaro (baro , buf);            
        tft.print(buf);
        baro += stepVal;
        yPos -= 45;
    }
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
    static uint16_t lastHigh , lastLow , lastRange;
    
    GetHighLowRange (high , low , range);
    ScaleHighLowRange(high, low, range);

    // Draw the Baro Scale
    if (lastHigh != high || lastLow != low || lastRange != range)
    {
        lastHigh = high;
        lastLow = low;
        lastRange = range;
        AddScale (low , range / 5);
    }

    int16_t offset = m_baroDataHead - BARO_ARRAY_SIZE + 1;
    if (offset < 0) offset += BARO_ARRAY_SIZE;
    uint16_t lastX = 0;
    uint16_t lastY = 0;

    // reset yPos filter
    for (int i=0; i < FILTER_SIZE ; i++)
    {
        m_yPosFilter[i] = 0;
    }
    
    for (int i = 0 ; i < BARO_ARRAY_SIZE ; i++)
    {
        // need to draw the pixel
        // scale between 950 - 1050

        baro = m_baroDataArray[offset];
        uint16_t yPos = Interpolate (baro , high , low , TOP_GRAPH , BOTTOM_GRAPH);
        
        yPos = FilterDisplay(yPos);

        if (yPos < TOP_GRAPH) yPos = TOP_GRAPH;
        if (yPos > BOTTOM_GRAPH) yPos = BOTTOM_GRAPH;
        
        uint16_t xPos = LEFT_GRAPH + i;
        if (lastY != 0)
        {
            // Over draw the previous colours
            for (int x = lastX ; x <= xPos ; x++)
            {
                if ((x-22)%45 == 0 )
                    tft.drawFastVLine (x , TOP_GRAPH , GRAPH_HEIGHT , DARKGREEN);
                else if (x == 426)
                    tft.drawFastVLine (x , TOP_GRAPH , GRAPH_HEIGHT , YELLOW);
                else
                    tft.drawFastVLine (x , TOP_GRAPH , GRAPH_HEIGHT , BLACK);
            }

            // draw a line
            tft.drawLine (lastX , lastY , xPos , yPos , LIGHTGREEN);
            tft.drawLine (lastX , lastY+1 , xPos , yPos+1 , LIGHTGREEN);
        }

        // draw the Horiontal lines
        for (int y = low ; y <= high ; y+= range / 5)
        {
            uint16_t yPos = Interpolate (y , high , low , TOP_GRAPH , BOTTOM_GRAPH);
            tft.drawFastHLine (LEFT_GRAPH , yPos , GRAPH_WIDTH , DARKGREEN);           
        }

        // keep the last positions
        lastX = xPos;
        lastY = yPos;
        
        // check range  
        if (++offset >= BARO_ARRAY_SIZE) offset = 0;
    }
    
    // reset the array head if necessary
    if (++m_baroDataHead >= BARO_ARRAY_SIZE)
        m_baroDataHead = 0;
}

//----------------------------------------
//
//----------------------------------------
void StoreData ()
{
    uint16_t offset = m_baroDataHead;

    for (int i = 0 ; i < BARO_ARRAY_SIZE ; i++)
    {
        EEPROM.update (i*2 , m_baroDataArray[i] & 0xff);
        EEPROM.update ((i*2)+1 , (m_baroDataArray[i] >> 8) & 0xff);
    }
    EEPROM.update (0x3fe , m_baroDataHead &0xff);
    EEPROM.update (0x3ff , (m_baroDataHead >> 8) &0xff);   
}

//----------------------------------------
//
//----------------------------------------
void ReadData ()
{
    for (int i = 0 ; i < BARO_ARRAY_SIZE ; i++)
    {
        uint16_t value = EEPROM.read (i*2);
        value |= EEPROM.read((i*2)+1) << 8;
        if (value == 0xffff || value < 9500 || value > 10500)
        {
            m_baroDataArray[i] = 10134;
        }
        else
            m_baroDataArray[i] = value;

    }
    // Read in the data write position
    m_baroDataHead = EEPROM.read (0x3fe);
    m_baroDataHead |= (EEPROM.read (0x3ff) << 8);
    if (m_baroDataHead >= BARO_ARRAY_SIZE)
        m_baroDataHead = 0;
    
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
    DrawInitScreen ();
    ReadData();
    
    // Local variables

    uint32_t lastReadTime = 0;
    uint32_t lastUpdateTime = 0;

    int16_t lastPressure = 0;
    int16_t int_pressure = 0;
    int16_t counter = 0;
    while (true)
    {
        if (lastReadTime == 0 || millis() - lastReadTime > SAMPLE_TIME / 8)
        {
            lastReadTime = millis();
            int_pressure = (int16_t)(bmp.readPressure() / 10.0);
            int_pressure = FilterBaro (int_pressure);
        }        
      
        if (lastUpdateTime == 0 || millis() - lastUpdateTime > SAMPLE_TIME )
        {
            lastUpdateTime = millis();
            

            // Update the value only if its changed
            if (lastPressure != int_pressure)
            {
                lastPressure = int_pressure;
                UpdateBaro (int_pressure);
            }

            // update the graph
            DrawBaro (int_pressure);

            if (++counter%20 == 0)  // store every hour
                StoreData();
        }      
        delay(100);
    }
}
