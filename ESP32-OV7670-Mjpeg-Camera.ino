// Author : Mudassar Tamboli, Kautism<darkautism@gmail.com>

#include <Arduino.h>
#include <JPEGENC.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "OV7670.h"

#define ssid F("darkautism")
#define password F("darkautism")

#define SIOD 21
#define SIOC 22
#define VSYNC 34
#define HREF 35
#define XCLK 32
#define PCLK 33
#define D0 27
#define D1 17
#define D2 16
#define D3 15
#define D4 14
#define D5 13
#define D6 26
#define D7 4


#define NonUsedBuffer() (workingbuf == 1) ? bufferA : bufferB
#define FillingBuffer() (workingbuf == 0) ? bufferA : bufferB
#define JPEG_BUF_SIZE 14000

uint8_t jpgBuf[JPEG_BUF_SIZE];

OV7670 *camera;

WebServer server(80);

const char HEADER[] =
  "HTTP/1.1 200 OK\r\n"
  "Access-Control-Allow-Origin: *\r\n"
  "Content-Type: multipart/x-mixed-replace; "
  "boundary=darkautism\r\n";
const char BOUNDARY[] = "\r\n--darkautism\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const uint16_t hdrLen = strlen(HEADER);
const uint16_t bdrLen = strlen(BOUNDARY);
const uint16_t cntLen = strlen(CTNTTYPE);

enum ScanType : uint8_t {
  Infinite,
  Once,
  LastFrame,
};

// Sync stuff
uint8_t *bufferA = nullptr;
uint8_t *bufferB = nullptr;
uint8_t workingbuf = 0;
struct ScanRequest {
  uint16_t startRow;
  uint16_t endRow;
  uint8_t *targetBuffer;
  ScanType type;
};

QueueHandle_t scanRequestQueue = xQueueCreate(1, sizeof(ScanRequest));
SemaphoreHandle_t bufferReadySemaphore = xSemaphoreCreateBinary();

int encodeJpeg(uint8_t *jpgBuf, size_t bufsize) {
  uint8_t rc = JPEGE_SUCCESS;
  JPEGENC jpg;
  rc = jpg.open(jpgBuf, bufsize);
  if (rc == JPEGE_SUCCESS) {
    JPEGENCODE jpe;
    rc = jpg.encodeBegin(&jpe, camera->xres, camera->yres, JPEGE_PIXEL_RGB565,
                         JPEGE_SUBSAMPLE_420, JPEGE_Q_MED);
    if (rc == JPEGE_SUCCESS) {
      uint8_t blk_count =
        (camera->yres + I2SCamera::blockSlice - 1) / I2SCamera::blockSlice;
      if (blk_count > 1) {
        for (uint8_t i = 0; i < blk_count; i++) {
          uint16_t startRow = 0;
          uint16_t endRow = 0;
          ScanType type = (i + 1 == blk_count) ? LastFrame : Once;
          if (type == LastFrame) {
            startRow = 0;
          } else {
            startRow = (i + 1) * I2SCamera::blockSlice;
          }
          endRow = min(startRow + I2SCamera::blockSlice, camera->yres);
          uint8_t *targetBuffer = FillingBuffer();
          uint8_t *sendBuffer = NonUsedBuffer();
          workingbuf ^= 1;
          ScanRequest req = { startRow, endRow, sendBuffer, (i + 1 == blk_count) ? LastFrame : Once };
          xQueueSend(scanRequestQueue, &req, portMAX_DELAY);

          xSemaphoreTake(bufferReadySemaphore, portMAX_DELAY);
          // 之前的startRow是給送出用的，我們用的是這個
          startRow = startRow = i * I2SCamera::blockSlice;
          endRow = min(startRow + I2SCamera::blockSlice, camera->yres);

          for (uint16_t my = startRow; my < endRow && rc == JPEGE_SUCCESS; my += 16) {
            for (uint16_t mx = 0; mx < camera->xres && rc == JPEGE_SUCCESS; mx += 16) {
              uint16_t localRow = my - startRow;
              uint8_t *mcuStart =
                targetBuffer + (localRow * camera->xres + mx) * 2;
              rc = jpg.addMCU(&jpe, mcuStart, camera->xres * 2);
            }
          }
          if (rc != JPEGE_SUCCESS) {
            Serial.print(F("jpg failed err:"));
            Serial.println(rc);
            break;
          }
        }
      } else {
        uint8_t *targetBuffer = FillingBuffer();
        uint8_t *sendBuffer = NonUsedBuffer();
        workingbuf ^= 1;
        ScanRequest req = { 0, I2SCamera::blockSlice, sendBuffer, LastFrame };
        xQueueSend(scanRequestQueue, &req, portMAX_DELAY);
        xSemaphoreTake(bufferReadySemaphore, portMAX_DELAY);
        rc = jpg.addFrame(&jpe, targetBuffer, camera->xres * 2);
      }

      if (rc == JPEGE_SUCCESS) {
        return jpg.close();
      }
    }
  }
  return -1;  // error
}

