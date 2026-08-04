#include <HardwareSerial.h>
#include <DHT.h>
#include <SD.h>  // Include the SD card library
#include <SPI.h>  // Add SPI library

// Define the serial port for communication
HardwareSerial mySerial(1); // Use HardwareSerial1 (you can use 2 or 3 if needed)

// Define pin numbers for PMS7003 and DHT11
#define PMS7003_RX_PIN 16  // RX Pin of ESP32 (connected to PMS7003 TX)
#define PMS7003_TX_PIN 17  // TX Pin of ESP32 (connected to PMS7003 RX)
#define DHT11_PIN 21       // ESP32 pin GPIO21 connected to DHT11 sensor

// Initialize DHT11
DHT dht11(DHT11_PIN, DHT11);

// SD card connections
#define SD_CS_PIN 5  // Chip Select (CS) pin for SD card
#define SD_CLK_PIN 18  // SPI Clock (SCK)
#define SD_MISO_PIN 19  // SPI MISO
#define SD_MOSI_PIN 23  // SPI MOSI
#define SD_VCC_PIN 3.3  // VCC (3.3V pin of ESP32)
#define SD_GND_PIN 0    // GND pin of ESP32

// LED connections
#define GREEN 12 // Green LED GPIO12
#define RED 13 // Red LED GPIO13

uint8_t initFlag = 0;
uint8_t writeFlag = 0;

uint8_t prefix = 6;
String base_file_name = "_sensor_data_";
uint8_t suffix = 0;
String new_file_name = "";

float_t hum_bias = -1.0;
float_t tempF_bias = -6.3;
float_t tempC_bias = tempF_bias * (5.0/9.0);


void setup() {

  new_file_name = String("/") + String(prefix) + base_file_name + String(suffix) + ".txt";

  // Initialize LED pins
  pinMode(GREEN, OUTPUT); // Update Status LEDs
  pinMode(RED, OUTPUT);

  // Initialize Serial Monitor
  Serial.begin(115200);

  // Small delay before initializing
  delay(1000);

  // Initialize PMS7003 serial port
  mySerial.begin(9600, SERIAL_8N1, PMS7003_RX_PIN, PMS7003_TX_PIN);
  Serial.println("PMS7003 Sensor Initialized");

  // Small delay before initializing
  delay(1000);

  // Initialize the DHT11 sensor
  dht11.begin();
  Serial.println("DHT11 Sensor Initialized");

  // Small delay before initializing 
  delay(1000);

  // Initialize the SD card
  SPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);  // SCK, MISO, MOSI, CS
  Serial.println("SD Card Initializing");

  // Small delay before initializing 
  delay(1000);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    digitalWrite(GREEN, LOW); // Update Status LEDs
    digitalWrite(RED, HIGH);
  }
  else {
    Serial.println("SD card initialized.");
    digitalWrite(GREEN, HIGH); // Update Status LEDs
    digitalWrite(RED, LOW);
  }

  // Check if the SD Card reader is empty
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    digitalWrite(GREEN, LOW); // Update Status LEDs
    digitalWrite(RED, HIGH);
  }

  // Check for unused file name
  while (initFlag == 0) {
    if (SD.exists(new_file_name)) {
      suffix++;
      new_file_name = String("/") + String(prefix) + base_file_name + String(suffix) + ".txt";
    } else {
      initFlag = 1;
    }
  }
  Serial.println(new_file_name);

  // Create the file on the SD card for writing
  File dataFile = SD.open(new_file_name, FILE_WRITE);
  if (dataFile) {
    // Write header to the file (for reference)
    Serial.println("Timestamp(s),PM1.0,PM2.5,PM10,PM1.0atm,PM2.5atm,PM10atm,PNum0.3,PNum0.5,PNum1.0,PNum2.5,PNum5.0,PNum10.0,Humidity,Temperature_C,Temperature_F");
    dataFile.println("Timestamp(s),PM1.0,PM2.5,PM10,PM1.0atm,PM2.5atm,PM10atm,PNum0.3,PNum0.5,PNum1.0,PNum2.5,PNum5.0,PNum10.0,Humidity,Temperature_C,Temperature_F");
    dataFile.close(); // Close the file after writing the header
    digitalWrite(GREEN, HIGH); // Update Status LEDs
    digitalWrite(RED, LOW);
  } else {
    Serial.println("Failed to open file for writing.");
    digitalWrite(GREEN, LOW); // Update Status LEDs
    digitalWrite(RED, HIGH);
  }

}

