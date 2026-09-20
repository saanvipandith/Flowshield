#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <TinyGPSPlus.h>

// =====================================================
// GPS
// =====================================================

#define GPS_RX 16
#define GPS_TX 17

HardwareSerial GPSserial(2);
TinyGPSPlus gps;


// =====================================================
// FLOWSHIELD - NODE 1
// Velxio Hardware Test
// Updated Buzzer Handling
// =====================================================



// =====================================================
// MQTT / INTERNET TELEMETRY
// =====================================================
// Velxio provides internet access through Velxio-GUEST.
// MQTT is used so the FlowShield dashboard can receive
// live readings and send map-selected coordinates back.
// =====================================================
#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID = "Velxio-GUEST";
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* MQTT_TELEMETRY_TOPIC = "flowshield/node1/telemetry";
const char* MQTT_LOCATION_TOPIC  = "flowshield/node1/command/location";
const char* MQTT_STATUS_TOPIC    = "flowshield/node1/status";

WiFiClient flowShieldNet;
PubSubClient flowShieldMQTT(flowShieldNet);

double mapLatitude = 0.0;
double mapLongitude = 0.0;
bool mapLocationValid = false;

unsigned long lastMQTTReconnect = 0;
unsigned long lastTelemetry = 0;

void connectFlowShieldWiFi()
{
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID);

  Serial.print("Connecting to Velxio WiFi");

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
  {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("WiFi connection pending.");
  }
}

void flowShieldMQTTCallback(char* topic, byte* payload, unsigned int length)
{
  if (strcmp(topic, MQTT_LOCATION_TOPIC) != 0)
    return;

  // Dashboard sends: latitude,longitude
  char message[96];
  Serial.print("MQTT LOCATION RECEIVED RAW: ");
  Serial.write(payload, length);
  Serial.println();
  unsigned int n = min(length, (unsigned int)(sizeof(message) - 1));

  memcpy(message, payload, n);
  message[n] = '\0';

  double lat = 0.0;
  double lng = 0.0;

  if (sscanf(message, "%lf,%lf", &lat, &lng) == 2)
  {
    if (lat >= -90.0 && lat <= 90.0 &&
        lng >= -180.0 && lng <= 180.0)
    {
      mapLatitude = lat;
      mapLongitude = lng;
      mapLocationValid = true;

      Serial.print("MAP LOCATION FROM DASHBOARD: ");
      Serial.print(mapLatitude, 6);
      Serial.print(", ");
      Serial.println(mapLongitude, 6);
    }
  }
}

void ensureFlowShieldMQTT()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  if (flowShieldMQTT.connected())
    return;

  if (millis() - lastMQTTReconnect < 3000)
    return;

  lastMQTTReconnect = millis();

  char clientId[64];
  snprintf(
    clientId,
    sizeof(clientId),
    "FlowShieldNode1-%08lx",
    (unsigned long)millis()
  );

  Serial.print("Connecting MQTT... ");

  if (flowShieldMQTT.connect(clientId))
  {
    Serial.println("CONNECTED");

    if (flowShieldMQTT.subscribe(MQTT_LOCATION_TOPIC))
      Serial.println("Location topic subscribed");
    else
      Serial.println("Location topic SUBSCRIBE FAILED");
    flowShieldMQTT.publish(
      MQTT_STATUS_TOPIC,
      "{\"node\":1,\"status\":\"online\"}"
    );
  }
  else
  {
    Serial.print("FAILED, rc=");
    Serial.println(flowShieldMQTT.state());
  }
}

