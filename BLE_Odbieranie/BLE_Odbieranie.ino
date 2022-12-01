// Biblioteki wykorzystywane w programie
#include <string>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <time.h>
#include <BLEDevice.h>
#include <DS1307.h>

// Zmienne wykorzystywane w programie
#define LED 22                    // numer pinu odpowiedzialnego za stan LED na płtyce
#define I2C_SDA 17                // Piny, które zostaną wykorzystane do
#define I2C_SCL 16                // stworzenia magistralii I2C
std::string zmienna;              // zmienna, do której wpisywane są sczytane wartości
std::string sec_end;
std::string sec_start;
std::string millis_sec;
std::string dane;
RTC_DATA_ATTR int readingID = 0;  // zmienna przechowująca numer pomiaru w pamięci RTC
DS1307 rtc;                       // instacja zewnętrznego RTC do skalibrowania

static BLEUUID    serviceUUID("b1702e41-2dc3-474c-855b-1e4355291eb6");  // usługa zdalna, z którą chcemy się połączyć
static BLEUUID    charUUID("9a22faec-e998-4d85-ab0f-0d8683c99989");     // charakterystyka interesującej nas usługi zdalnej
static boolean doConnect = false;                                       // flaga sprawdzająca czy ustanowowić połączenie
static boolean connected = false;                                       // flaga sprawdzająca stan połączenia
static boolean doScan = false;                                          // flaga potrzebna do ponownego uruchomienia skanowania
static BLERemoteCharacteristic* pRemoteCharacteristic;                  // wskaźnik do charakterystyki
static BLEAdvertisedDevice* myDevice;                                   // wskaźnik do urządzenia BLE

// Funkcja odpowiedzialna za utworzenie pliku do zapisu danych
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Tworzenie i zapisywanie pliku: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Nie udało się otworzyć pliku do zapisu!");
    return;
  }
  if (file.print(message)) {
    Serial.println("Plik został utworzony!");
  } else {
    Serial.println("Tworzenie pliku nie powiodło się!");
  }
  file.close();
}

// Funkcja odpowiedzialna za dopisywanie danych do utworzonego wcześniej pliku
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Dopisywanie danych do pliku: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Nie udało się otworzyć pliku do dopisania danych!");
    return;
  }
  if (file.print(message)) {
    Serial.println("Dane zostały dopisane!");
  }
  else {
    Serial.println("Dopisywanie danych nie powiodło się!");
  }
  file.close();
}

// Funkcja do powiadomienia o odpowiedzi zwrotnej
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Powiadomienie o informacji zwrotnej dla charakterystyki ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" o długości danych ");
  Serial.println(length);
  Serial.print("dane: ");
  Serial.println((char*)pData);
}

// Klasa do wywołania informacji zwrotnej
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
    }
    void onDisconnect(BLEClient* pclient) {
      connected = false;
      Serial.println("Rozłączono");
    }
};

