/*
 * CHAT APP FOR M5STACK
 * This program provides a space to chat.
 * And based on tanopanta's base program.
 * INCLUDE FEATURES:
 * - OTA update
 * - system imfo
 * - backlight
 * - theme chooser
 * --THIS PROGRAM IS UNDER MIT LICENSE--
 * =======================================
 * TODO LIST:
 *    roll version
 *    China mirrors
 */

// includes
#include <M5Stack.h>
#include <SPIFFS.h>
#include <M5ez.h>
#include <vector>
#include "images.h"
#include <PubSubClient.h>

// names
WiFiClient espClient;
PubSubClient client(espClient);
// username
//const char* user_name = "M5FIRE";  // will set your username|Now is disabled
// config 
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
//const int latest_version = 1.3.3;

// codes
std::vector<String> msgList;
bool inchat = false;

// on message received
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  Serial.println("getcode:");
  Serial.println("200");
  String msg = String((char*) payload);
  Serial.println(msg);
  msgList.push_back(msg);
  redraw();  
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  // Serial.begin(921600);
  Serial.println("STARTING BOARD..............................................................................done");  
  Serial.println("pre-booting...... done");  
  Serial.println("starting console.............................................................................................................................done");
  Serial.print(".");Serial.print("loading...login as root...logged on");  
  Serial.println("start serial 115200 console...done");
  Serial.println("boot item is bootloader at 0x1000----------------------------------------------------------------------------------------------booting---success.");
  Serial.println("enter system...init...  Done.");
  Serial.print("ALL DONE- 10 -");Serial.print(".");Serial.print(".");Serial.print(".");Serial.print(".");Serial.print(".");Serial.print(".");Serial.print(".");Serial.print(".");Serial.print(".");Serial.print(".");
  #include "default.h"
  #include "dark.h"
  #include "deep.h"
  ez.begin();
  SPIFFS.begin();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  ezMenu mainmenu("Welcome to M5ez/v1.3.3");
  mainmenu.addItem("chat", chat_menu);
  mainmenu.addItem("option", mainmenu_image);
  mainmenu.addItem("settings", ez.settings.menu);
  mainmenu.addItem("update", ota_menu);
  mainmenu.run();
}

void ota_menu() {
  ezMenu otamenu("OTA options");
  otamenu.addItem("via stable channel", mainmenu_ota);
  otamenu.addItem("via dev-rolling channel", mainmenu_ota_dev);
  otamenu.addItem("current version",version_check);
  otamenu.run();
}

void version_check() {
  ez.msgBox("version:v1.3.3", "latest version:v1.3.3");
}

void mainmenu_image() {
  ezMenu images;
  images.imgBackground(TFT_BLACK);
  images.imgFromTop(40);
  images.imgCaptionColor(TFT_WHITE);
  images.addItem(wifi_jpg, "WiFi Settings", ez.wifi.menu);
  images.addItem(about_jpg, "About M5ez", aboutM5ez);
  images.addItem(sysinfo_jpg, "System Information", sysInfo);
  images.addItem(sleep_jpg, "Power Off", powerOff);
  images.addItem(return_jpg, "Back");
  images.run();
}
// true: connected,  false: not connected
bool keepMqttConn() {
    if (!client.connected()) {
        Serial.print("Attempting MQTT connection...");

        String clientId = getMacAddr(); 
        if (client.connect(clientId.c_str())) {
          Serial.print("Connecting done");
          // Once connected, publish an announcement...
          client.publish("outTopic", "ONLINE");
          client.subscribe("chat");
          return true;
        } else {
          return false;
        }
    }
    return true;
}

// draw chat messages on LCD
void redraw() {
  if(!inchat) {
    return;
  }
  ez.canvas.clear();
  ez.header.show("chat");
  ez.canvas.font(&FreeSans9pt7b);
  ez.canvas.lmargin(10);
  Serial.println("INPAGE  chat");
  // show latest 8 messages
  int total = 0;
  for(int i = msgList.size() - 1 ; i >= 0; i--) {
    if(total >= 8) {
      break;
    }
    ez.canvas.println(msgList[i]);
    total++;
  }
}

// chat main
void chat_menu() {
  if(WiFi.status() != WL_CONNECTED) {
    ez.msgBox("notice", "Wi-Fi is not enabled. Please setting Wi-Fi.");
    Serial.println("menuloaded");
    return;
  }
  inchat = true;
  
  ez.buttons.show("up # Back # input # # down #");
  redraw();
  
  while(true) {
    if(!keepMqttConn()) {
      Serial.println("ERROR.code:");
      ez.msgBox("notice", "MQTT server not found.");
      Serial.println("404");
      return;
    }
    client.loop();
    
    String btnpressed = ez.buttons.poll(); // check button status
    if(btnpressed == "Back") {
      break;
    }else if(btnpressed == "input") {
      ez.buttons.clear(true);
      ez.canvas.clear();
      
      String msg = ez.textInput();
      msg.trim();

      // send message
      if(msg.length() > 0) {
        //msgList.push_back(msg);
        keepMqttConn();
        //client.publish("chat", user_name);
        client.publish("chat", msg.c_str());
      }
      
      redraw();
      ez.buttons.clear(true);
      ez.buttons.show("up # Back # input # # down #");
    }else if (btnpressed != "") {

    }
  }
  inchat = false;
}