void publishFlowShieldTelemetry(
  float upstream,
  float downstream,
  int inflow,
  int drainage,
  int gas,
  float temperature,
  float humidity
)
{
  if (!flowShieldMQTT.connected())
    return;

  // The current hardware does not contain a dedicated rain sensor.
  // This is a configurable demo calibration from the inflow ADC.
  const float MAX_RAINFALL_MM_H = 250.0;

  float rainfall = ((float)inflow / 4095.0) * MAX_RAINFALL_MM_H;

  // DHT22 can briefly return NaN during startup; keep the MQTT payload valid JSON.
  if (isnan(temperature)) temperature = 0.0;
  if (isnan(humidity)) humidity = 0.0;

  // Approximate water height from the upstream ultrasonic distance.
  // Change this if the real sensor mounting height is different.
  const float SENSOR_HEIGHT_CM = 30.0;

  float waterLevel = 0.0;

  if (upstream > 0)
  {
    waterLevel = SENSOR_HEIGHT_CM - upstream;

    if (waterLevel < 0) waterLevel = 0;
    if (waterLevel > SENSOR_HEIGHT_CM) waterLevel = SENSOR_HEIGHT_CM;
  }

  float waterPercent =
    (waterLevel / SENSOR_HEIGHT_CM) * 100.0;

  // GPS altitude is exposed as an elevation/terrain proxy.
  bool gpsValid = gps.location.isValid();
  bool altitudeValid = gps.altitude.isValid();

  double telemetryLat = 0.0;
  double telemetryLng = 0.0;

  if (mapLocationValid)
  {
    telemetryLat = mapLatitude;
    telemetryLng = mapLongitude;
  }
  else if (gpsValid)
  {
    telemetryLat = gps.location.lat();
    telemetryLng = gps.location.lng();
  }

  double gpsAltitude = altitudeValid
    ? gps.altitude.meters()
    : -1.0;

  char payload[700];

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"node\":1,"
      "\"latitude\":%.6f,"
      "\"longitude\":%.6f,"
      "\"map_location\":%s,"
      "\"gps_valid\":%s,"
      "\"upstream_cm\":%.2f,"
      "\"downstream_cm\":%.2f,"
      "\"inflow_adc\":%d,"
      "\"drainage_adc\":%d,"
      "\"gas_adc\":%d,"
      "\"temperature_c\":%.2f,"
      "\"humidity_pct\":%.2f,"
      "\"rainfall_mm_h\":%.2f,"
      "\"water_level_cm\":%.2f,"
      "\"water_level_pct\":%.2f,"
      "\"gps_altitude_m\":%.2f,"
      "\"altitude_valid\":%s,"
      "\"timestamp_ms\":%lu"
    "}",
    telemetryLat,
    telemetryLng,
    mapLocationValid ? "true" : "false",
    gpsValid ? "true" : "false",
    upstream,
    downstream,
    inflow,
    drainage,
    gas,
    temperature,
    humidity,
    rainfall,
    waterLevel,
    waterPercent,
    gpsAltitude,
    altitudeValid ? "true" : "false",
    millis()
  );

  flowShieldMQTT.publish(
    MQTT_TELEMETRY_TOPIC,
    payload
  );
}


// =====================================================
// OLED
// =====================================================

#define OLED_SDA 21
#define OLED_SCL 22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);


// =====================================================
// DHT22
// =====================================================

#define DHT_PIN 4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);


// =====================================================
// ULTRASONIC
// =====================================================

#define UP_TRIG 5
#define UP_ECHO 18

#define DOWN_TRIG 19
#define DOWN_ECHO 32


// =====================================================
// ANALOG SENSORS
// =====================================================

#define INFLOW_PIN 34
#define DRAINAGE_PIN 35
#define GAS_PIN 36



// =====================================================
// RGB LED
// =====================================================

#define RED_PIN 25
#define GREEN_PIN 26
#define BLUE_PIN 27


// =====================================================
// BUZZER
// =====================================================

#define BUZZER_PIN 23

// Keeps track of whether the buzzer is currently active
bool buzzerActive = false;


// =====================================================
// SERVO
// =====================================================

#define SERVO_PIN 13

Servo drainageServo;


// =====================================================
// BUTTONS
// =====================================================

#define MAINTENANCE_BUTTON 14
#define RESET_BUTTON 33


// =====================================================
// RGB FUNCTIONS
// =====================================================

void rgbOff()
{
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
}


void rgbRed()
{
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
}


void rgbGreen()
{
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, LOW);
}


void rgbBlue()
{
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, HIGH);
}


