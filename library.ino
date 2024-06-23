#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "AndroidHotspot9738";  // WiFi 네트워크 이름
const char* password = "023759738";  // WiFi 비밀번호
const char* serverUrl = "http://52.79.204.104:8080/book/located-book";  // 데이터를 전송할 서버의 URL

String* rfidArray;  // RFID 데이터를 담을 배열
String location;  // 위치 정보를 저장할 변수
int rfidCount = 0;    // RFID 데이터의 개수
int maxRfidCount = 20;  // 초기 RFID 배열 크기

void setup() {
  Serial.begin(115200);
  // ESP32에서는 UART 통신으로 RFID 데이터를 받아옴
  Serial2.begin(115200, SERIAL_8N1, 16, 17);  // RX, TX
  // WiFi 연결
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  rfidArray = new String[maxRfidCount];  // 초기 RFID 배열 할당
}

void loop() {
  if (Serial2.available() > 0) {
    while (Serial2.available() > 0 && rfidCount < maxRfidCount) {
      char data[15];
      Serial2.readBytes(data, 15);
      
      // 위치 데이터 추출 (첫 3바이트를 ASCII 문자열로 변환)
      char locationData[4];
      strncpy(locationData, data, 3);
      locationData[3] = '\0';  // 널 문자 추가
      location = String(locationData);
      
    
      // RFID 데이터 추출 (첫 3바이트 이후 12바이트를 16진수 문자열로 저장)
      char rfidData[25];  // 12바이트 * 2 + 널 문자
      for (int i = 0; i < 12; i++) {
        byte upperNibble = data[3 + i] >> 4;
        byte lowerNibble = data[3 + i] & 0x0F;
        rfidData[2*i] = upperNibble < 0xA ? '0' + upperNibble : 'A' + (upperNibble - 0xA);
        rfidData[2*i + 1] = lowerNibble < 0xA ? '0' + lowerNibble : 'A' + (lowerNibble - 0xA);
      }
      rfidData[24] = '\0';  // 널 문자 추가
      String hexData = String(rfidData);

      // 데이터 배열에 저장
      rfidArray[rfidCount] = hexData;
      rfidCount++;
    }

    if (rfidCount > 0) {
      sendRFIDDataToServer();  // 서버에 데이터를 전송
    }
  }
}

void sendRFIDDataToServer() {
  // JSON 데이터를 생성
  DynamicJsonDocument jsonDocument(1024);
  JsonArray rfidJsonArray = jsonDocument.createNestedArray("rfidList");

  for (int i = 0; i < rfidCount; i++) {
    rfidJsonArray.add(rfidArray[i]);
  }

  jsonDocument["location"] = location;  // 위치 정보를 추가

  // JSON 데이터를 문자열로 변환
  String jsonString;
  serializeJson(jsonDocument, jsonString);

 // 시리얼 모니터에 JSON 데이터 출력
  Serial.println("Sending JSON data to server:");
  Serial.println(jsonString);
  // HTTPClient를 사용하여 서버에 POST 요청을 전송
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonString);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);

     // 응답이 빈 배열인지 확인 ([] 인 경우)
    if (response != "[]") {
      // 빈 배열이 아니면 UART2로 응답 전송(STM32,라즈베리파이)
      Serial2.println(response);
    }
    
  }
  else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  delete[] rfidArray;  // 동적으로 할당된 배열 메모리 해제
  rfidCount = 0;  // 데이터 전송 후 카운트 초기화
  rfidArray = new String[maxRfidCount];  // 새로운 배열 할당
}

