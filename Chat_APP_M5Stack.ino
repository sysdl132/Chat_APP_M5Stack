// DEPRECATED
/*#include <M5Stack.h>
#include <SPIFFS.h>
#include <M5ez.h>
#include <vector>
#include "images.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////#include "sysinfo.h"
#include <PubSubClient.h>


WiFiClient espClient;
PubSubClient client(espClient);



const char* mqtt_server = "test.mosquitto.org";
//const char* mqtt_server = "postman.cloudmqtt.com";
//const int mqtt_port = 18281;
const int mqtt_port = 1883;


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
  Serial.begin(115200);
  #include <themes/default.h>
  #include <themes/dark.h>
  // #include <themes/deep.h> //note:This is my customsize theme.If you have,uncomment this line.If you want,you can make an issue and I will post it.
  ez.begin();
  SPIFFS.begin();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  ezMenu mainmenu("Welcome to M5ez ");
  mainmenu.addItem("chat", chat_menu);
  mainmenu.addItem("option", mainmenu_image);
  mainmenu.addItem("settings", ez.settings.menu);
  mainmenu.addItem("update", mainmenu_ota);
  mainmenu.run();
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
  if (ez.msgBox("Get latest bin", "update chat with internet now!Current version:V1.3", "Cancel#OK#") == "OK") {
    Serial.println("updating..........");
    ezProgressBar progress_bar("OTA update in progress", "Downloading ...", "Abort");
    #include "rootcert.h" // the root certificate is now in const char * root_cert
    if (ez.wifi.update("https://github-production-release-asset-2e65be.s3.amazonaws.com/238684227/d29d0c00-4e71-11ea-9dd2-132f7009b8c2?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAIWNJYAX4CSVEH53A%2F20200214%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20200214T030542Z&X-Amz-Expires=300&X-Amz-Signature=f68651e385b9384c5590b74b0b2ae7ea587dab7a1be6c2f027dc3b9a56c810b1&X-Amz-SignedHeaders=host&actor_id=54708046&response-content-disposition=attachment%3B%20filename%3Dbinary.bin&response-content-type=application%2Foctet-stream", root_cert, &progress_bar)) {
      ez.msgBox("Over The Air updater", "OTA download successful. Reboot to new firmware", "Reboot");
      Serial.println("success");
      ESP.restart();
    } else {
      ez.msgBox("OTA error", ez.wifi.updateError(), "OK");
    }
  }
}

void powerOff() { m5.powerOFF(); }

void aboutM5ez() {
  ez.msgBox("About M5ez", "Rop Gonggrijp write|sysdl132 write chat");
}
*/