// Funkcja nawiązująca połączenie z urządzeniem
bool connectToServer() {
  Serial.print("Nawiązywanie połączenia z urządzeniem o adresie: ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Stworzono klienta");

  pClient->setClientCallbacks(new MyClientCallback());

  pClient->connect(myDevice);
  Serial.println(" - Nawiązano połączenie z urządzeniem");

  // Uzyskujemy odniesienie do usługi, której szukamy.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Nie udało się znaleźć poszukiwanego identyfikatora UUID usługi: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Znaleziono usługę");


  // Uzyskujemy odniesienie do charakterystyki usługi, której szukamy
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Nie udało się znaleźć identyfikatora UUID charakterystyki: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Znaleziono poszukiwaną charakterystykę");

  // Odczytano wartość charakterystyki
  if (pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
    Serial.print("Odczytana wartość charakterystyki: ");
    Serial.println(value.c_str());
    zmienna = value;  // wpisanie wartości charakterystyki do zmiennej

    // Część odpowiedzialna za odczytanie danych do kalibracji modułów
    int pos = zmienna.find_first_of('|');
    std::string first = zmienna.substr(pos + 1), last = zmienna.substr(0, pos);

    pos = last.find_first_of(':');
    first = last.substr(pos + 1), last = last.substr(0, pos);
    milli_sec = last;

    pos = first.find_first_of(':');
    first = first.substr(pos + 1), last = first.substr(0, pos);

    pos = first.find_first_of(':');
    first = first.substr(pos + 1), last = first.substr(0, pos);
    sec_start = first;
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  return true;
}
// Klasa odpowiedzialna za poszukiwanie urządzenia BLE, który nadaje poszukiwaną usługę
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    // Wywoływane dla każdego nadającego urządzenia BLE
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("Znalezione urządzenie BLE: ");
      Serial.println(advertisedDevice.toString().c_str());

      // Sprawdzamy czy znalezione urządzenia zawierają poszukiwaną usługę, jeśli tak to wyłączamy skanowanie i nawiązujemy połączenie
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;
        sec_end = rtc.getDateTime().second;
      }
    }
};

// Program główny
void setup() {
  Serial.begin(115200);
  rtc.begin(I2C_SDA, I2C_SCL);             // zmiana pinów I2C dla RTC
  pinMode (LED, OUTPUT);
  digitalWrite(LED, LOW);   // włączenie wbudowanego LEDa
  Serial.println("Uruchamianie stacji archiwizującej...");
  BLEDevice::init("");

  // Rozpoczęcie skanowania, wybranie funkcji do wywołania po znalezieniu nowego urządzenia.
  // Ustawienie skanowania na 13 sekund (po znalezieniu szukanego urządzenia zostanie ono przerwane).
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(13, true);

  delay(2000);  // Opóźnienie potrzebne po znalezieniu urządzenia

  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Nawiązano połączenie z serwerem BLE");
    }
    else {
      Serial.println("Nie udało się połączyć z serwerem.");
    }
    doConnect = false;
  }

  // Jeśli połączono się z urządzeniem - zapisz odczytaną charakterystykę na kartę microSD
  if (connected) {
    // Przy pierwszym uruchomieniu utwórz plik do zapisu pomiarów
    if (readingID == 0) {
      if (!SD.begin()) {
        Serial.println("Zamontowanie karty nie powiodło się!");
        return;
      }
      writeFile(SD, "/Stacja_meteorologiczna_ESP32.txt", "Numer pomiaru; Czas[hh:mm:ss]; Data[dd-mm-rrrr]; Temperatura[*C]; Ciśnienie[hPa]; Przybliżona wysokość n.p.m.[m]; Wilgotność[%] \r\n");
    }
    Serial.println(dane.c_str());
    SD.begin();
    appendFile(SD, "/Stacja_meteorologiczna_ESP32.txt", dane.c_str());
  }
  else if (doScan) {
    BLEDevice::getScan()->start(0);  // w razie przerwania połączenia, uruchamiane jest ponowne skanowanie
  }

  unsigned long end_sec = std::stoul(sec_end, nullptr, 0);
  unsigned long start_sec = std::stoul(sec_start, nullptr, 0);

  readingID++;
  unsigned long time_start = millis();
  delay(30000 - time_start - millis_sec - (end_sec - start_sec)*1000);  // od głównego czasu opóźnienia odejmowany jest czas wykonywania programu i różnica przy wybudzeniu modułów (całkowity czas działania programu będzie zawsze stały)
  digitalWrite(LED, HIGH);    // wyłączenie wbudowanego LEDa
  //  Serial.println(time_start);
  Serial.println("GOTOWE! Przechodzę w tryb głębokiego snu.");
  esp_sleep_enable_timer_wakeup(600000000);
  esp_deep_sleep_start();
}

// Ta pętla nigdy nie zostanie osiągnięta
void loop() {
}
