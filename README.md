# Improv WiFi Library

Improv is a free and open standard with ready-made SDKs that offer a great user experience to configure Wi-Fi on devices. More details [here](https://www.improv-wifi.com/).

This library provides an easier way to handle the Improv serial protocol in your firmware, allowing you to configure the WiFi via web browser as in the following [example](https://jnthas.github.io/improv-wifi-demo/).

**This fork of orininal library includes the complete wifi handling (storing, loading, connect and reconnect)**

Simplest use example:

```cpp
#include <ImprovWiFiLibrary.h>

ImprovWiFi improvSerial(&Serial);

void setup() {
  improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32, "My-Device-9a4c2b", "2.1.5", "My Device");
  improvSerial.ConnectToWifi();
}

void loop() { 
  improvSerial.loop();
}
```

## Documentation

The full library documentation can be seen in [docs/](docs/ImprovWiFiLibrary.md) folder.


## License

This open source code is licensed under the MIT license (see [LICENSE](LICENSE)
for details).