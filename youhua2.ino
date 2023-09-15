#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ELMduino.h>
#include <BluetoothSerial.h>

// SPI pin definitions
#define OLED_MISO           32
#define OLED_MOSI           23
#define OLED_SCLK           18
#define OLED_CS             5
#define OLED_DC             19
#define OLED_RST            4
#define OLED_PWR            33

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_SCLK, OLED_DC, OLED_RST, OLED_CS);
BluetoothSerial ELM_PORT;
ELM327 myELM327;


float rpm = 0;
float engineLoadVal = 0; 
float speedKph = 0;

float displayedRPM = 0;
float displayedEngineLoadVal = 0;
float displayedSpeedKph = 0;

enum DataState {
    GET_RPM,
    GET_ENGINE_LOAD,
    GET_SPEED
};

DataState currentState = GET_RPM;

void setup() {

  // Initialize OLED power pin
  pinMode(OLED_PWR, OUTPUT);
  digitalWrite(OLED_PWR, HIGH);  // Turn ON OLED display
  Serial.begin(115200);
  delay(1000);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("Starting..."));
  display.display();

  // Initialize Bluetooth
  if (!ELM_PORT.begin("ESP32_BT", true)) {
    display.clearDisplay();
    display.print(F("BT Init failed!"));
    display.display();
    return;
  }

  uint8_t address[6] = {0x5C, 0x5B, 0xB0, 0xA7, 0x06, 0xE6};
  if (!ELM_PORT.connect(address)) {
    display.clearDisplay();
    display.print(F("BT conn failed!"));
    display.display();
    return;
  }

  if (!myELM327.begin(ELM_PORT, true, 2000)) {
    display.clearDisplay();
    display.print(F("ELM327 init failed!"));
    display.display();
    return;
  }

  display.clearDisplay();
  display.print(F("BT Connected"));
  display.display();
}

void getRPM() {
    float tempRPM = myELM327.rpm();
    if (myELM327.nb_rx_state == ELM_SUCCESS) {
        rpm = tempRPM;
        Serial.print("RPM: "); Serial.println(rpm);
        currentState = GET_ENGINE_LOAD;  // Move to the next state
    } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
    }
}

void getEngineLoad() {
    float tempEngineLoad = myELM327.engineLoad();
    if (myELM327.nb_rx_state == ELM_SUCCESS) {
        engineLoadVal = tempEngineLoad;
        Serial.print("Engine Load: "); Serial.println(engineLoadVal);
        currentState = GET_SPEED;  // Move to the next state to get speed

    } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
    }
}

void getSpeed() {
    float tempSpeedKph = myELM327.kph(); // 获取车速（以千米/小时为单位）
    if (myELM327.nb_rx_state == ELM_SUCCESS) {
        speedKph = tempSpeedKph; // 存储千米/小时的车速
        Serial.print("Speed (KPH): ");
        Serial.println(speedKph); // 打印千米/小时的车速
        currentState = GET_RPM;  // Move to the next state
    } else if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
        myELM327.printError();
    }
}
void loop() {
    static unsigned long lastUpdateTime = 0;
    unsigned long currentTime = millis();

    // Update every 50 ms
    if (currentTime - lastUpdateTime >= 50) {
        lastUpdateTime = currentTime;

        // Update sensor data
        switch (currentState) {
            case GET_RPM:
                getRPM();
                break;

            case GET_ENGINE_LOAD:
                getEngineLoad();
                break;

            case GET_SPEED: // 改为获取车速
                getSpeed();  // 调用获取车速的函数
                break;
        }
        
        // Update display
        updateDisplay();
    }
}

void updateDisplay() {
  
    display.clearDisplay();
    display.setTextSize(2);
    
    // Calculate vertical positions for each data
    int yPosRPM = 3;
    int yPosLoad = 20;
    int yPosParo = 42;
    
    display.setCursor(0, yPosRPM);
    display.print("RPM: ");
    display.print(int(rpm));
    
    display.setCursor(0, yPosLoad);
    display.print("Load: ");
    display.print(int(engineLoadVal));  // 显示为整数
    display.setTextSize(1);
    display.print(" %");
    
    display.setTextSize(2);  // 恢复文字大小为2
    display.setCursor(0, yPosParo);  // 设置光标位置为车速
    display.print("KPH: ");
    display.print(int(speedKph));  // 修改为实际的车速值
    display.setTextSize(1);  // 设置单位的较小文字大小
    display.print(" km/h");  // 以km/h为单位显示车速
    
    display.display();
}
