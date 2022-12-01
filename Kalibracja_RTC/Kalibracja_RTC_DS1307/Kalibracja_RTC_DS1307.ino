// Biblioteki wykorzystywane w programie
#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <DS1307.h>

#define I2C_SDA 17    // Piny, które zostaną wykorzystane do
#define I2C_SCL 16    //    stworzenia magistralii I2C

DS1307 rtc;                                  // instacja zewnętrznego RTC do skalibrowania

const char* ssid       = "LOGIN";            // login oraz hasło do
const char* password   = "HASŁO";            //     sieci WiFi

const char* ntpServer = "pl.pool.ntp.org";   // polski serwer NTP do odczytania dokładnej godziny
const long  gmtOffset_sec = 3600;            // ustawienia dla naszej strefy czasowej
const int   daylightOffset_sec = 3600;       // oraz wykrywania czasu zimowego/letniego

struct tm timeinfo;                          // zmienna czasu systemu

// Funkcja przypisująca pobrany czas i datę z serwera NTP oraz wyświetlająca go
void printLocalTime() {
  if(!getLocalTime(&timeinfo)){
    Serial.println("Nie udało się uzyskać czasu!");
    return;
  }
  Serial.println(&timeinfo, "%A, %d %B %Y %H:%M:%S");
}

// Program główny
void setup() {
  Serial.begin(115200);
  rtc.begin(I2C_SDA, I2C_SCL);
  
  // nawiązywanie połączenia z siecią WiFi
  Serial.printf("Łączenie z siecią:  %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" Połączono z siecią WiFi");
 
  // zainicjuj zmianę oraz uzyskaj czas
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  // kalibrowanie zewnętrznego RTC
  if (!rtc.isReady()) {
    // Ustawienie daty i godziny (YYYY, MM, DD, HH, MM, SS)
    rtc.setDateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  }
  
  // rozłączenie się z siecią WiFi - nie jest już więcej potrzebna
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  
  Serial.println(rtc.dateFormat("d-m-Y H:i:s", rtc.getDateTime()));   // wypisanie daty i czasu
  Serial.print("\r\n");
}

void loop() {
}
