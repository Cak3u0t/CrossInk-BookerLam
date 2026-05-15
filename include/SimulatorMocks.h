#pragma once
#include <stdint.h>
#include <unistd.h>

// Mocking Arduino-specific types and functions for the Mac
typedef uint8_t byte;
#define HIGH 0x1
#define LOW 0x0

inline void delay(int ms) { usleep(ms * 1000); }

class SerialMock {
 public:
  void begin(int speed) {}
  void println(const char* s) { printf("%s\n", s); }
};
static SerialMock Serial;

// #pragma once
// #ifdef SIMULATOR
// #include <stdint.h>
// #include <string>
// #include <atomic>
//
// // --- 1. GIẢ LẬP CROSSPOINTWEBSERVER ---
// class CrossPointWebServer {
// public:
//   CrossPointWebServer() {}
//   ~CrossPointWebServer() {}
//   void begin() {}
//   void stop() {}
//   void handleClient() {}
//   bool getWsUploadStatus() const { return false; }
// };
//
// // --- 2. GIẢ LẬP OTAUPDATER ---
// namespace OtaUpdater {
//   inline void checkForUpdate() {}
//   inline bool isUpdateNewer() const { return false; }
//   inline std::string getLatestVersion() const { return "1.0.0-sim"; }
//   inline void installUpdate(void (*callback)(void*), void* arg, std::atomic<bool>* cancel) {}
// }
//
// // --- 3. GIẢ LẬP FIRMWARE_FLASH ---
// namespace firmware_flash {
//   enum Result {
//     SUCCESS = 0,
//     ERROR_FILE_NOT_FOUND,
//     ERROR_INVALID_IMAGE
//   };
//
//   inline Result validateImageFile(const char* path, unsigned long size) {
//     return SUCCESS;
//   }
//
//   inline const char* resultName(Result r) {
//     return "SIM_SUCCESS";
//   }
//
//   inline Result flashFromSdPath(const char* path, void (*progress)(unsigned long, unsigned long, void*), void* arg, bool flag) {
//     return SUCCESS;
//   }
// }
// #endif
