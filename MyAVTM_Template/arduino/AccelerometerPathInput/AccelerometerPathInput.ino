#include <Arduino_SensorKit.h>

constexpr unsigned long sampleIntervalMilliseconds = 20;
constexpr unsigned long buttonDebounceMilliseconds = 30;
constexpr int buttonPin = 4;
unsigned long lastSampleMilliseconds = 0;
unsigned long lastButtonChangeMilliseconds = 0;
int rawButtonState = LOW;
int stableButtonState = LOW;

void setup()
{
    Serial.begin(115200);

    Accelerometer.begin();
    pinMode(buttonPin, INPUT);
}

void loop()
{
    const unsigned long now = millis();
    const int buttonReading = digitalRead(buttonPin);
    if (buttonReading != rawButtonState)
    {
        rawButtonState = buttonReading;
        lastButtonChangeMilliseconds = now;
    }
    if (now - lastButtonChangeMilliseconds >= buttonDebounceMilliseconds)
        stableButtonState = rawButtonState;

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
    Serial.print(Accelerometer.readZ(), 4);
    Serial.print(',');
    Serial.println(stableButtonState == HIGH ? 1 : 0);
}