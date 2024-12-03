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
#include <vector>

#ifdef ARDUINO
  #include <Arduino.h>
#endif

/**
 * Improv WiFi class
 *
 * @brief Handles the Improv WiFi Serial protocol (https://www.improv-wifi.com/serial/)
 *
 * @attention This library is compatible with ESP32 and ESP8266.
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
  bool      WifiCredentialsAvailable = false;

  void sendDeviceUrl(ImprovTypes::Command cmd);
  bool onCommandCallback(ImprovTypes::ImprovCommand cmd);
  void onErrorCallback(ImprovTypes::Error err);
  void setState(ImprovTypes::State state);
  void sendResponse(std::vector<uint8_t> &response);
  void setError(ImprovTypes::Error error);
  void getAvailableWifiNetworks();
  inline void replaceAll(std::string &str, const std::string &from, const std::string &to);
  bool saveWiFiCredentials(std::string* ssid, std::string* password);
  bool loadWiFiCredentials(String &ssid, String &password);
  
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
   * @brief Constructor, create an instance of ImprovWiFi
   *
   * @param serial Pointer to stream object used to handle requests, for the most cases use `Serial`
   */
  ImprovWiFi(Stream *serial);
  
  /**
  * @brief     Callback functions called when any error occurs during the protocol handling or wifi connection.
  *            Multiple callbacks can be set.
  *
  * @param     Error  error message
  *
  * @return
  *    - none
  */
  void onImprovError(std::function<void(ImprovTypes::Error)> cb) {
    onImprovErrorCallbacks.push_back(cb);
  }
  std::vector<std::function<void(ImprovTypes::Error)>> onImprovErrorCallbacks;

  /**
  * @brief     Callback functions called when the attempt of wifi connection is successful. 
  *            It informs the SSID and Password used to that, it's a perfect time to save them for further use.
  *            Multiple callbacks can be set.
  *
  * @param     ssid  wifi ssid
  * @param     password  wifi password
  *
  * @return
  *    - none
  */
  void onImprovConnected(std::function<void(const char *ssid, const char *password)> cb) {
    onImprovConnectedCallbacks.push_back(cb);
  }
  std::vector<std::function<void(const char *ssid, const char *password)>> onImprovConnectedCallbacks;

  /**
  * @brief     Callback function to customize the wifi connection if you needed. Optional.
  *  
  * @attention If you set this callback, the default connection method will be ignored.
  *
  * @param     ssid  wifi ssid
  * @param     password  wifi password
  *
  * @return
  *    - none
  */
  void setCustomConnectWiFi(std::function<bool(const char *ssid, const char *password)> cb) {
    customConnectWiFiCallback = cb;
  }
  std::function<bool(const char *ssid, const char *password)> customConnectWiFiCallback;

 
  /**
  * @brief     Callback function to customize the wifi credential saving if you needed. Optional.
  *  
  * @attention If you set this callback, the default saving method will be ignored.
  *
  * @param     ssid  wifi ssid
  * @param     password  wifi password
  *
  * @return    
  *   - bool  true if the credentials were saved successfully
  */
  void setCustomWiFiCredentialSaving(std::function<bool(std::string *ssid, std::string *password)> cb) {
    customWiFiCredentialSavingCallback = cb;
  }
  std::function<bool(std::string *ssid, std::string *password)> customWiFiCredentialSavingCallback;


  /**
  * @brief     Callback function to customize the wifi credential loading if you needed. Optional.
  *  
  * @attention If you set this callback, the default loading method will be ignored.
  *
  * @param     ssid  wifi ssid
  * @param     password  wifi password
  *
  * @return    
  *   - bool  true if the credentials were loaded successfully
  */
   void setCustomWiFiCredentialLoading(std::function<bool(String &ssid, String &password)> cb) {
    customWiFiCredentialLoadingCallback = cb;
  }
  std::function<bool(String &ssid, String &password)> customWiFiCredentialLoadingCallback;


  /**
  * @brief     Check if a communication via serial is happening. It handles also wifi reconnection.
  *            Put this call on your loop().
  * 
  * @attention Use "onImprovError" callback to handle wifi connection errors.
  *  
  */
  void loop();

  bool handleBuffer(uint8_t *buffer, uint16_t bytes);

  
  /**
  * @brief     Set details of your device. It's used to inform the ImprovWiFi library about your device.
  *  
  * @param     chipFamily  Chip variant, supported are CF_ESP32, CF_ESP32_C3, CF_ESP32_S2, CF_ESP32_S3, CF_ESP8266
  * @param     firmwareName  Firmware name
  * @param     firmwareVersion  Firmware version
  * @param     deviceName  Your device name
  * @param     deviceUrl  The local URL to access your device. A placeholder called {LOCAL_IPV4} is available to form elaboreted URLs. E.g. `http://{LOCAL_IPV4}?name=Guest`.
  *     There is overloaded method without `deviceUrl`, in this case the URL will be the local IP.  
  *
  * @return    
  *   - none
  */
  void setDeviceInfo(ImprovTypes::ChipFamily chipFamily, const char *firmwareName, const char *firmwareVersion, const char *deviceName, const char *deviceUrl);
  void setDeviceInfo(ImprovTypes::ChipFamily chipFamily, const char *firmwareName, const char *firmwareVersion, const char *deviceName);

  
  /**
  * @brief     Default method to connect in a WiFi network.
  *   It waits `DELAY_MS_WAIT_WIFI_CONNECTION` milliseconds (default 500) during `MAX_ATTEMPTS_WIFI_CONNECTION` (default 20) until it get connected. 
  *   If it does not happen, an error `ERROR_UNABLE_TO_CONNECT` is thrown.
  *  
  * @param     ssid  wifi ssid
  * @param     password  wifi password
  *
  * @return    
  *   - bool  true if the credentials were loaded successfully
  */
  bool tryConnectToWifi(const char *ssid, const char *password);
  
 
  /**
  * @brief     regular method to connect to wifi with present credentials.
  *   Use this method in your setup function to connect to wifi. Optional.
  *  
  * @param     ssid  wifi ssid
  * @param     password  wifi password
  *
  * @return    
  *   - bool  true if the credentials were loaded successfully
  */
  bool ConnectToWifi(bool firstRun);

  /**
   * @brief if connection is established using `WiFi.status() == WL_CONNECTED`
   */
  bool isConnected();

};
