#include <Wire.h>

//-------------------------------------------------
//
// Project BaroGraph
//
//
//
//-------------------------------------------------
char version[] = "1.01f";

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <stdio.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// ESP32 Can Setup
#define ESP32_CAN_TX_PIN GPIO_NUM_16
#define ESP32_CAN_RX_PIN GPIO_NUM_17

#include <Arduino.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <N2kMessages.h>

#include "Free_Fonts.h"

TFT_eSPI tft = TFT_eSPI();

// Colour Defines
#define BLACK   0x0000
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0

#define RGB(r, g, b) (((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3))

#define DARKGREY  RGB(64, 64, 64)
#define DARKGREEN RGB(0,128,0)
#define LIGHTGREEN RGB(0,255,128)

Adafruit_BME280 bmp; // I2C

//led defines
#define LED_1 GPIO_NUM_4
#define LED_2 GPIO_NUM_33
#define LED_3 GPIO_NUM_32
#define LED_4 GPIO_NUM_25

bool LED_1_State = 0;
bool LED_2_State = 0;
bool LED_3_State = 0;
bool LED_4_State = 0;

bool EEPROM_TYPE = 0;

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

const uint16_t MIN_BARO = 9600;
const uint16_t MAX_BARO = 10500;
static int paddingBaro = 0;

// eeprom data
int EepromAddr = 0x50;  


// Define READ_STREAM to port, where you write data from PC e.g. with NMEA Simulator.
#define READ_STREAM Serial       
// Define ForwardStream to port, what you listen on PC side. On Arduino Due you can use e.g. SerialUSB
#define FORWARD_STREAM Serial    

Stream *ReadStream=&READ_STREAM;
Stream *ForwardStream=&FORWARD_STREAM;

// WIFI network defines
const char* ssid = "TALKTALK4AD3F0";
const char* password = "C3AKAC4R";
 
 #define BMP 
 //#define OTA
 
