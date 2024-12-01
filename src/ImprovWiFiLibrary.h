#pragma once

#ifndef IMPROV_RUN_FOR
#define IMPROV_RUN_FOR 60000
#endif

#if defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
  #define WIFI_OPEN ENC_TYPE_NONE 
  #include <EEPROM.h>
  #define EEPROM_SIZE 96
#elif defined(ARDUINO_ARCH_ESP32)
  #include <Preferences.h>
  #include <WiFi.h>
  #define WIFI_OPEN WIFI_AUTH_OPEN
#endif

#include <Stream.h>
#include "ImprovTypes.h"
#include <functional>

#ifdef ARDUINO
  #include <Arduino.h>
#endif

/**
 * Improv WiFi class
 *
 * ### Description
 *
 * Handles the Improv WiFi Serial protocol (https://www.improv-wifi.com/serial/)
 *
 * ### Example
 *
 * Simple example of using ImprovWiFi lib. A complete one can be seen in `examples/` folder.
 *
 * ```cpp
 * #include <ImprovWiFiLibrary.h>
 *
 * ImprovWiFi improvSerial(&Serial);
 *
 * void setup() {
 *   improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32, "My-Device-9a4c2b", "2.1.5", "My Device");
 * }
 *
 * void loop() {
 *   improvSerial.loop();
 * }
 * ```
 *
 */
class ImprovWiFi
{
private:
  const char *const CHIP_FAMILY_DESC[5] = {"ESP32", "ESP32-C3", "ESP32-S2", "ESP32-S3", "ESP8266"};
  ImprovTypes::ImprovWiFiParamsStruct improvWiFiParams;

  uint8_t  _buffer[128];
  uint8_t  _position = 0;
  uint32_t _stopme   = 0;
  String    SSID     = "";
  String    PASSWORD = "";

  Stream *serial;

  bool      connectFailure;
  uint16_t  maxConnectRetries;
  uint16_t  numConnectRetriesDone;
  uint16_t  retryDelay;
  uint32_t  millisLastConnectTry;
  bool      lastConnectStatus;

  void sendDeviceUrl(ImprovTypes::Command cmd);
  bool onCommandCallback(ImprovTypes::ImprovCommand cmd);
  void onErrorCallback(ImprovTypes::Error err);
  void setState(ImprovTypes::State state);
  void sendResponse(std::vector<uint8_t> &response);
  void setError(ImprovTypes::Error error);
  void getAvailableWifiNetworks();
  inline void replaceAll(std::string &str, const std::string &from, const std::string &to);
  void saveWiFiCredentials(std::string* ssid, std::string* password);
  void loadWiFiCredentials(String &ssid, String &password);
  
  // improv SDK
  bool parseImprovSerial(size_t position, uint8_t byte, const uint8_t *buffer);
  ImprovTypes::ImprovCommand parseImprovData(const std::vector<uint8_t> &data, bool check_checksum = true);
  ImprovTypes::ImprovCommand parseImprovData(const uint8_t *data, size_t length, bool check_checksum = true);
  std::vector<uint8_t> build_rpc_response(ImprovTypes::Command command, const std::vector<std::string> &datum, bool add_checksum);

  #ifdef ESP32
    Preferences preferences;
  #endif

public:
  /**
   * ## Constructors
   **/

  /**
   * Create an instance of ImprovWiFi
   *
   * ## Parameters
   *
   * - `serial` - Pointer to stream object used to handle requests, for the most cases use `Serial`
   */
  ImprovWiFi(Stream *serial);
  
  /**
   * ## Type definition
   */

  /**
   * Callback function called when any error occurs during the protocol handling or wifi connection.
   */
  std::function<void(ImprovTypes::Error)> onImprovErrorCallback;

  void onImprovError(std::function<void(ImprovTypes::Error)> cb) {
    onImprovErrorCallback = cb;
  }

  /**
   * Callback function called when the attempt of wifi connection is successful. It informs the SSID and Password used to that, it's a perfect time to save them for further use.
   */
  std::function<void(const char *ssid, const char *password)> onImprovConnectedCallback;

  void onImprovConnected(std::function<void(const char *ssid, const char *password)> cb) {
    onImprovConnectedCallback = cb;
  }

  /**
   * Callback function to customize the wifi connection if you needed. Optional.
   */
  std::function<bool(const char *ssid, const char *password)> customConnectWiFiCallback;

  void setCustomConnectWiFi(std::function<bool(const char *ssid, const char *password)> cb) {
    customConnectWiFiCallback = cb;
  }

  /**
   * Callback function to customize the wifi credential saving if you needed. Optional.
   */
  std::function<void(std::string *ssid, std::string *password)> customWiFiCredentialSavingCallback;

  void setCustomWiFiCredentialSaving(std::function<void(std::string *ssid, std::string *password)> cb) {
    customWiFiCredentialSavingCallback = cb;
  }

  /**
   * Callback function to customize the wifi credential saving if you needed. Optional.
   */
  std::function<void(String &ssid, String &password)> customWiFiCredentialLoadingCallback;

  void setCustomWiFiCredentialLoading(std::function<void(String &ssid, String &password)> cb) {
    customWiFiCredentialLoadingCallback = cb;
  }

  /**
   * Check if a communication via serial is happening. Put this call on your loop().
   *
   */
  void loop();
  bool handleBuffer(uint8_t *buffer, uint16_t bytes);

  /**
   * Set details of your device.
   *
   * # Parameters
   *
   * - `chipFamily` - Chip variant, supported are CF_ESP32, CF_ESP32_C3, CF_ESP32_S2, CF_ESP32_S3, CF_ESP8266. Consult ESP Home [docs](https://esphome.io/components/esp32.html) for more information.
   * - `firmwareName` - Firmware name
   * - `firmwareVersion` - Firmware version
   * - `deviceName` - Your device name
   * - `deviceUrl`- The local URL to access your device. A placeholder called {LOCAL_IPV4} is available to form elaboreted URLs. E.g. `http://{LOCAL_IPV4}?name=Guest`.
   *   There is overloaded method without `deviceUrl`, in this case the URL will be the local IP.
   *
   */
  void setDeviceInfo(ImprovTypes::ChipFamily chipFamily, const char *firmwareName, const char *firmwareVersion, const char *deviceName, const char *deviceUrl);
  void setDeviceInfo(ImprovTypes::ChipFamily chipFamily, const char *firmwareName, const char *firmwareVersion, const char *deviceName);

  /**
   * Default method to connect in a WiFi network.
   * It waits `DELAY_MS_WAIT_WIFI_CONNECTION` milliseconds (default 500) during `MAX_ATTEMPTS_WIFI_CONNECTION` (default 20) until it get connected. If it does not happen, an error `ERROR_UNABLE_TO_CONNECT` is thrown.
   *
   */
  bool tryConnectToWifi(const char *ssid, const char *password);
  
  /**
   * regular method to connect to wifi with present credentials
   */
  bool ConnectToWifi(bool firstRun);

  /**
   * Check if connection is established using `WiFi.status() == WL_CONNECTED`
   *
   */
  bool isConnected();

};