void handle_jpg_stream(void) {
  char buf[32];
  WiFiClient client = server.client();
  client.write(HEADER, hdrLen);
  client.write(BOUNDARY, bdrLen);
  while (true) {
    if (!client.connected()) break;
    uint16_t jpgSize = encodeJpeg(jpgBuf, JPEG_BUF_SIZE);

    if (jpgSize > 0) {
      client.write(CTNTTYPE, cntLen);
      sprintf(buf, "%d\r\n\r\n", jpgSize);
      client.write(buf, strlen(buf));
      client.write((char *)jpgBuf, jpgSize);
      client.write(BOUNDARY, bdrLen);
    } else {
      break;  // handle error
    }

  }
}


void cameraTask(void *pvParameters) {
  ScanRequest request;
  uint8_t ret = 0;
  while (true) {
    vTaskDelay(1);
    ret = xQueueReceive(scanRequestQueue, &request, (request.type == Infinite) ? 0 : pdMS_TO_TICKS(10));
    if (ret == pdPASS || request.type == Infinite) {
      if (request.type != Infinite)
        xSemaphoreGive(bufferReadySemaphore);
      if (request.type == LastFrame)
        request.type = Infinite;

      camera->startBlock = request.startRow + 1;
      camera->endBlock = request.endRow;
      camera->oneFrame(request.targetBuffer);
    }
  }
}

//uint8_t FakeBuffer[320*128*2];

void setup() {
  Serial.begin(115200);
  camera = new OV7670(OV7670::Mode::QVGA_RGB565, SIOD, SIOC, VSYNC, HREF, XCLK,
                      PCLK, D0, D1, D2, D3, D4, D5, D6, D7);

  const int bufferSize = camera->xres * I2SCamera::blockSlice * 2;
  bufferA = (uint8_t *)malloc(bufferSize);
  bufferB = (uint8_t *)malloc(bufferSize);
  if (bufferA == nullptr || bufferB == nullptr) {
    Serial.println(F("Failed to allocate buffers"));
    while (1)
      ;
  }
  xTaskCreatePinnedToCore(cameraTask, "CameraTask", 1024, NULL, 1, NULL, 0);
  // 初始無限掃描
  ScanRequest initReq = { 0, I2SCamera::blockSlice, bufferA, Infinite };
  xQueueSend(scanRequestQueue, &initReq, portMAX_DELAY);
  IPAddress ip;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  ip = WiFi.localIP();
  Serial.println(F("Connected"));
  Serial.print(F("Stream Link: http://"));
  Serial.print(ip);
  Serial.println(F("/stream"));
  server.on(F("/stream"), HTTP_GET, handle_jpg_stream);
  server.begin();
}

void loop() {
  server.handleClient();
}
