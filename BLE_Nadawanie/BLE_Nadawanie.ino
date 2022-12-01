// Biblioteki wykorzystywane w programie
#include <Wire.h>
#include <time.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Rtc_Pcf8563.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Zmienne wykorzystywane w programie
#define SEALEVELPRESSURE_HPA (1013.25) // Wartość ciśnienia na uśrednionym poziomie morza (Mean Sea Level)
#define I2C_SDA 17                     // Piny, które zostaną wykorzystane do
#define I2C_SCL 18                     // stworzenia magistralii I2C
#define LED 22                         // numer pinu odpowiedzialnego za stan LED na płtyce
static BLEUUID    SERVICE_UUID("b1702e41-2dc3-474c-855b-1e4355291eb6");         // usługa zdalna, pod którą będziemy nadawać
static BLEUUID    CHARACTERISTIC_UUID("9a22faec-e998-4d85-ab0f-0d8683c99989");  // charakterystyka naszej usługi

TwoWire I2CBME = TwoWire(0);           // instancja do zmiany pinów magistralii I2C
Adafruit_BME280 bme;                   // instancja czujnika BME280
Rtc_Pcf8563 rtc;                       // instacja zewnętrznego RTC

RTC_DATA_ATTR int readingID = 0;       // zmienna numeru pomiarów zapisywana w pamięci RTC

// Funkcja tworząca gotowego do wysłania stringa ze zmierzonymi danymi.
String sendMessage() {
  String data_info = String(millis()) + ":";
  data_info += String(rtc.formatTime(RTCC_TIME_HMS)) + "|";
  data_info += String(readingID) + ";";                               // nr pomiaru
  data_info += String(rtc.formatTime(RTCC_TIME_HMS)) + ";";           // czas
  data_info += String(rtc.formatDate(RTCC_DATE_WORLD)) + ";";         // data
  data_info += String(bme.readTemperature()) + ";";                   // temperatura
  data_info += String(bme.readPressure() / 100.0F) + ";";             // ciśnienie
  data_info += String(bme.readAltitude(SEALEVELPRESSURE_HPA)) + ";";  // ~wysokość n.p.m.
  data_info += String(bme.readHumidity()) + "\r\n";                   // wilgotność
  return data_info;
}

// Program główny
void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);             // zmiana pinów I2C dla RTC
  I2CBME.begin(I2C_SDA, I2C_SCL, 100000);   // zmiana pinów I2C dla BME280
  bool status;
  pinMode (LED, OUTPUT);
  digitalWrite(LED, LOW);                   // włączenie wbudowanego LEDa
  status = bme.begin(0x76, &I2CBME);  
  Serial.println("\nUruchamianie stacji nadającej...");

  if (!status) {
    Serial.println("Nie znaleziono urządzenia BME280! Sprawdź podłączenie.");
  }

  // Stworzenie urządzenia BLE do nadawania wpisanej charakterystyki. 
  String info = sendMessage();
  BLEDevice::init("STACJA_METEOROLOGICZNA");        // wybranie nazwy dla urządzenia
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue(info.c_str());  // wybranie charakterystyki do wysłania
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();            // rozpoczęcie nadawania
  
  Serial.println("Rozpoczynam nadawanie!");
  Serial.println("Zdefiniowano charakterystykę: ");
  Serial.println(info.c_str());
  
  readingID++;
  unsigned long time_start = millis();
  delay(30000 - time_start);  // od głównego czasu opóźnienia odejmowany jest czas wykonywania programu (całkowity czas działania programu będzie zawsze stały)
  digitalWrite(LED, HIGH);    // wyłącznie wbudowanego LEDa
//  Serial.println(time_start);
  Serial.println("GOTOWE! Przechodzę w tryb głębokiego snu.");
  esp_sleep_enable_timer_wakeup(600000000);
  esp_deep_sleep_start();
}

// Ta pętla nigdy nie zostanie osiągnięta
void loop() { 
}
