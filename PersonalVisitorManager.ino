#include <WiFi.h>
#include <WebServer.h> 
#include "webpage.h"
#include <ESPmDNS.h>
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <TM1637Display.h>
#include <Adafruit_NeoPixel.h>
#include "freertos/FreeRTOS.h"
#include <ElegantOTA.h>

#define LOCAL_SSID "YOUR_SSID"
#define LOCAL_PASS "YOUR_PASSWORD"

#define CLK                   1
#define DIO                   2
#define BLUE_LED              7
#define RGB_EN                19
#define WS2812_PIN            20
#define NUM_PIXELS            1
#define SWITCH_PIN            9

/* Display related definitions */
#define NUM_OF_DIGITS         4
#define NUM_OF_PIXELS         7
#define BRIGHTNESS_LEVEL_0    0
#define BRIGHTNESS_LEVEL_1    1
#define BRIGHTNESS_LEVEL_2    2
#define BRIGHTNESS_LEVEL_3    3
#define BRIGHTNESS_LEVEL_4    4
#define BRIGHTNESS_LEVEL_5    5
#define BRIGHTNESS_LEVEL_6    6
#define BRIGHTNESS_LEVEL_7    7

/* Definitions for Switch 1 */
#define BUSY                  0x01
#define AVAILABLE             0x00
#define BUSY_BIT              0x80

/* Definitions for Switch 2 */
#define NO                    0x00
#define YES                   0x01
#define COME_TO_MEET          0x02

#define SHOW_COLON            0x40
#define DONT_SHOW_COLON       0x00

#define POS_EDGE              0x01
#define NEG_EDGE              0x02

#define WIFI_CONNECTED        1
#define WIFI_DISCONNECTED     0

#define SHORT_PRESS           0x01
#define DOUBLE_PRESS          0x02
#define LONG_PRESS            0x80

#define MAX_MULTIPLIER_VALUE  100
#define MIN_MULTIPLIER_VALUE  0  
#define INCREASE_BRIGHTNESS   1
#define DECREASE_BRIGHTNESS   0

QueueHandle_t rgbLEDQueue;
QueueHandle_t blueLEDQueue;
QueueHandle_t switchIntrQueue;

static SemaphoreHandle_t mutexTimeInfo;
static SemaphoreHandle_t mutexCurrState;
static SemaphoreHandle_t mutexOtherVariables;

struct tm globalTimeInfo;
uint8_t globalCurrState = 0;
uint8_t globalRequestToMeet = 0, globalIsBusy = 0;

const uint8_t busy[] = 
{
  SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  // b
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,  // U
  SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,  // S
  SEG_B | SEG_C | SEG_D | SEG_F | SEG_G   // y
};

const uint8_t pixel[] =
{
  SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G
};

/* XML needs 64 characters, keep 100 bytes in buffer to be on safe side */
char XML[100];
char buf[32];

WebServer server(80);
TM1637Display display = TM1637Display(CLK, DIO);
Adafruit_NeoPixel strip(NUM_PIXELS, WS2812_PIN,
                          NEO_GRB + NEO_KHZ800);

/* Interrupt Service Routine */
static void IRAM_ATTR switchISR(void);

/* FreeRTOS Tasks */
void rgbTask(void *arg);
void switchTask(void *arg);
void serverTask(void *arg);
void displayTask(void *arg);
void blueLEDTask(void *arg);

/* Function prototypes for web server callbacks */
void sendXML(void);
void sendNotFound(void);
void sendHomePage(void);
void displayScroll(void);
void processButton0(void);
void processButton1(void);
void dispTimeOnUART(void);
void dispTimeOnDisplay(uint8_t busyFlag);

/* Function prototypes for OTA callbacks */
void onOTAStart(void);
void onOTAEnd(bool success);
void onOTAProgress(size_t current, size_t final);

/* WiFi event callbacks */
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
void WiFiConnLost(WiFiEvent_t event, WiFiEventInfo_t info);
void WiFiConnReestablished(WiFiEvent_t event, WiFiEventInfo_t info);

