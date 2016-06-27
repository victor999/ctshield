#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdio.h>
#include "EmonLib.h"                   // Include Emon Library
#include "ESP8266.h"

#define HOST_PORT   (80)

//define ESP8266 communication port and baudrate
ESP8266 wifi(Serial2, 115200);

//create display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define NUM_OF_SENSORS 3

EnergyMonitor emon[NUM_OF_SENSORS];                   // Create an energyMonitor instances

//measure data
struct g_dataToSendType
{
  int enabled;
  float irms;
  float realPower;
  float apparentPower;
  float powerFactor;
  float supplyVoltage;
} g_dataToSend[NUM_OF_SENSORS];

float g_totalPowerReal = 0.0;
float g_totalCurrent = 0.0;
float maxCurrent[NUM_OF_SENSORS];

char g_SSID[] = "YOUR_WIFI_SSID";
char g_PASSWORD[] = "YOUR_WIFI_PASSWORD";
char inputBuffer[256] = "";
char workBuffer[50] = "";

unsigned int bootLoopCount = 5;
#define BOOT_LOOP_FACTOR 40000

//website data
char g_hostName[256] = "emoncms.org";
char g_webSite[256] = "http://emoncms.org/emoncms";
char g_apiKey[256] = "api_key";

char g_espStatus[50] = "WIFI OK";
char g_connectionStatus[50] = "Connection OK";
char g_sendBuffer[500] = "";
uint8_t g_rcvBuffer[500] = "";
char g_sensorString[100] = "";

//counter for loop
unsigned int g_loopCounter = 0;

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup()   
{
  //////////////////////////////////////////////////////////////
  //init serial
  Serial.begin(9600);    
  //////////////////////////////////////////////////////////////  

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
  // init done

  //////////////////////////////////////////////////////////////
  // Clear the buffer.
  display.clearDisplay();

  
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,20);
  
  display.println("Starting...");
  
  display.display();

  //reset data array
  for(int i = 0; i < NUM_OF_SENSORS; i ++)
  {
    memset(&g_dataToSend[i], 0, sizeof(g_dataToSend[i]));
    g_dataToSend[i].enabled = 1;
    maxCurrent[i] = 30.0;
  }  

  emon[0].current(0, maxCurrent[0]);             // Current: input pin, calibration.
  emon[1].current(2, maxCurrent[1]);             // Current: input pin, calibration.
  emon[2].current(4, maxCurrent[2]);             // Current: input pin, calibration.
  emon[0].voltage(7, 230.0, 1.7);  // Voltage: input pin, calibration, phase_shift
  emon[1].voltage(7, 230.0, 1.7);  // Voltage: input pin, calibration, phase_shift
  emon[2].voltage(7, 230.0, 1.7);  // Voltage: input pin, calibration, phase_shift
  
  //////////////////////////////////////////////////////////////  
  //print firmware version
  display.clearDisplay();
  //get the firmware version from wifi board
  display.println("FW Ver.");
  display.println(wifi.getVersion().c_str());

  display.display();
  delay(2000);
  
  connectWiFi();
  
  delay(2000);
}

void connectWiFi()
{
  //set operation mode
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,20);
  
  if (wifi.setOprToStationSoftAP()) 
  {
    display.println("Set mode success");
  } 
  else 
  {
    display.println("Error set mode");
  }

  //join AP
  if (wifi.joinAP(g_SSID, g_PASSWORD)) 
  {
    display.println("Join AP success");
    //display.println(wifi.getIPStatus().c_str());       
  } 
  else 
  {
    display.println("Join AP failure");
  }

  //disable MUX
  if (wifi.disableMUX()) 
  {
    display.println("Single OK");
  } 
  else 
  {
    display.println("Single err");
  }

  display.display();
}

