#include <Arduino_SensorKit.h>

constexpr unsigned long sampleIntervalMilliseconds = 20;
unsigned long lastSampleMilliseconds = 0;

void setup()
{
    Serial.begin(115200);

    Accelerometer.begin();
}

void loop()
{
    const unsigned long now = millis();

    if (now - lastSampleMilliseconds < sampleIntervalMilliseconds)
        return;

    lastSampleMilliseconds = now;

    Serial.print("ACCEL,1,");
    Serial.print(now);
    Serial.print(',');
    Serial.print(Accelerometer.readX(), 4);
    Serial.print(',');
    Serial.print(Accelerometer.readY(), 4);
    Serial.print(',');
    Serial.println(Accelerometer.readZ(), 4);
}