// Biblioteki wykorzystywane w programie
#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <Rtc_Pcf8563.h>

#define I2C_SDA 17    // Piny, które zostaną wykorzystane do
#define I2C_SCL 18    //    stworzenia magistralii I2C

Rtc_Pcf8563 rtc;                             // instacja zewnętrznego RTC do skalibrowania

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
  Wire.begin(I2C_SDA, I2C_SCL); // zmiana pinów I2C dla RTC

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
  rtc.initClock();
  rtc.setDate(timeinfo.tm_mday, timeinfo.tm_wday, timeinfo.tm_mon + 1, 0, timeinfo.tm_year - 100);
  rtc.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  
  // rozłaczenie się z siecią WiFi - nie jest już więcej potrzebna
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  Serial.print(rtc.formatTime(RTCC_TIME_HMS));    // wypisanie czasu
  Serial.print("\r\n");
  Serial.print(rtc.formatDate(RTCC_DATE_WORLD));  // wypisanie daty
  Serial.print("\r\n");
}

void loop() {
}