void loop() 
{
  //increment counter
  g_loopCounter ++;

  //calculate values
  
  g_totalPowerReal = 0.0;
  g_totalCurrent = 0.0;
  
  for(int i = 0; i < NUM_OF_SENSORS; i ++)
  {
    if(g_dataToSend[i].enabled)
    {
      emon[i].calcVI(20,2000);         // Calculate all. No.of half wavelengths (crossings), time-out
      g_dataToSend[i].realPower       = emon[i].realPower;        //extract Real Power into variable
      g_dataToSend[i].apparentPower   = emon[i].apparentPower;    //extract Apparent Power into variable
      g_dataToSend[i].powerFactor     = emon[i].powerFactor;      //extract Power Factor into Variable
      g_dataToSend[i].supplyVoltage   = emon[i].Vrms;             //extract Vrms into Variable
      g_dataToSend[i].irms            = emon[i].Irms;             //extract Irms into Variable
    } 
    g_totalPowerReal += g_dataToSend[i].realPower;
    g_totalCurrent += g_dataToSend[i].irms;
  }
  
  //check the timeout
  if((g_loopCounter % 20) == 0)
  { 
    //check WIFI status
    checkWifi();   
    //send data
    sendData();
  }

  //display status
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,0);
  display.println(g_espStatus);
  display.println(g_connectionStatus);

  //display power
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(0,18);

  display.print(g_totalPowerReal);
  display.print(" W\n");
  
  display.setTextSize(1);

  //display current
  display.setCursor(0,35);

  display.print(g_totalCurrent);
  display.print(" A\n");
  
  //display voltage
  display.setCursor(0,45);
  display.print(emon[0].Vrms);
  display.print(" V ");

  /*
  //display power
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0,20);

  display.print("Est ");
  display.print(g_dataToSend[0].power);
  display.print(" W\n");
  
  display.print("Real ");
  display.print(emon[0].realPower);
  display.print(" W\n");
  
  display.print("App ");
  display.print(emon[0].apparentPower);
  display.print(" W\n");
  
  display.print(emon[0].Vrms);
  display.print(" V ");
  
  display.print(emon[0].Irms);
  display.print(" A\n");

  display.print(g_dataToSend[0].irms);
  display.print(" A\n");
  */

  display.display();
}

void sendData()
{
  int len = 0;
  
  memset(g_sendBuffer, 0, sizeof(g_sendBuffer));
  memset(g_sensorString, 0, sizeof(g_sensorString));
  for(int i = 0; i < NUM_OF_SENSORS; i ++)
  {
    if(i)
      len = strlen(g_sensorString);
    sprintf(g_sensorString + len, "power%d:%d,", i + 1, (int)g_dataToSend[i].realPower);
  }
  sprintf(g_sendBuffer, "GET /emoncms/input/post.json?apikey=%s&node=1&csv=%spower_total:%d HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", g_apiKey, g_sensorString, (int)g_totalPowerReal, g_hostName);

  if(wifi.createTCP(g_hostName, HOST_PORT))
  {
    sprintf(g_connectionStatus, "Connection OK");
    
    Serial.println("Create TCP OK");
    
    Serial.print(g_sendBuffer);
    Serial.println("\n");
    
    wifi.send((const uint8_t*)g_sendBuffer, strlen(g_sendBuffer));
    
    len = wifi.recv(g_rcvBuffer, sizeof(g_rcvBuffer), 10000);
    
    if(len > 0)
    {
      Serial.println("Received");
      Serial.println((char*)g_rcvBuffer);
    }
    
    if(wifi.releaseTCP())
    {
      Serial.println("Release TCP OK");
    }
    else
    {
      Serial.println("Failed Release TCP");
    }
  }
  else
  {
    Serial.println("Failed Create TCP");
    sprintf(g_connectionStatus, "Connection FAIL"); 

    wifi.restart();
     
    connectWiFi();
  } 
}

void checkWifi()
{
  Serial.println("Check WIFI start");
  
  if(!wifi.kick())
  {
    Serial.println("WIFI problem - restart");
    
    wifi.restart();
    
    Serial.println("WIFI restarted");
  }
  
  Serial.println("Check WIFI end");
}
