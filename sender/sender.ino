/*
  SENDER NODEMCU (ESP8266)
  MAC Address of this device: 8C:AA:B5:52:BE:F4
  Sends detection status to Receiver MAC: A4:CF:12:FF:88:31

  Pin mapping used in this code:
  D1  Green LED   (through 220 ohm resistor)
  D2  Red LED     (through 220 ohm resistor)
  D3  Yellow LED  (through 220 ohm resistor)
  D5  Ultrasonic HC-SR04 TRIG
  D6  Ultrasonic HC-SR04 ECHO
  D7  PIR Motion Sensor OUT
  G   Common ground for LED cathodes, ultrasonic GND, PIR GND
  VU  Ultrasonic VCC (5V line)
  3V  PIR VCC

  Local LED logic (unchanged, for on-board indication only):
  Both ultrasonic and motion detect something  -> Red LED ON
  Only motion detects something                -> Yellow LED ON
  Only ultrasonic detects something             -> Green LED ON
  Nothing detected                              -> All LEDs OFF

  WIRELESS RULE - PART 1 (unchanged):
  A message is only ever considered when BOTH the ultrasonic sensor
  AND the PIR motion sensor detect something AT THE SAME TIME.
  If only one sensor detects something, or if nothing is detected,
  no message goes anywhere near the Receiver.

  WIRELESS RULE - PART 2 (new, confirmation + cooldown):
  Instead of sending the moment both sensors trigger, the Sender now
  waits and double-checks first, so a single quick, noisy, or
  accidental trigger cannot cause a message to be sent.

  Step A - CONFIRMATION WINDOW (about 1 second):
  The instant both sensors trigger together, the Sender notes the
  current ultrasonic distance as a baseline and starts a 1 second
  timer. During that 1 second, it keeps re-checking the ultrasonic
  distance. If the object stays within a small tolerance of that
  same baseline distance, and both sensors remain triggered, for the
  full 1 second, the detection is treated as CONFIRMED.
  If the object moves away, the distance changes too much, or either
  sensor stops triggering before the 1 second is complete, the
  confirmation is cancelled and the Sender simply goes back to
  watching for a fresh detection, nothing is sent.

  Step B - COOLDOWN WINDOW (3 seconds):
  Once a confirmed detection is actually sent to the Receiver, the
  Sender will not start a new confirmation cycle for the next
  3 seconds, even if both sensors keep triggering. This stops the
  same ongoing event from flooding the Receiver with many messages
  per second, and matches the requested minimum spacing of 1 to 3
  seconds between messages.

  Detection distance threshold for ultrasonic: 20 cm
  Data is sent to the receiver using ESP-NOW, no WiFi router or internet involved.
*/

#include <ESP8266WiFi.h>
#include <math.h>
extern "C" {
  #include <espnow.h>
}

#define GREEN_LED   D1
#define RED_LED     D2
#define YELLOW_LED  D3
#define TRIG_PIN    D5
#define ECHO_PIN    D6
#define PIR_PIN     D7

#define DETECTION_DISTANCE_CM 20

/* Tolerance used while confirming that the object is still at the
   same distance. A small allowance handles normal sensor jitter. */
#define DISTANCE_TOLERANCE_CM 3.0

/* How long the detection must stay stable before it is trusted */
const unsigned long CONFIRM_DURATION_MS = 1000;

/* Minimum gap enforced between two messages sent to the Receiver */
const unsigned long COOLDOWN_DURATION_MS = 3000;

uint8_t receiverMac[] = {0xA4, 0xCF, 0x12, 0xFF, 0x88, 0x31};

typedef struct struct_message {
  bool anyDetected;
  bool motionDetected;
  bool ultrasonicDetected;
  float distance;
} struct_message;

struct_message outgoingData;

unsigned long lastSendTime = 0;      /* used for the 3 second cooldown */
bool cooldownActive = false;

bool confirming = false;             /* true while inside the 1 second check */
unsigned long confirmStartTime = 0;
float confirmBaselineDistance = 0;

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Send status: ");
  Serial.println(sendStatus == 0 ? "Success" : "Failed");
}

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    return -1;
  }
  float distanceCM = duration * 0.0343 / 2.0;
  return distanceCM;
}

void setup() {
  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);

  WiFi.mode(WIFI_STA);
  Serial.print("Sender MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed, restarting");
    ESP.restart();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(receiverMac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_register_send_cb(OnDataSent);
}

void loop() {
  float distance = readDistanceCM();
  bool ultrasonicDetected = (distance > 0 && distance <= DETECTION_DISTANCE_CM);
  bool motionDetected = (digitalRead(PIR_PIN) == HIGH);
  bool bothDetected = ultrasonicDetected && motionDetected;

  /* Local Sender LEDs still react immediately, this part is unchanged */
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);

  if (bothDetected) {
    digitalWrite(RED_LED, HIGH);
  } else if (motionDetected) {
    digitalWrite(YELLOW_LED, HIGH);
  } else if (ultrasonicDetected) {
    digitalWrite(GREEN_LED, HIGH);
  }

  /* Check whether the 3 second cooldown after the last send is still active */
  if (cooldownActive && (millis() - lastSendTime >= COOLDOWN_DURATION_MS)) {
    cooldownActive = false;
  }

  if (cooldownActive) {
    /* Still inside the post-send cooldown window, ignore everything */
    Serial.println("Cooldown active, waiting before next possible detection");
  }
  else if (bothDetected) {
    if (!confirming) {
      /* A fresh detection just started, begin the 1 second confirmation */
      confirming = true;
      confirmStartTime = millis();
      confirmBaselineDistance = distance;
      Serial.print("Detection started, confirming for 1 second at ~");
      Serial.print(confirmBaselineDistance);
      Serial.println(" cm");
    } else {
      float drift = fabs(distance - confirmBaselineDistance);

      if (drift > DISTANCE_TOLERANCE_CM) {
        /* Object moved too much, this was not a stable detection */
        Serial.println("Distance changed too much, confirmation cancelled");
        confirming = false;
      } else if (millis() - confirmStartTime >= CONFIRM_DURATION_MS) {
        /* Stayed steady for the full 1 second, this is a confirmed detection */
        outgoingData.anyDetected = true;
        outgoingData.motionDetected = true;
        outgoingData.ultrasonicDetected = true;
        outgoingData.distance = distance;

        esp_now_send(receiverMac, (uint8_t *) &outgoingData, sizeof(outgoingData));

        Serial.print("CONFIRMED after 1 second | Combined Distance: ");
        Serial.print(distance);
        Serial.println(" cm | Packet sent to Receiver");

        confirming = false;
        cooldownActive = true;
        lastSendTime = millis();
      } else {
        Serial.print("Still confirming... ");
        Serial.print((millis() - confirmStartTime));
        Serial.println(" ms elapsed");
      }
    }
  }
  else {
    /* Either sensor stopped triggering, or neither ever triggered */
    if (confirming) {
      Serial.println("Detection lost before confirmation completed, cancelled");
    }
    confirming = false;

    Serial.print("Ultrasonic: ");
    Serial.print(ultrasonicDetected ? "DETECTED" : "clear");
    Serial.print(" | Distance: ");
    Serial.print(distance);
    Serial.print(" cm | Motion: ");
    Serial.print(motionDetected ? "DETECTED" : "clear");
    Serial.println(" | No packet sent (single or no detection)");
  }

  delay(100);
}
