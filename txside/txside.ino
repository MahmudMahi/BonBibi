#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <driver/i2s.h>
#include <math.h>

//SW-420
#define VIBRATION_PIN 15

// I2S MIC (INMP441)
#define I2S_WS  25
#define I2S_SCK 33
#define I2S_SD  32

// LoRa
#define LORA_SS   5
#define LORA_RST  14
#define LORA_DIO0 26

//MPU6050 
#define MPU_ADDR 0x68
#define ACCEL_XOUT_H 0x3B
#define PWR_MGMT_1   0x6B

#define RMS_SAMPLES 256


const String NODE_ID = "CLUSTER_07";
float ACC_THRESHOLD = 0.2;
float RMS_THRESHOLD = 1200.0;
const int REQUIRED_SPIKES = 3;
const unsigned long WINDOW_MS = 6000;
const unsigned long COOLDOWN_MS = 300;
volatile bool vibrationFlag = false;

int spikeCount = 0;

unsigned long firstSpikeTime = 0;
unsigned long lastEventTime = 0;

void setup()
{
    Serial.begin(115200);

    delay(1000);

    Serial.println("TREE NODE START");

    pinMode(VIBRATION_PIN, INPUT);

    attachInterrupt(
        digitalPinToInterrupt(VIBRATION_PIN),
        vibrationISR,
        RISING
    );

    Wire.begin(21, 22);
    mpuWrite(PWR_MGMT_1, 0x00);
    delay(100);

    micInit();

    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(433E6))
    {
        Serial.println("LoRa FAIL");
        while (1);
    }

    Serial.println("SYSTEM READY");
}

void loop()
{
    float liveRMS = getRMS();

    Serial.print("LIVE RMS: ");
    Serial.println(liveRMS);

    if (vibrationFlag)
    {
        vibrationFlag = false;
        handleEvent();
    }

    delay(100);
}

void handleEvent()
{
    if (millis() - lastEventTime < COOLDOWN_MS)
        return;

    lastEventTime = millis();

    //Serial.println("\n--- EVENT ---");

    float dynamicAccel = 0;

    if (!mpuCheck(dynamicAccel))
    {
        Serial.println("MPU rejected event");
        return;
    }

    Serial.println("MPU OK");

    float rms = getRMS();

    Serial.print("RMS: ");
    Serial.println(rms);

    float micFactor;

    if (rms < 800)
    {
        micFactor = 0.2;
    }
    else if (rms < 2000)
    {
        micFactor = 0.6;
    }
    else
    {
        micFactor = 1.0;
    }

    float vibrationScore = 1.0;
    float score =
        0.5 * vibrationScore +
        0.3 * 1.0 +
        0.2 * micFactor;

    Serial.print("SCORE: ");
    Serial.println(score);

    if (score > 0.75)
    {
        processSpike(rms, dynamicAccel, score);
    }
}
void processSpike(float rms,
                  float dynAccel,
                  float score)
{
    unsigned long now = millis();

    if (spikeCount == 0)
    {
        firstSpikeTime = now;
    }

    spikeCount++;

    Serial.print("Spike Count: ");
    Serial.println(spikeCount);
    if (spikeCount == 1)
    {
        String msg =
            "NODE=" + NODE_ID +
            ",CONF=LOW" +
            ",RMS=" + String((int)rms) +
            ",ACC=" + String(dynAccel,2);

        LoRa.beginPacket();
        LoRa.print(msg);
        LoRa.endPacket();

        Serial.println(msg);
    }
    if (spikeCount >= REQUIRED_SPIKES)
    {
        if ((now - firstSpikeTime) <= WINDOW_MS)
        {
            Serial.println("HIGH CONFIDENCE EVENT");

            String msg =
                "NODE=" + NODE_ID +
                ",CONF=HIGH" +
                ",RMS=" + String((int)rms) +
                ",ACC=" + String(dynAccel,2) +
                ",SCORE=" + String(score,2);

            LoRa.beginPacket();
            LoRa.print(msg);
            LoRa.endPacket();

            Serial.println(msg);
        }

        spikeCount = 0;
    }

    if ((now - firstSpikeTime) > WINDOW_MS)
    {
        spikeCount = 0;
    }
}

void mpuWrite(uint8_t reg, uint8_t data)
{
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

void mpuRead(uint8_t reg, uint8_t len, uint8_t *buf)
{
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);

    Wire.requestFrom(MPU_ADDR, len);

    for (int i = 0; i < len; i++)
    {
        buf[i] = Wire.read();
    }
}

int32_t rawSamples[RMS_SAMPLES];

void micInit()
{
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = RMS_SAMPLES,
        .use_apll = false
    };

    i2s_pin_config_t pins = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };

    i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);

    i2s_set_pin(I2S_NUM_0, &pins);

    i2s_zero_dma_buffer(I2S_NUM_0);
}

float getRMS()
{
    size_t bytesRead = 0;

    i2s_read(
        I2S_NUM_0,
        rawSamples,
        sizeof(rawSamples),
        &bytesRead,
        pdMS_TO_TICKS(100)
    );

    int n = bytesRead / sizeof(int32_t);

    if (n == 0)
        return 0;

    double sum = 0;

    for (int i = 0; i < n; i++)
    {
        double s = rawSamples[i] >> 12;
        sum += s * s;
    }

    return sqrt(sum / n);
}

void IRAM_ATTR vibrationISR()
{
    vibrationFlag = true;
}

bool mpuCheck(float &dynOut)
{
    uint8_t d[6];

    mpuRead(ACCEL_XOUT_H, 6, d);

    int16_t ax = (d[0] << 8) | d[1];
    int16_t ay = (d[2] << 8) | d[3];
    int16_t az = (d[4] << 8) | d[5];

    float x = ax / 16384.0;
    float y = ay / 16384.0;
    float z = az / 16384.0;

    float mag = sqrt(x*x + y*y + z*z);

    float dyn = abs(mag - 1.0);

    dynOut = dyn;

    Serial.print("ACC dyn: ");
    Serial.println(dyn);

    return dyn > ACC_THRESHOLD;
}