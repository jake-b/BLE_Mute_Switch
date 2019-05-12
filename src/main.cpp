
/**
 * main.cpp - for BLE Mute Switch
 * Use the an "iTag" device as a remote control mute switch.
 * Based on sample code who's author is unknown, updated by chegewara
 */

// Basic includes
#include <Arduino.h>
#include <math.h>

// Stuff for BLE
#include "BLEDevice.h"
#include "esp_log.h"

// Stuff for WiFi OTA
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Constants & Globals
#define LED_BUILTIN 22
#define MUTE_PIN 17
#define PROGRAMMING_MODE_MAGIC 0xBADC0DE1

#define SSID   "BlueMute"
#define PASSWD "BlueMute"

// I have found "iTag" devices with slightly different UUIDs.
// This program will identify two of them, but you may need to modify this
// for your own iTag device.

// The remote service we wish to connect to.      
#define SERVICE_UUID1 "0000ffe0-0000-1000-8000-00805f9b34fb"
#define SERVICE_UUID2 "0000fff0-0000-1000-8000-00805f9b34fb"

// The characteristic of the remote service we are interested in.
#define CHAR_UUID1 "0000ffe1-0000-1000-8000-00805f9b34fb"
#define CHAR_UUID2 "0000fff1-0000-1000-8000-00805f9b34fb"

// Some enums to track modes
enum DeviceMode {BLE_MODE, WIFI_MODE} mode = BLE_MODE;
enum ConnectState {PRE_INIT, SCANNING, CONNECTING, CONNECTED} state = PRE_INIT;

static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

// Supporting Globals
static RTC_NOINIT_ATTR  uint32_t enterProgramMode = 0;
static float blinkRate = 1.0;
static uint32_t nextBlinkTime = 0;

// Used to blink the LED for status
inline void toggleLED(){
  int state = digitalRead(LED_BUILTIN);  // get the current state of GPIO1 pin
  digitalWrite(LED_BUILTIN, !state);     // set pin to the opposite state
}

// This function is called when the subscribed notification is received.
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

    // To enter Wifi OTA programmer mode, connect to BLE by simulating the iTAG Device.
    // Submit the magic word "0xBADCODE1".  This will reboot the device, broadcasting a wifi access point
    // For use with ArduinoOTA
    // We do this because BLE and WiFi code at the same time can be problematic due to memory constraints.

    if (length==4 && __builtin_bswap32(*(uint32_t*)pData) == PROGRAMMING_MODE_MAGIC) {
      Serial.print("Request to enter programming mode");
      enterProgramMode = PROGRAMMING_MODE_MAGIC;
      ESP.restart();
    } else {
      // Otherwise blink the LED and toggle the "MUTE_PIN"
      Serial.print("^");
      digitalWrite(LED_BUILTIN, false);
      digitalWrite(MUTE_PIN, true);
      delay(300);
      digitalWrite(LED_BUILTIN, true);
      digitalWrite(MUTE_PIN, false);
    }
}

// Used to handle connection/disconnection events
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
  	Serial.println("onDisconnect");
  	pclient->disconnect();
  	delete myDevice;
  	delete pclient;
  	state = PRE_INIT;
  	// if (pRemoteCharacteristic != NULL) {
  	// 	Serial.print("Unregistring callback...");
  	// 	pRemoteCharacteristic->registerForNotify(NULL);
  	// 	Serial.println("done.");
  	// }
    
  }
};

