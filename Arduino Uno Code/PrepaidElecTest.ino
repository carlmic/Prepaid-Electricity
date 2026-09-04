/*
 * MAIN BRAIN: Arduino Uno (Dynamic Load Smart Grid)
 * Architecture: Blynk Stream over SoftwareSerial via ESP-12E Bridge
 */

#define BLYNK_TEMPLATE_ID "TMPL68w52Ri5o"
#define BLYNK_TEMPLATE_NAME "Prepaid Electricity"
#define BLYNK_AUTH_TOKEN "9Kgys92P1Gbfql9JqFUapDK8CH-QFX4c"
#define BLYNK_PRINT Serial

#include <SoftwareSerial.h>
#include <BlynkSimpleStream.h>

SoftwareSerial EspSerial(2, 3); // RX, TX for ESP-12E Bridge

// --- Hardware Pins ---
const int pinsA[4] = {4, 5, 6, 7};   // House A LEDs
const int pinsB[4] = {8, 9, 10, 11}; // House B LEDs

// --- House States ---
bool stateA[4] = {false, false, false, false}; // Tracks intended state of House A LEDs
float balanceA = 0.0; 
float drawA = 0.0;      

bool stateB[4] = {false, false, false, false}; // Tracks intended state of House B LEDs
float balanceB = 0.0; 
float drawB = 0.0;      

// Power consumption per active LED per 2-second cycle
const float COST_PER_LED = 0.2; 

BlynkTimer timer;

void setup() {
  Serial.begin(9600);
  EspSerial.begin(9600);

  // Initialize all LED pins to OFF
  for (int i = 0; i < 4; i++) {
    pinMode(pinsA[i], OUTPUT);
    digitalWrite(pinsA[i], LOW);
    
    pinMode(pinsB[i], OUTPUT);
    digitalWrite(pinsB[i], LOW);
  }

  Serial.println("--- Dynamic Load Meter Started ---");
  delay(5000); 

  Blynk.begin(EspSerial, BLYNK_AUTH_TOKEN);
  
  // Run the core metering cycle every 2 seconds
  timer.setInterval(2000L, manageSmartGrid);
}

void loop() {
  Blynk.run();
  timer.run();
}

// ==============================================
// BLYNK INPUT HANDLERS
// ==============================================

// House A: LED Switches
BLYNK_WRITE(V1) { stateA[0] = param.asInt(); }
BLYNK_WRITE(V2) { stateA[1] = param.asInt(); }
BLYNK_WRITE(V3) { stateA[2] = param.asInt(); }
BLYNK_WRITE(V4) { stateA[3] = param.asInt(); }

// House A: Top-Up
BLYNK_WRITE(V5) {
  float topUp = param.asFloat();
  if (topUp > 0) {
    balanceA += topUp;
    Blynk.virtualWrite(V6, balanceA); 
  }
}

// House B: LED Switches
BLYNK_WRITE(V9)  { stateB[0] = param.asInt(); }
BLYNK_WRITE(V10) { stateB[1] = param.asInt(); }
BLYNK_WRITE(V11) { stateB[2] = param.asInt(); }
BLYNK_WRITE(V12) { stateB[3] = param.asInt(); }

// House B: Top-Up
BLYNK_WRITE(V13) {
  float topUp = param.asFloat();
  if (topUp > 0) {
    balanceB += topUp;
    Blynk.virtualWrite(V14, balanceB); 
  }
}

// ==============================================
// CORE METERING LOGIC
// ==============================================

void manageSmartGrid() {
  // 1. CALCULATE DYNAMIC POWER DRAW
  int activeLedsA = stateA[0] + stateA[1] + stateA[2] + stateA[3];
  int activeLedsB = stateB[0] + stateB[1] + stateB[2] + stateB[3];
  
  drawA = activeLedsA * COST_PER_LED;
  drawB = activeLedsB * COST_PER_LED;

  // 2. PROCESS HOUSE A
  if (balanceA > 0) {
    balanceA -= drawA;
    if (balanceA <= 0) {
      balanceA = 0; // Prevent negative balance
      killPower('A'); // Cut power if balance depleted mid-cycle
    } else {
      // Balance is good, apply intended LED states
      for(int i=0; i<4; i++) digitalWrite(pinsA[i], stateA[i] ? HIGH : LOW);
    }
  } else if (activeLedsA > 0) {
    // Balance is 0 but user tried to turn a switch on
    killPower('A');
  }

  // 3. PROCESS HOUSE B
  if (balanceB > 0) {
    balanceB -= drawB;
    if (balanceB <= 0) {
      balanceB = 0; 
      killPower('B'); 
    } else {
      for(int i=0; i<4; i++) digitalWrite(pinsB[i], stateB[i] ? HIGH : LOW);
    }
  } else if (activeLedsB > 0) {
    killPower('B');
  }

  // 4. CALCULATE ESTIMATED TIME REMAINING
  String timeA = calculateTimeLeft(balanceA, drawA);
  String timeB = calculateTimeLeft(balanceB, drawB);

  // 5. UPDATE BLYNK DASHBOARD
  Blynk.virtualWrite(V6, balanceA); 
  Blynk.virtualWrite(V7, drawA);      
  Blynk.virtualWrite(V8, timeA);      

  Blynk.virtualWrite(V14, balanceB); 
  Blynk.virtualWrite(V15, drawB);      
  Blynk.virtualWrite(V16, timeB);      
}

// ==============================================
// HELPER FUNCTIONS
// ==============================================

// Calculates remaining time and formats as MM:SS
String calculateTimeLeft(float balance, float draw) {
  if (balance <= 0) return "POWER CUT";
  if (draw == 0) return "Standby (No Draw)";
  
  // Total cycles left * 2 seconds per cycle
  int totalSeconds = (balance / draw) * 2; 
  int mins = totalSeconds / 60;
  int secs = totalSeconds % 60;
  
  return String(mins) + "m " + String(secs) + "s";
}

// Forcefully shuts off LEDs and resets App switches
void killPower(char house) {
  if (house == 'A') {
    for(int i=0; i<4; i++) {
      digitalWrite(pinsA[i], LOW); // Turn off physical LED
      stateA[i] = false;           // Reset internal logic state
      Blynk.virtualWrite(V1 + i, 0); // Flip Blynk switch to OFF visually
    }
  } else if (house == 'B') {
    for(int i=0; i<4; i++) {
      digitalWrite(pinsB[i], LOW); 
      stateB[i] = false;           
      Blynk.virtualWrite(V9 + i, 0); 
    }
  }
}