// get MAC address of device
String getMacAddr() {
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char baseMacChr[16] = {0};
  sprintf(baseMacChr, "%02X%02X%02X%02X%02X%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  return String(baseMacChr);
}

void sysInfo() {
  sysInfoPage1();
  while(true) {
    String btn = ez.buttons.poll();
    if (btn == "up") sysInfoPage1();
    if (btn == "down") sysInfoPage2();
    if (btn == "Exit") break;
  }
}


void sysInfoPage1() {
  const byte tab = 120;
  ez.screen.clear();
  ez.header.show("System Info  (1/2)");
  ez.buttons.show("# Exit # down");
  ez.canvas.font(&FreeSans9pt7b);
  ez.canvas.lmargin(10);
  ez.canvas.println("");
  ez.canvas.print("CPU freq:");  ez.canvas.x(tab); ez.canvas.println(String(ESP.getCpuFreqMHz()) + " MHz");
  ez.canvas.print("CPU cores:");  ez.canvas.x(tab); ez.canvas.println("2");    //   :)
  ez.canvas.print("Chip rev.:");  ez.canvas.x(tab); ez.canvas.println(String(ESP.getChipRevision()));
  ez.canvas.print("Flash speed:");  ez.canvas.x(tab); ez.canvas.println(String(ESP.getFlashChipSpeed() / 1000000) + " MHz");
  ez.canvas.print("Flash size:");  ez.canvas.x(tab); ez.canvas.println(String(ESP.getFlashChipSize() / 1000000) + " MB");
  ez.canvas.print("ESP SDK:");  ez.canvas.x(tab); ez.canvas.println(String(ESP.getSdkVersion()));
  ez.canvas.print("M5ez:");  ez.canvas.x(tab); ez.canvas.println(String(ez.version()));
}

void sysInfoPage2() {
  const byte tab = 140;
  ez.screen.clear();
  ez.header.show("System Info  (2/2)");
  ez.buttons.show("up # Exit #");
  ez.canvas.font(&FreeSans9pt7b);
  ez.canvas.lmargin(10);
  ez.canvas.println("");
  ez.canvas.print("Free RAM:");  ez.canvas.x(tab);  ez.canvas.println(String((long)ESP.getFreeHeap()) + " bytes");
  ez.canvas.print("Min. free seen:");  ez.canvas.x(tab); ez.canvas.println(String((long)esp_get_minimum_free_heap_size()) + " bytes");
  ez.canvas.print("SPIFFS size:"); ez.canvas.x(tab); ez.canvas.println(String((long)SPIFFS.totalBytes()) + " bytes");
  ez.canvas.print("SPIFFS used:"); ez.canvas.x(tab); ez.canvas.println(String((long)SPIFFS.usedBytes()) + " bytes");
  
}

void mainmenu_ota() {
  if (ez.msgBox("V1.3.3-LTS/OTA update", "update software via stable channel with internet now!", "Cancel#OK#") == "OK") {
    ezProgressBar progress_bar("OTA update in progress", "Downloading ...", "Abort");
    #include "github-production-release-asset-2e65be_s3_amazonaws_com.h"              // the root certificate is now in const char * root_cert
    if (ez.wifi.update("https://github-production-release-asset-2e65be.s3.amazonaws.com/238684227/68cddf00-beb2-11ea-97a3-12166123b94f?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAIWNJYAX4CSVEH53A%2F20200705%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20200705T035602Z&X-Amz-Expires=300&X-Amz-Signature=cce1f7e74cdc3a9513fd5c6f675b9620cac9fa58929d7e2d0363a8510fe422a5&X-Amz-SignedHeaders=host&actor_id=54708046&repo_id=238684227&response-content-disposition=attachment%3B%20filename%3Dmain.bin&response-content-type=application%2Foctet-stream", root_cert, &progress_bar)) {                // second address:https://gitee.com/sysdl132/Chat_APP_M5Stack/raw/master/1.bin
      ez.msgBox("OTA update success", "OTA download successful. Reboot to new firmware", "Reboot");
      ESP.restart();
    } else {
      ez.msgBox("OTA error", ez.wifi.updateError(), "OK");
    }
  }
};

void mainmenu_ota_dev() {
  if (ez.msgBox("V1.3.3-LTS/OTA update", "update software via dev channel with internet now!", "Cancel#OK#") == "OK") {
    ezProgressBar progress_bar("OTA update in progress", "Downloading ...", "Abort");
    //#include "gitee_com.h"    // the root certificate is now in const char * root_cert
    #include "github-production-release-asset-2e65be_s3_amazonaws_com.h"
    if (ez.wifi.update("https://github-production-release-asset-2e65be.s3.amazonaws.com/238684227/68cddf00-beb2-11ea-97a3-12166123b94f?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAIWNJYAX4CSVEH53A%2F20200705%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20200705T035602Z&X-Amz-Expires=300&X-Amz-Signature=cce1f7e74cdc3a9513fd5c6f675b9620cac9fa58929d7e2d0363a8510fe422a5&X-Amz-SignedHeaders=host&actor_id=54708046&repo_id=238684227&response-content-disposition=attachment%3B%20filename%3Ddev.bin&response-content-type=application%2Foctet-stream", root_cert, &progress_bar)) {                // second address:https://gitee.com/sysdl132/Chat_APP_M5Stack/raw/master/2.bin  ,same website has the same certificate. 
      ez.msgBox("OTA update success", "OTA download successful. Reboot to new firmware", "Reboot");
      ESP.restart();
    } else {
      ez.msgBox("OTA error", ez.wifi.updateError(), "OK");
    }
  }
};

void powerOff() { 
  m5.powerOFF(); 
  }

void aboutM5ez() {
  ez.msgBox("About M5ez", "Rop Gonggrijp write|sysdl132 write chat");
}