// Once a server iTag is identfied, connect.
bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    if (!pClient->connect(myDevice)) {  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)    
    	return false;
    }
    Serial.println(" - Connected to server");

    // UUDIs on the Stack rather than the heap.
    BLEUUID serviceUUID1(SERVICE_UUID1);
    BLEUUID serviceUUID2(SERVICE_UUID2);
    BLEUUID charUUID1(CHAR_UUID1);
    BLEUUID charUUID2(CHAR_UUID2);

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = NULL;
    BLEUUID desiredCharactersitic;
    if (myDevice->isAdvertisingService(serviceUUID1)) {
      pRemoteService = pClient->getService(serviceUUID1);
      desiredCharactersitic = charUUID1;
    } else if (myDevice->isAdvertisingService(serviceUUID2)) {
      pRemoteService = pClient->getService(serviceUUID2);
      desiredCharactersitic = charUUID2;
    } else {
      Serial.printf("Failed to find our service UUIDs: %s, %s\n", serviceUUID1.toString().c_str(), serviceUUID2.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");
    Serial.println(pRemoteService->toString().c_str());

    std::map<uint16_t, BLERemoteCharacteristic*> characteristics; 
    pRemoteService->getCharacteristics(&characteristics);
    std::map<uint16_t, BLERemoteCharacteristic*>::iterator it;
    for ( it = characteristics.begin(); it != characteristics.end(); it++ ) {
      Serial.printf("index %d: %s\n",it->first, it->second->toString().c_str());
    }

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(desiredCharactersitic);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(desiredCharactersitic.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    //Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      //uint8_t value = pRemoteCharacteristic->readUInt8();
      //Serial.printf("The characteristic value was: %d\n", value);
    }

    if(pRemoteCharacteristic->canNotify()) {
	  pRemoteCharacteristic->registerForNotify(notifyCallback);
    }

    return true;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // UUDIs on the Stack rather than the heap.
    BLEUUID serviceUUID1(SERVICE_UUID1);
    BLEUUID serviceUUID2(SERVICE_UUID2);

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && 
          (advertisedDevice.isAdvertisingService(serviceUUID1) || advertisedDevice.isAdvertisingService(serviceUUID2))) {
      Serial.println("Found desired BLE Device...");
      Serial.printf("getServiceData: %s\n", advertisedDevice.getServiceData().c_str());
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      Serial.print("myDevice: ");
      Serial.printf(myDevice->toString().c_str());
      state = CONNECTING;
    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void startWifi() {
    // Configure Wifi    
    Serial.println("Starting WiFi OTA Programming Mode...");
    enterProgramMode = 0; // after reboot, come back up in BLE mode.
    blinkRate = 0.05;

    WiFi.softAP(SSID, PASSWD);
    delay(100);
  
    Serial.println("Set softAPConfig");
    
    IPAddress Ip(192, 168, 1, 1);
    IPAddress NMask(255, 255, 255, 0);
    WiFi.softAPConfig(Ip, Ip, NMask);
  
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
      ESP.restart();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });
    ArduinoOTA.begin();

    myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
}


void setup() {
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(MUTE_PIN, OUTPUT);
  digitalWrite(MUTE_PIN, LOW);
  
  Serial.begin(115200);
  Serial.println("");
  Serial.printf("Available Heap: %d\n",ESP.getFreeHeap());

  // Determine if we're expected to enter BLE or WIFI Mode
  mode = (enterProgramMode == PROGRAMMING_MODE_MAGIC)?WIFI_MODE:BLE_MODE;

  // Initialized based on the selected mode
  if (mode == BLE_MODE) {
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("");
    state = PRE_INIT;
    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    // BLEScan* pBLEScan = BLEDevice::getScan();
    // pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    // pBLEScan->setInterval(1349);
    // pBLEScan->setWindow(449);
    // pBLEScan->setActiveScan(true);
    // pBLEScan->start(1, false);
  }  else if (mode == WIFI_MODE) {
    startWifi();
  }
} // End of setup.

// This is the Arduino main loop function for the BLE application
void ble_loop() {

  // If the state is "CONNECTING" is then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  //Serial.printf("|%d|", state);
  switch (state) {
  	case CONNECTING:
    	if (connectToServer()) {
      		Serial.println("We are now connected to the BLE Server.");
	     	state = CONNECTED;
	     	blinkRate = INFINITY; // Turn on LED
	     	digitalWrite(LED_BUILTIN, true);
    	} else {
      		Serial.println("We have failed to connect to the server; there is nothin more we will do.");
      		state = PRE_INIT;
    	}
  		break;

  		
  		case CONNECTED:
  			//Print heartbeat
        blinkRate = NAN; // do not drive the LED from the binker
    		break;

    	case PRE_INIT:
			 blinkRate = NAN;
       state = SCANNING;
       //intentional fall through

      case SCANNING: {
		    BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
        pBLEScan->setInterval(1349);
        pBLEScan->setWindow(449);
        pBLEScan->setActiveScan(true);
        pBLEScan->start(1, false);
        toggleLED();
    		break;
        }

    	default:
    		//No-op
    		break;
   }
  
   //delay(1000); // Delay a second between loops.
} // End of loop


// Thei is the Arduino main loop for the OTA programing mode
void wifi_loop() {
  ArduinoOTA.handle();
}

void loop() {
  if (blinkRate == NAN) {
    // noop
  } else if (blinkRate == INFINITY) {
    digitalWrite(LED_BUILTIN, true);
  } else if (blinkRate == 0.0) {
    digitalWrite(LED_BUILTIN, false);
  } else if (blinkRate > 0.0) {
    unsigned long now = millis();
    if (now > nextBlinkTime) {
      toggleLED();
      nextBlinkTime = now + (1000.0 * blinkRate);
    }
  }

  if (mode == BLE_MODE) {
    ble_loop();
  } else if (mode == WIFI_MODE) {
    wifi_loop();
  }
}
