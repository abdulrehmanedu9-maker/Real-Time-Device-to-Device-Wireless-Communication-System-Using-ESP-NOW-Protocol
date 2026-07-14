/*
  RECEIVER NODEMCU (ESP8266)
  MAC Address of this device: A4:CF:12:FF:88:31
  Receives detection status from Sender MAC: 8C:AA:B5:52:BE:F4

  Pin mapping used in this code:
  D1  Green LED  (through 220 ohm resistor)
  D2  Red LED    (through 220 ohm resistor)
  G   Common ground for LED cathodes

  UPDATED LOGIC (as instructed by supervisor):
  The Sender now only transmits a packet when BOTH the ultrasonic
  sensor AND the PIR motion sensor detect something at the same time.

  So on the Receiver side:
  A packet arrives            -> Red LED ON, print the combined
                                  detection distance to Serial Monitor
  No packet arrives / only
  one sensor was triggered on
  the Sender / nothing at all
  was detected                -> Green LED ON (safe / idle state)

  NOTEPAD LOGGING (new):
  Every line that is printed on the Serial Monitor for a received
  detection is ALSO appended, at the same time, to a small text log
  file stored on the Receiver's own flash memory using LittleFS.
  The file is called /detection_log.txt and keeps growing with one
  line per detection event, acting like a running notepad file.

  How to read the log file later:
  Option 1: Add a small extra sketch that calls LittleFS.begin(),
            opens "/detection_log.txt" in read mode, and prints its
            full content to the Serial Monitor.
  Option 2: Use the "ESP8266 LittleFS Filesystem Uploader / Downloader"
            tool in the Arduino IDE to pull the file to your computer.

  If the connection with the sender is lost (no packet within the
  timeout window) the receiver falls back to Green LED, matching the
  same behavior as never having received a combined detection.
*/

#include <ESP8266WiFi.h>
#include <LittleFS.h>
extern "C" {
  #include <espnow.h>
}

#define GREEN_LED  D1
#define RED_LED    D2

/* Slightly longer than the Sender's 3 second cooldown, so the red
   LED does not flicker back to green during the normal gap between
   two confirmed messages while the object is still there. */
const unsigned long noDataTimeout = 4000;
const char *LOG_FILE_PATH = "/detection_log.txt";

typedef struct struct_message {
  bool anyDetected;
  bool motionDetected;
  bool ultrasonicDetected;
  float distance;
} struct_message;

struct_message incomingData;
unsigned long lastReceiveTime = 0;
bool hasReceivedOnce = false;

/* Appends one line of text to the notepad log file on LittleFS */
void logToFile(const String &line) {
  File f = LittleFS.open(LOG_FILE_PATH, "a");
  if (!f) {
    Serial.println("Warning: could not open log file for writing");
    return;
  }
  f.println(line);
  f.close();
}

void OnDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  memcpy(&incomingData, data, sizeof(incomingData));
  lastReceiveTime = millis();
  hasReceivedOnce = true;

  /* Build one line of text that goes to BOTH the Serial Monitor
     and the notepad log file, so they always match exactly. */
  String line = "[" + String(millis() / 1000) + "s] BOTH sensors detected on Sender | Combined Distance: " +
                String(incomingData.distance) + " cm | Receiver LED: RED";

  Serial.println(line);
  logToFile(line);
}

void setup() {
  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);

  WiFi.mode(WIFI_STA);

  /* Start the on-chip file system used for the notepad log file */
  if (!LittleFS.begin()) {
    LittleFS.format();
    LittleFS.begin();
  }

  if (esp_now_init() != 0) {
    ESP.restart();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);

  /* Nothing else is printed here on purpose. The Serial Monitor
     stays silent until the Sender actually reports that BOTH
     sensors detected something together. */
}

void loop() {
  bool connectionLost = (millis() - lastReceiveTime > noDataTimeout);

  if (!hasReceivedOnce || connectionLost || !incomingData.anyDetected) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  }

  delay(100);
}