//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void setup() 
{
    Serial.begin(115200);
    Serial.println("Booting");
    Serial.print ("Version ");
    Serial.println (version);
    
    // Setup the LEDS
    pinMode (LED_1 , OUTPUT);
    pinMode (LED_2 , OUTPUT);
    pinMode (LED_3 , OUTPUT);
    pinMode (LED_4 , OUTPUT);

    digitalWrite (LED_1 , LOW);
    digitalWrite (LED_2 , LOW);
    digitalWrite (LED_3 , LOW);
    digitalWrite (LED_4 , LOW);


#ifdef OTA
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) 
    {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    // Port defaults to 3232
    // ArduinoOTA.setPort(3232);

    // Hostname defaults to esp3232-[MAC]
    // ArduinoOTA.setHostname("myesp32");

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

// Over the Air Updates
    ArduinoOTA
        .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
        })
        .onEnd([]() {
            Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) 
        {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });

    ArduinoOTA.begin();

    Serial.println("Wifi Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    digitalWrite (LED_4 , HIGH);// wifi started
    digitalWrite (LED_3 , LOW);// wifi started

#endif
    // Start the TFT
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(BLACK);
    tft.setTextDatum (TL_DATUM);

    tft.setFreeFont(FSS12);
 
    paddingBaro = tft.textWidth ("9999.9" , 1);

    tft.setFreeFont (FSS9);
    // Set up the Barometer chip
    /* Default settings from datasheet. */
#ifdef BMP
    if (!bmp.begin(0x76))
    {
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    }
#endif
    Wire.begin();
    Wire.setClock(400000);


    // make a unique number from the mac address
    uint8_t mac[6];
    WiFi.macAddress (mac);
    unsigned long uniqueId = (mac[3] << 16) | (mac[4] <<  8) | mac[5];

    // setup pwm
    // channel , freq , resolution
    //ledcSetup (0,5000,8);

    // Can Bus Set up
    // Reserve enough buffer for sending all messages. 
    NMEA2000.SetN2kCANSendFrameBufSize(250);
    // Set Product information
    NMEA2000.SetProductInformation("00000001", // Manufacturer's Model serial code
                                 100, // Manufacturer's product code
                                 "Barograph ESP32",  // Manufacturer's Model ID
                                 "1.0.0.0 (2020-08-15)",  // Manufacturer's Software version code
                                 "1.0.2.0 (2019-08-15)" // Manufacturer's Model version
                                 );
    // Set device information
    NMEA2000.SetDeviceInformation(uniqueId, // Unique number. Use e.g. Serial number.
                                132, // Device function=Analog to NMEA 2000 Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                25, // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf                               
                               );
    
    // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
    NMEA2000.SetForwardStream(&Serial);
    NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly , 23);
    NMEA2000.EnableForward(false); // Disable all msg forwarding to USB (=Serial)
    NMEA2000.SendProductInformation (0xff);
    NMEA2000.Open();
   
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void SendPressure (uint16_t baro)
{
    Serial.println("Sending Baro");
    tN2kMsg N2kMsg;
    SetN2kPressure(N2kMsg,0,2,N2kps_Atmospheric,baro*10);
    NMEA2000.SendMsg(N2kMsg);
}
//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void DrawInitScreen()
{
    // put a box on the screen
    Serial.print ("draw init screen");
    tft.setFreeFont (FSS12);

    tft.fillScreen (BLACK);
    tft.fillRect (0, 0 , 479 , 4 , GREEN);      // Top Line
    tft.fillRect (0, 316 , 479 , 4 , GREEN);    // Bottom Line
    tft.fillRect (0 , 4 , 4 , 319 , GREEN);     // Left Line
    tft.fillRect (475 , 0 , 4 , 319 , GREEN);   // Right Line
    tft.fillRect (0, 66 , 475 , 4 , GREEN);     // Horiz Divider
    tft.fillRect (66 , 70 , 2 , 250 , GREEN);   // Vertical Divider

    tft.setTextColor(YELLOW);
    tft.setFreeFont (FSS9);
    tft.setTextSize(1);

    tft.setCursor (10, 30);
    tft.print("Baro");
    tft.setCursor (140, 30);
    tft.print("High");
    tft.setCursor (10, 60);
    tft.print("Trend");
    tft.setCursor (260, 30);
    tft.print("Low");
    tft.setCursor (410, 30);
    tft.print("24 Hrs");
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void UpdateDelta (int16_t delta)
{
 
    int16_t x = 290 + paddingBaro;
    int16_t y = 42;

    tft.setFreeFont(FSS12);
    tft.setTextColor(CYAN , BLACK);
    tft.setTextDatum (TR_DATUM);
    tft.setTextPadding (paddingBaro);
    tft.drawFloat (delta / 10.0 , 1 , x , y , 1);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void UpdateLow (uint16_t low)
{
    tft.setFreeFont(FSS12);
    tft.setTextColor(CYAN , BLACK);
    tft.setTextDatum (TR_DATUM);
    int16_t x = 298 + paddingBaro;
    int16_t y = 12;
    tft.setTextPadding (paddingBaro);
    tft.drawFloat (low / 10.0 , 1 , x , y , 1);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void UpdateHigh (uint16_t high)
{   
    tft.setFreeFont(FSS12);
    tft.setTextColor(CYAN , BLACK);
    tft.setTextDatum (TR_DATUM);
    int16_t x = 180 + paddingBaro;
    int16_t y = 12;
    tft.setTextPadding (paddingBaro);
    tft.drawFloat (high / 10.0 , 1 , x , y , 1);
}

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void UpdateTrend (int16_t baro)
{
    tft.setFreeFont(FSS12);
    static int16_t padding = tft.textWidth (" Falling Rapidlyy ");
    // get last update and 3 hours before
    int16_t offset = m_baroDataHead - POINTS_PER_DAY / 8;
    if (offset < 0) offset += POINTS_PER_DAY; 
    int16_t threeHours = m_baroDataArray[offset] ;
    if (threeHours < MIN_BARO || threeHours > MAX_BARO)
        threeHours = baro;
            
    int16_t diff = baro - threeHours;
    
    UpdateDelta(diff);
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

    int16_t x = 80;
    int16_t y = 42;
    
    tft.setTextColor(MAGENTA , BLACK);
    tft.setTextDatum (TL_DATUM);
    tft.setTextPadding (padding);
    tft.drawString (f_r , x , y , 1);
}


//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void UpdateBaro (int16_t baro)
{
    UpdateTrend (baro);

    uint16_t high = baro;
    uint16_t low = baro;
    uint16_t range = 0;;
    GetHighLowRange(high , low , range);

    UpdateHigh (high);
    UpdateLow (low);

    tft.setFreeFont(FSS12);
    int16_t x = 58 + paddingBaro;
    int16_t y = 12;
    tft.setTextDatum (TR_DATUM);
    tft.setTextColor(CYAN , BLACK);
    tft.setTextPadding (paddingBaro);
    tft.drawFloat (baro / 10.0 , 1 , x , y , 1);
}


//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void GetHighLowRange (uint16_t& high , uint16_t &low , uint16_t &range)
{
    // loop the data and find the high and low
    range = 0;    
    
    for (uint16_t i = 0 ; i < BARO_ARRAY_SIZE ; i++)
    {
        uint16_t value = m_baroDataArray[i];
        if (value > MIN_BARO && value < MAX_BARO)
        {
            if (value < low ) 
                low = value;
            else if (value > high) 
                high = value;
        }        
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
    else if (range < 300) range = 300;
    else if (range < 400) range = 400;
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
    uint16_t yPos = 298;

    tft.setFreeFont(FSS9);
    tft.setTextColor(CYAN , BLACK);
    tft.setTextPadding (paddingBaro-20);
    tft.setTextDatum (TR_DATUM);

    for (int i = 0 ; i <= 5 ; i++)
    {
        tft.drawFloat (baro / 10.0 , 1 , paddingBaro-8 , yPos , 1);
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
    static uint16_t lastHigh = 0 , lastLow = 0, lastRange = 0;
    high = baro;
    low = baro;
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
                if (x == 420)
                    tft.drawFastVLine (x , TOP_GRAPH , GRAPH_HEIGHT , YELLOW);
                else if ((x-20)%50 == 0 )
                    tft.drawFastVLine (x , TOP_GRAPH , GRAPH_HEIGHT , DARKGREEN);
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
void WriteEEPROM(int deviceaddress, unsigned int eeaddress, byte data ) 
{
    if (EEPROM_TYPE == 0)
    {
        WriteEEPROM0 (deviceaddress , eeaddress , data);
    }
    else
    {
        WriteEEPROM1 (deviceaddress , eeaddress , data);
    }
}

//----------------------------------------
//
//----------------------------------------
byte ReadEEPROM(int deviceaddress, unsigned int eeaddress ) 
{
    if (EEPROM_TYPE == 0)
    {
        return ReadEEPROM0 (deviceaddress , eeaddress);
    }
    else
    {
        return ReadEEPROM1 (deviceaddress , eeaddress);
    }
}

//----------------------------------------
//
//----------------------------------------
void WriteEEPROM0(int deviceaddress, unsigned int eeaddress, byte data ) 
{
    int addrOffset = eeaddress >> 8;
    addrOffset <<= 1;
    deviceaddress = deviceaddress | addrOffset;
    
    Wire.beginTransmission(deviceaddress);// + addrOffset);
    //Wire.write((int)((eeaddress >> 8) & 0xff)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();
 
    delay(5);
}
 
//----------------------------------------
//
//----------------------------------------
byte ReadEEPROM0(int deviceaddress, unsigned int eeaddress ) 
{
    byte rdata = 0xFF;
    int addrOffset = eeaddress >> 8;
    addrOffset <<= 1;
    deviceaddress = deviceaddress | addrOffset;

    Wire.beginTransmission(deviceaddress);// + addrOffset);
    //Wire.write((int)((eeaddress>> 8) & 0xff)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
 
    Wire.requestFrom(deviceaddress,1);
 
    if (Wire.available()) 
        rdata = Wire.read();
    else
    {
        // ROM read failure switch to other type
        EEPROM_TYPE = 1;
        return ReadEEPROM1 (deviceaddress , eeaddress);
    }
 
    return rdata;
}


//----------------------------------------
//
//----------------------------------------
void WriteEEPROM1(int deviceaddress, unsigned int eeaddress, byte data ) 
{
    int addrOffset = eeaddress >> 8;
    addrOffset <<= 1;
    deviceaddress = deviceaddress;// | addrOffset;
    
    Wire.beginTransmission(deviceaddress);// + addrOffset);
    Wire.write((int)((eeaddress >> 8) & 0xff)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();
 
    delay(5);
}
 
//----------------------------------------
//
//----------------------------------------
byte ReadEEPROM1(int deviceaddress, unsigned int eeaddress ) 
{
    byte rdata = 0xFF;
    int addrOffset = eeaddress >> 8;
    addrOffset <<= 1;
    deviceaddress = deviceaddress;// | addrOffset;

    Wire.beginTransmission(deviceaddress);// + addrOffset);
    Wire.write((int)((eeaddress>> 8) & 0xff)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
 
    Wire.requestFrom(deviceaddress,1);
 
    if (Wire.available()) rdata = Wire.read();
 
    return rdata;
}

//----------------------------------------
//
//----------------------------------------
void StoreData ()
{
    uint16_t offset = m_baroDataHead;

    for (int i = 0 ; i < BARO_ARRAY_SIZE ; i++)
    {
        WriteEEPROM (EepromAddr , i*2 , m_baroDataArray[i] & 0xff);
        WriteEEPROM (EepromAddr , (i*2)+1 , (m_baroDataArray[i] >> 8) & 0xff);
    }
    WriteEEPROM (EepromAddr , 0x3fe , m_baroDataHead &0xff);
    WriteEEPROM (EepromAddr , 0x3ff , (m_baroDataHead >> 8) &0xff);   
}

//----------------------------------------
//
//----------------------------------------
void ReadData ()
{
    for (int i = 0 ; i < BARO_ARRAY_SIZE ; i++)
    {
        uint16_t value = ReadEEPROM (EepromAddr , i*2);
        value |= ReadEEPROM(EepromAddr , (i*2)+1) << 8;
        if (value == 0xffff || value < 9500 || value > 10500)
        {
            m_baroDataArray[i] = 10134;
        }
        else
            m_baroDataArray[i] = value;

    }
    // Read in the data write position
    m_baroDataHead = ReadEEPROM (EepromAddr , 0x3fe);
    m_baroDataHead |= (ReadEEPROM (EepromAddr , 0x3ff) << 8);
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
void PWMSetup (int led ,int channel, int duty)
{
    ledcSetup (channel , 5000 , 8);
    ledcAttachPin (led , channel);
    ledcWrite (channel , duty);
}

//----------------------------------------
//
//----------------------------------------
void SplashScreen ()
{
    const int cBackground = TFT_BLACK;
    tft.fillScreen (cBackground);
    tft.setFreeFont (FSS24);
    tft.setTextColor(TFT_SKYBLUE , cBackground);
    tft.setTextDatum (TL_DATUM);
    tft.drawString ("Barograph" , 150 , 100 , 1);

    tft.setFreeFont (FSS12);
    tft.drawString ("Version" , 150 , 150 , 1);
    tft.drawString (version , 300 , 150 , 1);
    tft.setTextColor(TFT_YELLOW , cBackground);
    tft.drawString ("Lee Playford (c) 2024" , 150 , 200 , 1);
}

//----------------------------------------
//
//----------------------------------------
void loop()
{
    SplashScreen();
    LED_1_State = HIGH;
    LED_2_State = LOW;
    
    for (int i = 0 ; i < 10 ; i++)
    {
        digitalWrite (LED_1 , LED_1_State);
        digitalWrite (LED_2 , LED_2_State);
        digitalWrite (LED_3 , LED_1_State);
        digitalWrite (LED_4 , LED_2_State);
        LED_1_State = !LED_1_State;
        LED_2_State = !LED_2_State;
        delay (500);
    }
    digitalWrite (LED_1 , LOW);
    digitalWrite (LED_2 , LOW);
    digitalWrite (LED_3 , LOW);
    digitalWrite (LED_4 , LOW);

     // draw the screen
    DrawInitScreen ();

    // Read the eeprom
    ReadData();
    // Local variables

    uint32_t lastReadTime = 0;
    uint32_t lastUpdateTime = 0;
    uint32_t lastSendTime = 0;
    int16_t lastPressure = 0;

#ifdef BMP
     int16_t int_pressure = (int16_t)(bmp.readPressure() / 10.0);
#else
    int16_t int_pressure = 10134;
#endif
 
    int16_t counter = 0;
    
    while (true)
    {
        if (lastSendTime == 0 || millis() > lastSendTime + 1000)
        {
            SendPressure(int_pressure);
            lastSendTime = millis();
            
            //digitalWrite (LED_3 , LED_1_State);
            //LED_1_State = !LED_1_State; save current
            
        }
        if (lastReadTime == 0 || millis() - lastReadTime > SAMPLE_TIME / 8)
        {
            lastReadTime = millis();
#ifdef BMP
            int_pressure = (int16_t)(bmp.readPressure() / 10.0);
#else
            int_pressure = 10134;
#endif
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
        delay(10);
#ifdef OTA
        ArduinoOTA.handle();
#endif
        NMEA2000.ParseMessages();

                
    }
}
