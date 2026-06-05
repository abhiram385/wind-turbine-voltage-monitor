# Wind Turbine Voltage Monitor

Real-time DC voltage monitoring for small wind turbines using ESP32 and ThingSpeak cloud dashboard. Measures 0–72V with ~17.6mV resolution via a resistive voltage divider, transmitting readings every 5 seconds over Wi-Fi.

---

## Overview

Small wind turbines produce variable DC output after rectification. This system continuously monitors that output voltage and streams it to a ThingSpeak cloud dashboard for 24/7 remote visibility — no local display required, accessible from any device with internet.

Built as a low-cost alternative to commercial monitoring systems using an ESP32 and two resistors as the core measurement circuit.



Output- https://abhiram385.github.io/wind-turbine-voltage-monitor/docs/wind_turbine_voltage_plot.html

---

## Hardware

| Component | Details |
|-----------|---------|
| Microcontroller | ESP32 DevKit V1 (CP2102, 30-pin) |
| R1 | 200kΩ (high side — series with turbine output) |
| R2 | 10kΩ (low side — shunt to ground) |
| ADC pin | GPIO34 (input only, ADC1_CH6) |
| ADC reference | 3.3V |
| Power | USB 5V via CP2102 |

---

## Circuit — Voltage Divider

```
Turbine DC output (+)
        │
       [R1 = 200kΩ]
        │
        ├──────────── GPIO34 (ESP32 ADC input)
        │
       [R2 = 10kΩ]
        │
       GND
```

**Scaling factor:** (R1 + R2) / R2 = 210k / 10k = **21.0×**

| Parameter | Value |
|-----------|-------|
| Max input voltage | ~69.3V (3.3V × 21.0) |
| Turbine operating range | 0–72V |
| ADC resolution | 12-bit (4096 steps) |
| Voltage resolution | 72V ÷ 4096 = **~17.6 mV/step** |
| ADC input at 72V | 72 ÷ 21.0 = **3.43V** (within 3.3V with marginal headroom) |

---

## System Architecture

```
Wind Turbine (DC output)
    │
    │  0–72V DC
    ▼
Voltage Divider (R1=200kΩ, R2=10kΩ)
    │
    │  0–3.3V scaled
    ▼
ESP32 GPIO34 (12-bit ADC)
    ├─ Average 64 ADC readings (noise reduction)
    ├─ Scale ADC voltage × 21.0 → turbine voltage
    └─ HTTP GET to ThingSpeak every 5s
         │
         │  Wi-Fi · HTTP · ThingSpeak API
         ▼
ThingSpeak Cloud Dashboard
    └─ Field 1: Voltage (V) over time
```

---

## Signal Processing

### Noise Reduction
64 ADC readings are averaged per measurement to reduce quantisation and thermal noise:
```c
long sum = 0;
for (int i = 0; i < 64; i++) {
    sum += analogRead(ADC_PIN);
    delayMicroseconds(100);
}
float adc_avg = sum / 64.0f;
```

### Voltage Scaling
```c
float adc_voltage     = (adc_avg / 4096.0f) * 3.3f;
float turbine_voltage = adc_voltage * 21.0f;  // Divider ratio
```

---

## ThingSpeak Setup

1. Create a free account at [thingspeak.com](https://thingspeak.com)
2. Create a new channel with Field 1 labelled "Voltage (V)"
3. Copy your Write API Key into the code:
   ```c
   const char* api_key = "YOUR_THINGSPEAK_API_KEY";
   ```
4. Data updates every 5 seconds (ThingSpeak free tier minimum is 15s — adjust `UPLOAD_INTERVAL_MS` if needed)

---

## Setup

1. Clone this repository
2. Open `wind_turbine_monitor.ino` in Arduino IDE
3. Set credentials:
   ```c
   const char* ssid    = "YOUR_WIFI";
   const char* pass    = "YOUR_PASS";
   const char* api_key = "YOUR_THINGSPEAK_API_KEY";
   ```
4. Flash to ESP32 (board: ESP32 Dev Module)
5. Open Serial Monitor at 115200 baud — voltage readings printed every 5s

---

## Cost Comparison

| Item | Cost |
|------|------|
| ESP32 DevKit | ~₹350 |
| R1 200kΩ resistor | ~₹2 |
| R2 10kΩ resistor | ~₹2 |
| **Total BOM** | **~₹354** |
| Commercial equivalent | ₹6000+ |

---

## Applications

- Small wind turbine health monitoring
- Remote off-grid power system monitoring
- Any high-voltage DC source requiring cloud-connected voltage logging

---

## Author

**Abhiram Kurella**  
B.Tech Electronics and Instrumentation Engineering  
VNR Vignana Jyothi Institute of Engineering & Technology (2023–2027)  
abhiram.kurella@gmail.com