void setup() 
{
  Serial.begin(115200);

  /* Create a mutex */
  mutexTimeInfo = xSemaphoreCreateMutex();
  mutexCurrState = xSemaphoreCreateMutex();
  mutexOtherVariables = xSemaphoreCreateMutex();

  /* Create tasks */
  xTaskCreate(blueLEDTask,
              "Blue LED",
              1024,
              NULL,
              3,
              NULL
              );

  xTaskCreate(displayTask,
              "Display",
              1024,
              NULL,
              2,
              NULL
              );

  xTaskCreate(serverTask,
              "WiFi & Server",
              20480,
              NULL,
              5,
              NULL
              );

  xTaskCreate(rgbTask,
              "RGB LED",
              1024,
              NULL,
              1,
              NULL
              );

  xTaskCreate(switchTask,
              "Switch",
              2048,
              NULL,
              4,
              NULL
              );
}

void loop()
{
  
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void WiFiConnLost(WiFiEvent_t event, WiFiEventInfo_t info)
{
  uint8_t tempData = WIFI_DISCONNECTED;

  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(LOCAL_SSID, LOCAL_PASS);
  xQueueSendFromISR(blueLEDQueue, &tempData, NULL);
}

void WiFiConnReestablished(WiFiEvent_t event, WiFiEventInfo_t info)
{
  long rssi;
  uint8_t tempData = WIFI_CONNECTED;

  Serial.println("Connected to WiFi access point");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  xQueueSendFromISR(blueLEDQueue, &tempData, NULL);
}

void dispTimeOnUART(void)
{
  char timeStr[40];

  if(xSemaphoreTake(mutexTimeInfo, portMAX_DELAY) == pdTRUE)
  {
    if(getLocalTime(&globalTimeInfo)) 
    {
      strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %H:%M:%S", &globalTimeInfo);
      Serial.println(timeStr);
    }
    else 
    {
      Serial.println("Failed to obtain local time");
    }
    xSemaphoreGive(mutexTimeInfo);
  }
}

void displayScroll(void)
{
  uint8_t index;
  static uint8_t count = 0;
  uint8_t segment[NUM_OF_DIGITS];

  display.clear();

  for(index = 0; index < NUM_OF_DIGITS; index++)
  {
    segment[index] = pixel[count];
  }

  display.setSegments(segment);
  count++;
  count %= NUM_PIXELS;
}

void dispTimeOnDisplay(uint8_t busyFlag)
{
  static uint16_t timeToDisplay;
  static uint8_t flag = SHOW_COLON;
  static uint16_t previousTime = 0;
  static uint16_t previousMinute = 61;

  if((busyFlag == BUSY) && (flag == SHOW_COLON))
  {
    /* Do nothing, the display has everything to show
       it will bypass the below else segment and will 
       not update the display, keeping the colon on */
  }
  else
  {
    if(flag == SHOW_COLON)
    {
      flag = DONT_SHOW_COLON;
      previousTime = 0;
    }
    else
    {
      flag = SHOW_COLON;
      previousTime = 0;
    }
  }

  if(xSemaphoreTake(mutexTimeInfo, portMAX_DELAY) == pdTRUE)
  {
    if(getLocalTime(&globalTimeInfo))
    {
      if(previousMinute != globalTimeInfo.tm_min)
      {
        previousMinute = globalTimeInfo.tm_min;
        
        /* Display hour in 12-hour format */
        if(globalTimeInfo.tm_hour == 0)
        {
          timeToDisplay = 12;
        }
        else
        {
          timeToDisplay = globalTimeInfo.tm_hour;  
        }

        if(globalTimeInfo.tm_hour > 12)
        {
          timeToDisplay = globalTimeInfo.tm_hour - 12;
        }

        timeToDisplay = (timeToDisplay * 100) + globalTimeInfo.tm_min;
      } 

      /* Update display if time has changed */
      if(previousTime != timeToDisplay)
      {
        previousTime = timeToDisplay;
          display.showNumberDecEx(timeToDisplay, flag, true, 4, 0); 
      }
    }
    xSemaphoreGive(mutexTimeInfo);
  }
}

void displayTask(void *arg)
{
  display.setBrightness(BRIGHTNESS_LEVEL_0);
  display.clear();

  while(1)
  {
    if(xSemaphoreTake(mutexOtherVariables, portMAX_DELAY) == pdTRUE)
    {
      dispTimeOnDisplay((globalIsBusy & BUSY) == BUSY);
      xSemaphoreGive(mutexOtherVariables);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void blueLEDTask(void *arg)
{
  static bool ledOff = 0;
  uint8_t queueDataReceived;
  uint16_t onPeriod = 75, offPeriod = 75;

  blueLEDQueue = xQueueCreate(5, sizeof(uint8_t));
  if(blueLEDQueue == NULL)
  {
    Serial.println("Failed to create Blue LED queue");
    while(1);
  }

  pinMode(BLUE_LED, OUTPUT);

  while(1)
  {
    /* Receive data from queue */
    if(xQueueReceive(blueLEDQueue, &queueDataReceived, 0))
    {
      if(queueDataReceived == WIFI_CONNECTED)
      {
        onPeriod = 2;
        offPeriod = 998;
      }
      else if(queueDataReceived == WIFI_DISCONNECTED)
      {
        onPeriod = 75;
        offPeriod = 75;
      }

      else if(queueDataReceived == LONG_PRESS)
      {
        ledOff = !ledOff;
        if(ledOff == true)
        {
          digitalWrite(BLUE_LED, false);
        }
      }
      else 
      {
        onPeriod = 500;
        offPeriod = 500;  
      }
    }

    if(ledOff == false)
    {
      /* Turn on blue LED */
      digitalWrite(BLUE_LED, true);
      vTaskDelay(onPeriod / portTICK_PERIOD_MS);

      /* Turn off blue LED */
      digitalWrite(BLUE_LED, false);
      vTaskDelay(offPeriod / portTICK_PERIOD_MS);
    }
    else
    {
      /* Just kill some time and recheck if anything changed,
      we could have even suspended here, but unfortunately
      it is not working in Arduino IDE */
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}

void rgbTask(void *arg)
{
  uint8_t busyFlag, meetFlag, numb; 
  uint8_t state, multiplier;
  uint8_t red, green, blue, direction = 0;
  float redEff, greenEff, blueEff;

  rgbLEDQueue = xQueueCreate(5, sizeof(uint8_t));

  if(rgbLEDQueue == NULL)
  {
    Serial.println("Failed to create RGB LED queue");
    while(1);
  }

  pinMode(RGB_EN, OUTPUT);
  digitalWrite(RGB_EN, HIGH);
  
  /* Initialize RGB LED with the initial state */
  red   = 0;
  green = 255;
  blue  = 0;
  
  strip.begin();
  strip.show();

  while(1)
  {
    if(xQueueReceive(rgbLEDQueue, &numb, 0))
    {
      /* If the MSB is set (0x80) then the queue 
      data is indicating the status of busy bit */
      if((numb & BUSY_BIT) != 0)
      {
        busyFlag = numb & 0x01;
      }
      /* If the MSB is not set (0x80) then the queue 
        data is indicating the status of meet bit */
      else
      {
        meetFlag = (numb << 1);
      }

      if(xSemaphoreTake(mutexCurrState, portMAX_DELAY) == pdTRUE)
      {   
        globalCurrState = meetFlag | busyFlag;

        switch(globalCurrState)
        {
          /* Available & No */
          case 0x00:
            red   = 0;
            green = 255;
            blue  = 0;
          break;
          
          /* Busy & No */
          case 0x01:
            red   = 255;
            green = 0;
            blue  = 0;  
          break;
          
          /* Busy & Yes */
          case 0x03:
            red   = 0;
            green = 0;
            blue  = 255;
          break;

          /* Busy & Come to meet */
          case 0x05:
            red   = 255;
            green = 104;
            blue  = 0;
          break;

          default:
            red   = 0;
            green = 0;
            blue  = 0;
          break;
        }
        xSemaphoreGive(mutexCurrState);
      }
    } 

    if(direction == INCREASE_BRIGHTNESS)
    {
      multiplier++;
    }
    else
    {
      multiplier--;
    }

    if(multiplier == MAX_MULTIPLIER_VALUE)
    {
      direction = DECREASE_BRIGHTNESS;
    }

    if(multiplier == MIN_MULTIPLIER_VALUE)
    {
      direction = INCREASE_BRIGHTNESS;
    }

    /* Calculate effective RGB values */
    redEff   = (float) (red   * multiplier);
    greenEff = (float) (green * multiplier);
    blueEff  = (float) (blue  * multiplier);
    redEff   /= 255;
    greenEff /= 255;
    blueEff  /= 255;

    /* Set pixel color */
    strip.setPixelColor(0, strip.Color((uint8_t)redEff, 
                      (uint8_t)greenEff, (uint8_t)blueEff));
    strip.show();
    vTaskDelay(18 / portTICK_PERIOD_MS);
  }
}

void switchTask(void *arg)
{
  uint8_t lastEdgeDetected, negEdgeCount, lastLevel, switchQueue;
  uint8_t switchLowCount, switchHighCount,  continueSwitchLoop;
  uint32_t loopCount = 0;

  switchIntrQueue = xQueueCreate(5, sizeof(uint8_t));

  if(switchIntrQueue == NULL)
  {
    Serial.println("Failed to create Switch interrupt queue");
    while(1);
  }

  /* Initialize GPIOs and related interrupt */
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  attachInterrupt(SWITCH_PIN, switchISR, FALLING);

  while(1)
  {
    if(xQueueReceive(switchIntrQueue, &switchQueue, portMAX_DELAY))
    {
      loopCount = 0;
      lastLevel = HIGH;
      negEdgeCount = 0;
      switchLowCount = 0;
      switchHighCount = 0;
      continueSwitchLoop = true;
      lastEdgeDetected = NEG_EDGE;

      while(continueSwitchLoop == true)
      {
        /* Switch Released */
        if(digitalRead(SWITCH_PIN) == HIGH)
        {
          if(lastLevel == LOW)
          {
            lastEdgeDetected = POS_EDGE;
            lastLevel = HIGH;
            /* If the switch is released in less 
            than 20 ms then terminate the loop */
            if((loopCount < 4) && (negEdgeCount == 1))
            {
              continueSwitchLoop = false;
            }
          }
          switchHighCount++;
        }
        /* Switch pressed */
        else
        {
          if(lastLevel == HIGH)
          {
            lastLevel = LOW;
            negEdgeCount++;
            switchHighCount = 0;
            lastEdgeDetected = NEG_EDGE;
          }
          switchLowCount++;
        }

        /* If switch is released for more than 500ms */
        if(switchHighCount > 100)
        {
          /* Break the loop */
          continueSwitchLoop = false;
        }

        /* Press and hold for 500 ms*/
        if((switchLowCount > 100) && (negEdgeCount == 1))
        {
          negEdgeCount = LONG_PRESS;
        }

        /* Disable the Swith interrupt */
        if(negEdgeCount > 0)
        {
          detachInterrupt(SWITCH_PIN);
        }
        loopCount++;
        vTaskDelay(5 / portTICK_PERIOD_MS);
      }
      
      /* Re-enable the interrupt */
      attachInterrupt(SWITCH_PIN, switchISR, FALLING);

      if(xSemaphoreTake(mutexOtherVariables, portMAX_DELAY) == pdTRUE)
      {
        if((negEdgeCount == SHORT_PRESS) && (globalIsBusy == (BUSY | BUSY_BIT)))
        {
          globalRequestToMeet = YES;
          xQueueSend(rgbLEDQueue, &globalRequestToMeet, 0);
          server.send(200, "text/plain", "");
        }
        else if((negEdgeCount == DOUBLE_PRESS) && (globalIsBusy == (BUSY | BUSY_BIT)))
        {
          globalRequestToMeet = NO;
          xQueueSend(rgbLEDQueue, &globalRequestToMeet, 0);
          server.send(200, "text/plain", "");
        }
        else if(negEdgeCount == LONG_PRESS)
        {
          xQueueSend(blueLEDQueue, &negEdgeCount, 0);
        }
        xSemaphoreGive(mutexOtherVariables);
      }
    }
  } 
}

void serverTask(void *arg)
{
  uint8_t ledQueueData = WIFI_DISCONNECTED;
  const long gmtOffsetSec = 3600 * 5.5;
  const int daylightOffsetSec = 0;

  xQueueSend(blueLEDQueue, &ledQueueData, NULL);

  /* Configure WiFi events */
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiConnLost, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(WiFiConnReestablished, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  
  WiFi.begin(LOCAL_SSID, LOCAL_PASS);
  while (WiFi.status() != WL_CONNECTED) 
  {
    displayScroll();
    vTaskDelay(250 / portTICK_PERIOD_MS);
  }
 
  ledQueueData = WIFI_CONNECTED; 
  xQueueSend(blueLEDQueue, &ledQueueData, NULL);
  
  if (MDNS.begin("myBoard")) 
  {
    Serial.println("MDNS responder started");
  }

  server.on("/", sendHomePage);
  server.on("/xml", sendXML);
  server.on("/BUTTON_0", processButton0);
  server.on("/BUTTON_1", processButton1);
  server.onNotFound(sendNotFound);

  /* Configure OTA server */
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  
  configTime(gmtOffsetSec, daylightOffsetSec, "pool.ntp.org");
  time_t now = time(nullptr);
  
  while(now < 24 * 3600) 
  {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println("NTP synchronized");
  time(&now);

  dispTimeOnUART();

  while(1)
  {   
    ElegantOTA.loop();
    server.handleClient();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void processButton0(void) 
{
  if(xSemaphoreTake(mutexOtherVariables, portMAX_DELAY) == pdTRUE)
  {
    switch(globalIsBusy)
    {
      case AVAILABLE:
      case (AVAILABLE | BUSY_BIT):
        globalIsBusy = BUSY | BUSY_BIT;
      break;
      
      case (BUSY | BUSY_BIT):
        globalIsBusy = AVAILABLE | BUSY_BIT;
        globalRequestToMeet = NO;
        xQueueSend(rgbLEDQueue, &globalRequestToMeet, 0);
      break;
    }
  }
  xQueueSend(rgbLEDQueue, &globalIsBusy, 0);
  xSemaphoreGive(mutexOtherVariables);
  server.send(200, "text/plain", "");
}

void processButton1(void) 
{
  if(xSemaphoreTake(mutexOtherVariables, portMAX_DELAY) == pdTRUE)
  {
    if(globalIsBusy == (BUSY | BUSY_BIT))
    {
      switch(globalRequestToMeet)
      {
        case YES:
          globalRequestToMeet = COME_TO_MEET;
        break;

        case COME_TO_MEET:
          globalRequestToMeet = NO;
        break;

        default:  
          globalRequestToMeet = NO;
        break;
      }
      xQueueSend(rgbLEDQueue, &globalRequestToMeet, 0);
    }
    xSemaphoreGive(mutexOtherVariables);
  }
  server.send(200, "text/plain", "");
}

void sendHomePage(void) 
{
  Serial.println("Sending Homepage");
  server.send(200, "text/html", PAGE_MAIN);
}

void sendNotFound(void) 
{
  Serial.println("Sending 404 page");
  server.send(404, "text/html", NOT_FOUND_PAGE);
}

void sendXML(void) 
{
  strcpy(XML, "<?xml version = '1.0'?>\n<Data>\n");

  if(xSemaphoreTake(mutexCurrState, portMAX_DELAY) == pdTRUE)
  {
    if((globalCurrState & 0x01) == 1) 
    {
      strcat(XML, "<SW1>1</SW1>\n");
    } 
    else 
    {
      strcat(XML, "<SW1>0</SW1>\n");
    }

    if((globalCurrState & 0x06) == 4) 
    {
      strcat(XML, "<SW2>2</SW2>\n");
    } 
    else if((globalCurrState & 0x06) == 2)
    {
      strcat(XML, "<SW2>1</SW2>\n");
    }
    else
    {
      strcat(XML, "<SW2>0</SW2>\n");
    }
    xSemaphoreGive(mutexCurrState);
  }
  strcat(XML, "</Data>\n");
  server.send(200, "text/xml", XML);
}

void onOTAStart(void) 
{
  Serial.println("OTA update started!");
}

void onOTAProgress(size_t current, size_t final) 
{
  static unsigned long otaProgressMillis = 0;

  if (millis() - otaProgressMillis > 1000) 
  {
    otaProgressMillis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) 
{
  if (success) 
  {
    Serial.println("OTA update finished successfully!");
  } 
  else 
  {
    Serial.println("There was an error during OTA update!");
  }
}

static void IRAM_ATTR switchISR(void)
{
  uint8_t queueData = true;
  xQueueSendFromISR(switchIntrQueue, &queueData, NULL);
}