void loop() {
    
  // Initialize PMS variables
  uint16_t frame_len = 0;
  uint16_t pm1_0 = 0;  // PM1.0 concentration
  uint16_t pm2_5 = 0;  // PM2.5 concentration
  uint16_t pm10 = 0;   // PM10 concentration
  uint16_t pm1_0atm = 0; // PM1.0 concentration under atmospheric env.
  uint16_t pm2_5atm = 0; // PM2.5 concentration under atmospheric env.
  uint16_t pm10atm = 0; // PM10 concentration under atmospheric env.
  uint16_t num0_3 = 0; // Num particles beyond 0.3um in 0.1 L of air
  uint16_t num0_5 = 0; // Num particles beyond 0.5um in 0.1 L of air
  uint16_t num1_0 = 0; // Num particles beyond 1.0um in 0.1 L of air
  uint16_t num2_5 = 0; // Num particles beyond 2.5um in 0.1 L of air
  uint16_t num5_0 = 0; // Num particles beyond 5.0um in 0.1 L of air
  uint16_t num10 = 0; // Num particles beyond 10.0um in 0.1 L of air
  uint16_t reserved = 0; // Reserved
  uint16_t check = 0;

  // Initialize humidity and temperature variables for DHT11 sensor
  float humi = 0;
  float tempC = 0;
  float tempF = 0;

  // Read PMS7003 data if available
  if (mySerial.available() >= 32) {  // We expect a full data frame of 32 bytes
    uint8_t data[32];  // Array to store incoming data frame
    mySerial.readBytes(data, 32);  // Read the 32 bytes of data

    // Check if the data starts with the correct start bytes (0x42, 0x4D)
    if (data[0] == 0x42 && data[1] == 0x4D) {
      frame_len = (data[2] << 8) | data[3];
      pm1_0 = (data[4] << 8) | data[5];  // PM1.0 concentration
      pm2_5 = (data[6] << 8) | data[7];  // PM2.5 concentration
      pm10 = (data[8] << 8) | data[9];   // PM10 concentration
      pm1_0atm = (data[10] << 8) | data[11]; // PM1.0 concentration under atmospheric env.
      pm2_5atm = (data[12] << 8) | data[13]; // PM2.5 concentration under atmospheric env.
      pm10atm = (data[14] << 8) | data[15]; // PM10 concentration under atmospheric env.
      num0_3 = (data[16] << 8) | data[17]; // Num particles beyond 0.3um in 0.1 L of air
      num0_5 = (data[18] << 8) | data[19]; // Num particles beyond 0.5um in 0.1 L of air
      num1_0 = (data[20] << 8) | data[21]; // Num particles beyond 1.0um in 0.1 L of air
      num2_5 = (data[22] << 8) | data[23]; // Num particles beyond 2.5um in 0.1 L of air
      num5_0 = (data[24] << 8) | data[25]; // Num particles beyond 5.0um in 0.1 L of air
      num10 = (data[26] << 8) | data[27]; // Num particles beyond 10.0um in 0.1 L of air
      reserved = (data[28] << 8) | data[29]; // Reserved
      check = (data[30] << 8) | data[31]; // Check code 

      // Total Check Sum value for comparison
      uint16_t checksum = 0;
      for (int i = 0; i<30; i++) {
        checksum += data[i];
      }

      // Print Checksum Test - Uncomment for Debugging Purposes
      // Serial.print("Check: ");
      // Serial.print(check);
      // Serial.print("  vs: ");
      // Serial.print(checksum);
      // Serial.print("  |   ");

      if (check == checksum) {
        // Print Data to Serial Monitor - Uncomment for Debugging Purposes 
        // Serial.print("PM1.0: ");
        // Serial.print(pm1_0);
        // Serial.print(" µg/m³, PM2.5: ");
        // Serial.print(pm2_5);
        // Serial.print(" µg/m³, PM10: ");
        // Serial.print(pm10);
        // Serial.print(" µg/m³  PM1.0atm: ");
        // Serial.print(pm1_0atm);
        // Serial.print(" um/0.1L, PM2.5atm: ");
        // Serial.print(pm2_5atm);
        // Serial.print(" um/0.1L, PM10atm: ");
        // Serial.print(pm10atm);
        // Serial.print(" um/0.1L, PNum0.3: ");
        // Serial.print(num0_3);
        // Serial.print(" um/0.1L, PNum0.5: ");
        // Serial.print(num0_5);      
        // Serial.print(" um/0.1L, PNum1.0: ");
        // Serial.print(num1_0);
        // Serial.print(" um/0.1L, PNum2.5: ");
        // Serial.print(num2_5);
        // Serial.print(" um/0.1L, PNum5.0: ");
        // Serial.print(num5_0);
        // Serial.print(" um/0.1L, PNum10.0: ");
        // Serial.print(num10);
        // Serial.print(" um/0.1L  |   ");      

        writeFlag = 1;
        digitalWrite(GREEN, HIGH); // Update Status LEDs
        digitalWrite(RED, LOW);
      } else {
        Serial.print("***Incorrect Checksum***"); // Print that the data was corrupted
        writeFlag = 0;
        digitalWrite(GREEN, LOW); // Update Status LEDs
        digitalWrite(RED, HIGH);
      }

    } else {
      Serial.println("Invalid data frame received."); // Print that the data frame was incorrect
      writeFlag = 0;
      digitalWrite(GREEN, LOW); // Update Status LEDs
      digitalWrite(RED, HIGH);
    }
  } else {
    Serial.println("Serial Unavailable (PMS7003)"); // Print that the Serial Monitor was unavailable
    writeFlag = 0;
    digitalWrite(GREEN, LOW); // Update Status LEDs
    digitalWrite(RED, HIGH);
  }

  // Read humidity and temperature from DHT11 sensor
  humi = dht11.readHumidity() + hum_bias;
  tempC = dht11.readTemperature() + tempC_bias;
  tempF = dht11.readTemperature(true) + tempF_bias;

  if (isnan(tempC) || isnan(humi)) {
    Serial.print("Failed to read from DHT11 sensor!");
    if (isnan(tempC)) Serial.print(" Temperature reading failed.");
    if (isnan(humi)) Serial.print(" Humidity reading failed.");
    Serial.println();
    writeFlag = 0;
    digitalWrite(GREEN, LOW); // Update Status LEDs
    digitalWrite(RED, HIGH);
  } else {
    // Print Temperature and Humidity Data - Uncomment for Debugging Purposes
    // Serial.print("Humidity: ");
    // Serial.print(humi);
    // Serial.print("%  |  ");
    // Serial.print("Temperature: ");
    // Serial.print(tempC);
    // Serial.print("°C ~ ");
    // Serial.print(tempF);
    // Serial.println("°F");
  }

  // Write data to SD Card if all data is valid
  if (writeFlag == 1) {

    // Compile data to a string for writing
    String writeData = String(int(millis()/1000)) + "," + String(pm1_0) + "," + String(pm2_5) + "," + String(pm10) + "," + String(pm1_0atm) + "," + String(pm2_5atm) + "," + String(pm10atm) + "," + String(num0_3) + "," + String(num0_5) + "," + String(num1_0) + "," + String(num2_5) + "," + String(num5_0) + "," + String(num10) + "," + String(humi) + "," + String(tempC) + "," + String(tempF);

    File dataFile = SD.open(new_file_name, FILE_APPEND); // Open the file for appending data
    // Serial.println(dataFile); // Check that the data file is being opened properly
    if (dataFile) {
      digitalWrite(GREEN, HIGH); // Update Status LEDs
      digitalWrite(RED, LOW);
      Serial.println(writeData); // Write timestamp and sensor data to the Serial Monitor
      dataFile.println(writeData); // Write timestamp and sensor data to the file
      dataFile.flush();  // Ensure data is written to the SD card
      dataFile.close();  // Close the file after writing
      // Serial.println(dataFile); // Check that the data file is being closed properly
    } else {
      Serial.println("***Error writing data to file***"); // Print Error if file is not opened properly
      digitalWrite(GREEN, LOW); // Update Status LEDs
      digitalWrite(RED, HIGH);
    }
  }

  delay(1000);  // Wait for 1 second before reading again
}
