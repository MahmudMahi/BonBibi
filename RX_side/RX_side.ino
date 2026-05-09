#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// OLED (SH1106)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SH1106G display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

#define BUZZER_PIN 27
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 26

String clusterID = "NONE";
String confidence = "CLEAR";

int packetRSSI = 0;
float packetSNR = 0;

unsigned long packetCount = 0;
unsigned long lastPacketTime = 0;

bool alertState = false;


void setup()
{
    Serial.begin(115200);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    Wire.begin(21,22);
    display.begin(0x3C, true);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20,20);
    display.println("BOOT");
    display.display();

    delay(1000);

    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

    if(!LoRa.begin(433E6))
    {
        Serial.println("LoRa FAIL");

        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0,20);
        display.println("LORA FAIL");
        display.display();

        while(1);
    }

    Serial.println("LoRa READY");

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("BASE STATION READY");
    display.println("Monitoring Forest...");
    display.display();

    shortBeep();
}

void loop()
{
    int packetSize = LoRa.parsePacket();

    if(packetSize)
    {
        String msg = "";

        while(LoRa.available())
        {
            msg += (char)LoRa.read();
        }

        Serial.println("----------------------");
        Serial.print("RX: ");
        Serial.println(msg);

        packetRSSI = LoRa.packetRssi();
        packetSNR  = LoRa.packetSnr();

        packetCount++;
        lastPacketTime = millis();

        parseMessage(msg);

        alertState = true;

        if(confidence == "HIGH")
            highAlertBeep();
        else
            shortBeep();

        drawDisplay();
    }
    // AUTO RESET (FOREST NORMAL STATE)
    if(millis() - lastPacketTime > 15000)
    {
        alertState = false;

        clusterID = "NONE";
        confidence = "CLEAR";

        packetRSSI = 0;
        packetSNR = 0;

        drawDisplay();
    }
}

void shortBeep()
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(70);
    digitalWrite(BUZZER_PIN, LOW);
}

void highAlertBeep()
{
    for(int i = 0; i < 3; i++)
    {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(120);
        digitalWrite(BUZZER_PIN, LOW);
        delay(80);
    }
}

void drawDisplay()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0,0);
    display.println("       BONBIBI       ");
    display.drawLine(0,10,127,10,SH110X_WHITE);

    display.setCursor(0,16);
    display.print("CLUSTER : ");
    display.println(clusterID);

    display.setCursor(0,28);

    if(confidence == "HIGH")
    {
        display.print("STATUS  : ALERT");
        display.fillCircle(120, 5, 4, SH110X_WHITE);
    }
    else if(confidence == "LOW")
    {
        display.print("STATUS  : SUSPICIOUS");
        display.drawCircle(120, 5, 4, SH110X_WHITE);
    }
    else
    {
        display.print("STATUS:CLUSTER NORMAL");
    }

    display.setCursor(0,40);
    display.print("RSSI    : ");
    display.print(packetRSSI);
    display.println(" dBm");

    display.setCursor(0,52);
    display.print("PACKETS : ");
    display.println(packetCount);

    display.display();
}

void parseMessage(String msg)
{
    int n = msg.indexOf("NODE=");
    int c = msg.indexOf("CONF=");

    if(n >= 0)
    {
        int end = msg.indexOf(",", n);
        if(end == -1) end = msg.length();
        clusterID = msg.substring(n + 5, end);
    }

    if(c >= 0)
    {
        int end = msg.indexOf(",", c);
        if(end == -1) end = msg.length();
        confidence = msg.substring(c + 5, end);
    }
}