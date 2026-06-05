/* =============================================================
 *  Wind Turbine Voltage Monitor
 *  Hardware : ESP32 DevKit V1 (CP2102, 30-pin)
 *  Sensor   : Resistive voltage divider (R1=200kΩ, R2=10kΩ)
 *  Cloud    : ThingSpeak (HTTP POST, 5s interval)
 *
 *  Measures DC output voltage of a small wind turbine (0–72V)
 *  via a voltage divider that scales 72V → 3.3V for the ESP32
 *  ADC. Transmits voltage readings to ThingSpeak every 5s for
 *  remote 24/7 monitoring — no local display required.
 *
 *  Voltage divider scaling:
 *  Vout = Vin * R2 / (R1 + R2)
 *       = Vin * 10k / (200k + 10k)
 *       = Vin * 0.04762
 *  Max input: 3.3V / 0.04762 = ~69.3V (safe for 72V with margin)
 *
 *  ADC resolution:
 *  72V / 4096 steps = ~17.6 mV per step (12-bit ADC)
 * ============================================================= */

#include <WiFi.h>
#include <HTTPClient.h>

/* ─── Wi-Fi credentials ─────────────────────────────────────── */
const char* ssid = "YOUR_WIFI";
const char* pass = "YOUR_PASS";

/* ─── ThingSpeak configuration ──────────────────────────────── */
const char* thingspeak_url = "http://api.thingspeak.com/update";
const char* api_key        = "YOUR_THINGSPEAK_API_KEY";

/* ─── ADC configuration ─────────────────────────────────────── */
#define ADC_PIN         34      /* GPIO34 — input only, ADC1_CH6  */
#define ADC_RESOLUTION  4096.0f /* 12-bit ADC                     */
#define ADC_VREF        3.3f    /* ESP32 ADC reference voltage     */

/* ─── Voltage divider scaling ───────────────────────────────── 
 *  R1 = 200kΩ (series, high side)
 *  R2 = 10kΩ  (shunt, low side)
 *  Scale factor = (R1 + R2) / R2 = 210k / 10k = 21.0
 *  Actual turbine voltage = ADC voltage * 21.0
 * ──────────────────────────────────────────────────────────── */
#define VOLTAGE_SCALE   21.0f

/* ─── Sampling and upload interval ─────────────────────────── */
#define UPLOAD_INTERVAL_MS  5000   /* 5 seconds */
#define ADC_SAMPLES         64     /* Average 64 readings to reduce noise */

/* ─── Global state ───────────────────────────────────────────── */
unsigned long last_upload = 0;

/* ─── Read and average ADC, convert to real voltage ─────────── */
float readVoltage() {
    /* Average multiple ADC readings to reduce quantisation noise */
    long sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        sum += analogRead(ADC_PIN);
        delayMicroseconds(100);
    }
    float adc_avg     = sum / (float)ADC_SAMPLES;
    float adc_voltage = (adc_avg / ADC_RESOLUTION) * ADC_VREF;

    /* Scale back to actual turbine voltage using divider ratio */
    float turbine_voltage = adc_voltage * VOLTAGE_SCALE;
    return turbine_voltage;
}

/* ─── Upload voltage to ThingSpeak via HTTP GET ─────────────── */
void uploadToThingSpeak(float voltage) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Not connected — skipping upload");
        return;
    }

    HTTPClient http;
    String url = String(thingspeak_url) +
                 "?api_key=" + api_key +
                 "&field1="  + String(voltage, 2);

    http.begin(url);
    int response = http.GET();

    if (response > 0) {
        Serial.printf("[ThingSpeak] Upload OK — %.2fV (HTTP %d)\n",
                      voltage, response);
    } else {
        Serial.printf("[ThingSpeak] Upload failed — error %d\n", response);
    }
    http.end();
}

/* ─── Setup ─────────────────────────────────────────────────── */
void setup() {
    Serial.begin(115200);

    /* Configure ADC */
    analogReadResolution(12);   /* 12-bit = 0–4095 */
    analogSetAttenuation(ADC_11db); /* Full 0–3.3V range */

    /* Connect to Wi-Fi */
    WiFi.begin(ssid, pass);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected. IP: " + WiFi.localIP().toString());
    Serial.println("Monitoring started. Uploading every 5s to ThingSpeak.");
}

/* ─── Main loop ─────────────────────────────────────────────── */
void loop() {
    unsigned long now = millis();

    if (now - last_upload >= UPLOAD_INTERVAL_MS) {
        float voltage = readVoltage();
        Serial.printf("[ADC] Turbine voltage: %.2f V\n", voltage);
        uploadToThingSpeak(voltage);
        last_upload = now;
    }
}