void rgbYellow()
{
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
  digitalWrite(BLUE_PIN, LOW);
}


// =====================================================
// BUZZER FUNCTIONS
// =====================================================

// Turn buzzer ON
void buzzerOn(int frequency)
{
  if (!buzzerActive)
  {
    tone(BUZZER_PIN, frequency);
    buzzerActive = true;
  }
}


// Turn buzzer OFF
void buzzerOff()
{
  // Only call noTone() if the buzzer is actually running.
  // This prevents the Velxio warning:
  // "noTone(): Tone is not running on given pin"
  
  if (buzzerActive)
  {
    noTone(BUZZER_PIN);
    buzzerActive = false;
  }
}


// =====================================================
// ULTRASONIC FUNCTION
// =====================================================

float readDistanceCM(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duration = pulseIn(
    echoPin,
    HIGH,
    30000
  );

  // No echo received
  if (duration == 0)
  {
    return -1;
  }

  // Convert microseconds to centimeters
  return duration * 0.0343 / 2.0;
}


// =====================================================
// OLED DISPLAY
// =====================================================

void showOLED(
  float upstream,
  float downstream,
  int inflow,
  int drainage,
  int gas
)
{
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Title
  display.setCursor(0, 0);
  display.println("FLOWSHIELD");

  // Upstream
  display.setCursor(0, 12);
  display.print("UP: ");

  if (upstream < 0)
  {
    display.println("ERROR");
  }
  else
  {
    display.print(upstream);
    display.println(" cm");
  }

  // Downstream
  display.setCursor(0, 22);
  display.print("DOWN: ");

  if (downstream < 0)
  {
    display.println("ERROR");
  }
  else
  {
    display.print(downstream);
    display.println(" cm");
  }

  // Inflow
  display.setCursor(0, 32);
  display.print("INFLOW: ");
  display.println(inflow);

  // Drainage
  display.setCursor(0, 42);
  display.print("DRAIN: ");
  display.println(drainage);

  // Gas
  display.setCursor(0, 52);
  display.print("GAS: ");
  display.println(gas);

  display.display();
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  // ---------------------------------------------------
  // SERIAL
  // ---------------------------------------------------

  Serial.begin(115200);


  // ---------------------------------------------------
  // OLED
  // ---------------------------------------------------

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
      ))
  {
    Serial.println("OLED FAILED");
  }
  else
  {
    Serial.println("OLED OK");
  }


  // Initial OLED message

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);

  display.println("FLOWSHIELD");
  display.println();
  display.println("NODE 1");
  display.println("INITIALIZING...");

  display.display();


  // ---------------------------------------------------
  // DHT22
  // ---------------------------------------------------

  dht.begin();


  // ---------------------------------------------------
  // ULTRASONIC
  // ---------------------------------------------------

  pinMode(
    UP_TRIG,
    OUTPUT
  );

  pinMode(
    UP_ECHO,
    INPUT
  );

  pinMode(
    DOWN_TRIG,
    OUTPUT
  );

  pinMode(
    DOWN_ECHO,
    INPUT
  );


  // ---------------------------------------------------
  // ANALOG SENSORS
  // ---------------------------------------------------

  pinMode(
    INFLOW_PIN,
    INPUT
  );

  pinMode(
    DRAINAGE_PIN,
    INPUT
  );

  pinMode(
    GAS_PIN,
    INPUT
  );


  // ---------------------------------------------------
  // RGB LED
  // ---------------------------------------------------

  pinMode(
    RED_PIN,
    OUTPUT
  );

  pinMode(
    GREEN_PIN,
    OUTPUT
  );

  pinMode(
    BLUE_PIN,
    OUTPUT
  );

  rgbOff();


  // ---------------------------------------------------
  // BUZZER
  // ---------------------------------------------------

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  digitalWrite(
    BUZZER_PIN,
    LOW
  );

  buzzerActive = false;


  // ---------------------------------------------------
  // BUTTONS
  // ---------------------------------------------------

  pinMode(
    MAINTENANCE_BUTTON,
    INPUT_PULLUP
  );

  pinMode(
    RESET_BUTTON,
    INPUT_PULLUP
  );


  // ---------------------------------------------------
  // SERVO
  // ---------------------------------------------------

  drainageServo.attach(
    SERVO_PIN
  );

  drainageServo.write(30);


  // ---------------------------------------------------
  // GPS
  // ---------------------------------------------------

  GPSserial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX,
    GPS_TX
  );


  // ---------------------------------------------------
  // MQTT / INTERNET
  // ---------------------------------------------------

  flowShieldMQTT.setServer(
    MQTT_BROKER,
    MQTT_PORT
  );

  // Telemetry JSON is larger than PubSubClient's default 256-byte buffer.
  flowShieldMQTT.setBufferSize(1024);

  flowShieldMQTT.setCallback(
    flowShieldMQTTCallback
  );

  connectFlowShieldWiFi();
  ensureFlowShieldMQTT();


  // ---------------------------------------------------
  // STARTUP SERIAL MESSAGE
  // ---------------------------------------------------

  Serial.println();
  Serial.println("==============================");
  Serial.println(" FLOWSHIELD NODE 1");
  Serial.println(" Hardware Test");
  Serial.println("==============================");


  // ===================================================
  // RGB TEST
  // ===================================================

  Serial.println("Testing RED LED...");

  rgbRed();
  delay(500);


  Serial.println("Testing GREEN LED...");

  rgbGreen();
  delay(500);


  Serial.println("Testing BLUE LED...");

  rgbBlue();
  delay(500);


  rgbOff();


  // ===================================================
  // BUZZER TEST
  // ===================================================

  Serial.println("Testing buzzer...");

  buzzerOn(1000);

  delay(300);

  buzzerOff();


  // ===================================================
  // SERVO TEST
  // ===================================================

  Serial.println("Testing servo...");

  drainageServo.write(30);
  delay(500);

  drainageServo.write(90);
  delay(500);

  drainageServo.write(150);
  delay(500);

  drainageServo.write(90);

  Serial.println("Hardware test complete.");
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  // ===================================================
  // MQTT / INTERNET
  // ===================================================

  connectFlowShieldWiFi();
  ensureFlowShieldMQTT();
  flowShieldMQTT.loop();


  // ===================================================
  // WATER LEVELS
  // ===================================================

  float upstream =
    readDistanceCM(
      UP_TRIG,
      UP_ECHO
    );


  float downstream =
    readDistanceCM(
      DOWN_TRIG,
      DOWN_ECHO
    );


  // ===================================================
  // POTENTIOMETERS
  // ===================================================

  int inflow =
    analogRead(
      INFLOW_PIN
    );


  int drainage =
    analogRead(
      DRAINAGE_PIN
    );


  // ===================================================
  // GAS SENSOR
  // ===================================================

  int gas =
    analogRead(
      GAS_PIN
    );


  // ===================================================
  // DHT22
  // ===================================================

  float temperature =
    dht.readTemperature();


  float humidity =
    dht.readHumidity();


  // ===================================================
  // GPS
  // ===================================================

  while (
    GPSserial.available()
  )
  {
    gps.encode(
      GPSserial.read()
    );
  }


  // ===================================================
  // MQTT TELEMETRY
  // ===================================================

  if (
    millis() - lastTelemetry >= 1000
  )
  {
    lastTelemetry = millis();

    publishFlowShieldTelemetry(
      upstream,
      downstream,
      inflow,
      drainage,
      gas,
      temperature,
      humidity
    );
  }


  // ===================================================
  // SERIAL OUTPUT
  // ===================================================

  Serial.println();

  Serial.println(
    "------------------------------"
  );


  // Upstream

  Serial.print(
    "Upstream: "
  );

  if (upstream < 0)
  {
    Serial.println(
      "ERROR / NO ECHO"
    );
  }
  else
  {
    Serial.print(
      upstream
    );

    Serial.println(
      " cm"
    );
  }


  // Downstream

  Serial.print(
    "Downstream: "
  );

  if (downstream < 0)
  {
    Serial.println(
      "ERROR / NO ECHO"
    );
  }
  else
  {
    Serial.print(
      downstream
    );

    Serial.println(
      " cm"
    );
  }


  // Inflow

  Serial.print(
    "Inflow ADC: "
  );

  Serial.println(
    inflow
  );


  // Drainage

  Serial.print(
    "Drainage ADC: "
  );

  Serial.println(
    drainage
  );


  // Gas

  Serial.print(
    "Gas ADC: "
  );

  Serial.println(
    gas
  );


  // Temperature

  Serial.print(
    "Temperature: "
  );

  if (isnan(temperature))
  {
    Serial.println(
      "ERROR"
    );
  }
  else
  {
    Serial.print(
      temperature
    );

    Serial.println(
      " C"
    );
  }


  // Humidity

  Serial.print(
    "Humidity: "
  );

  if (isnan(humidity))
  {
    Serial.println(
      "ERROR"
    );
  }
  else
  {
    Serial.print(
      humidity
    );

    Serial.println(
      " %"
    );
  }


  // ===================================================
  // GPS OUTPUT
  // ===================================================

  if (
    gps.location.isValid()
  )
  {
    Serial.print(
      "Latitude: "
    );

    Serial.println(
      gps.location.lat(),
      6
    );


    Serial.print(
      "Longitude: "
    );

    Serial.println(
      gps.location.lng(),
      6
    );
  }
  else
  {
    Serial.println(
      "GPS: Waiting for fix..."
    );
  }


  // ===================================================
  // OLED
  // ===================================================

  showOLED(
    upstream,
    downstream,
    inflow,
    drainage,
    gas
  );


  // ===================================================
  // MAINTENANCE BUTTON
  // ===================================================

  if (
    digitalRead(
      MAINTENANCE_BUTTON
    ) == LOW
  )
  {
    Serial.println(
      "MAINTENANCE MODE"
    );


    // Yellow = maintenance
    rgbYellow();


    // Open inspection/flush gate
    drainageServo.write(150);


    // Short warning tone
    buzzerOn(1500);

    delay(500);

    buzzerOff();
  }


  // ===================================================
  // RESET BUTTON
  // ===================================================

  if (
    digitalRead(
      RESET_BUTTON
    ) == LOW
  )
  {
    Serial.println(
      "SYSTEM RESET"
    );


    // Return servo to normal position
    drainageServo.write(30);


    // Green = normal
    rgbGreen();


    // Stop buzzer safely
    buzzerOff();


    delay(500);
  }


  // ===================================================
  // AUTOMATIC STATUS
  // ===================================================

  /*
     HC-SR04 measures DISTANCE to the water.

     Smaller distance = water is closer to sensor
                     = water level is higher.

     For this initial hardware test:
     
     < 10 cm = high water condition
  */


  if (
    upstream > 0 &&
    upstream < 10
  )
  {
    // -----------------------------------------------
    // HIGH WATER CONDITION
    // -----------------------------------------------

    Serial.println(
      "STATUS: HIGH WATER"
    );


    // RED
    rgbRed();


    // Move drainage gate
    drainageServo.write(150);


    // Alarm
    buzzerOn(1800);
  }
  else
  {
    // -----------------------------------------------
    // NORMAL CONDITION
    // -----------------------------------------------

    Serial.println(
      "STATUS: NORMAL"
    );


    // GREEN
    rgbGreen();


    // Return drainage gate
    drainageServo.write(30);


    // Stop alarm safely
    buzzerOff();
  }


  // ===================================================
  // LOOP DELAY
  // ===================================================

  // Live proof that Velxio potentiometers are reaching analogRead().
  float rainfallNow = ((float)inflow / 4095.0f) * 250.0f;
  Serial.print("LIVE ADC -> INFLOW(GPIO34)=");
  Serial.print(inflow);
  Serial.print("  DRAINAGE(GPIO35)=");
  Serial.print(drainage);
  Serial.print("  RAINFALL=");
  Serial.print(rainfallNow, 1);
  Serial.println(" mm/h");

  delay(1000);
}