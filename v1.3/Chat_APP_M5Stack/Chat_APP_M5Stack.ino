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
#include <EEPROM.h>
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
#define TFTW            320     // screen width
#define TFTH            240     // screen height
#define TFTW2           160     // half screen width
#define TFTH2           120     // half screen height
// game constant
#define SPEED             1
#define GRAVITY         9.8
#define JUMP_FORCE     2.15
#define SKIP_TICKS     20.0     // 1000 / 50fps
#define MAX_FRAMESKIP     5
// bird size
#define BIRDW             16     // bird width
#define BIRDH             16     // bird height
#define BIRDW2            8     // half width
#define BIRDH2            8     // half height
// pipe size
#define PIPEW            24     // pipe width
#define GAPHEIGHT        42     // pipe gap height
// floor size
#define FLOORH           30     // floor height (from bottom of the screen)
// grass size
#define GRASSH            4     // grass height (inside floor, starts at floor y)

int maxScore = 0;
const int buttonPin = 2;     
// background
const unsigned int BCKGRDCOL = M5.Lcd.color565(138,235,244);
// bird
const unsigned int BIRDCOL = M5.Lcd.color565(255,254,174);
// pipe
const unsigned int PIPECOL  = M5.Lcd.color565(99,255,78);
// pipe highlight
const unsigned int PIPEHIGHCOL  = M5.Lcd.color565(250,255,250);
// pipe seam
const unsigned int PIPESEAMCOL  = M5.Lcd.color565(0,0,0);
// floor
const unsigned int FLOORCOL = M5.Lcd.color565(246,240,163);
// grass (col2 is the stripe color)
const unsigned int GRASSCOL  = M5.Lcd.color565(141,225,87);
const unsigned int GRASSCOL2 = M5.Lcd.color565(156,239,88);

// bird sprite
// bird sprite colors (Cx name for values to keep the array readable)
#define C0 BCKGRDCOL
#define C1 M5.Lcd.color565(195,165,75)
#define C2 BIRDCOL
#define C3 TFT_WHITE
#define C4 TFT_RED
#define C5 M5.Lcd.color565(251,216,114)

static unsigned int birdcol[] =
{ C0, C0, C1, C1, C1, C1, C1, C0, C0, C0, C1, C1, C1, C1, C1, C0,
  C0, C1, C2, C2, C2, C1, C3, C1, C0, C1, C2, C2, C2, C1, C3, C1,
  C0, C2, C2, C2, C2, C1, C3, C1, C0, C2, C2, C2, C2, C1, C3, C1,
  C1, C1, C1, C2, C2, C3, C1, C1, C1, C1, C1, C2, C2, C3, C1, C1,
  C1, C2, C2, C2, C2, C2, C4, C4, C1, C2, C2, C2, C2, C2, C4, C4,
  C1, C2, C2, C2, C1, C5, C4, C0, C1, C2, C2, C2, C1, C5, C4, C0,
  C0, C1, C2, C1, C5, C5, C5, C0, C0, C1, C2, C1, C5, C5, C5, C0,
  C0, C0, C1, C5, C5, C5, C0, C0, C0, C0, C1, C5, C5, C5, C0, C0};

// bird structure
static struct BIRD {
  long x, y, old_y;
  long col;
  float vel_y;
} bird;

// pipe structure
static struct PIPES {
  long x, gap_y;
  long col;
} pipes;

// score
int score;
// temporary x and y var
static short tmpx, tmpy;

// ---------------
// draw pixel
// ---------------
// faster drawPixel method by inlining calls and using setAddrWindow and pushColor
// using macro to force inlining
#define drawPixel(a, b, c) M5.Lcd.setAddrWindow(a, b, a, b); M5.Lcd.pushColor(c)
// codes
std::vector<String> msgList;
bool inchat = false;


// cmc
void game_loop() {
  // ===============
  // prepare game variables
  // draw floor
  // ===============
  // instead of calculating the distance of the floor from the screen height each time store it in a variable
  unsigned char GAMEH = TFTH - FLOORH;
  // draw the floor once, we will not overwrite on this area in-game
  // black line
  M5.Lcd.drawFastHLine(0, GAMEH, TFTW, TFT_BLACK);
  // grass and stripe
  M5.Lcd.fillRect(0, GAMEH+1, TFTW2, GRASSH, GRASSCOL);
  M5.Lcd.fillRect(TFTW2, GAMEH+1, TFTW2, GRASSH, GRASSCOL2);
  // black line
  M5.Lcd.drawFastHLine(0, GAMEH+GRASSH, TFTW, TFT_BLACK);
  // mud
  M5.Lcd.fillRect(0, GAMEH+GRASSH+1, TFTW, FLOORH-GRASSH, FLOORCOL);
  // grass x position (for stripe animation)
  long grassx = TFTW;
  // game loop time variables
  double delta, old_time, next_game_tick, current_time;
  next_game_tick = current_time = millis();
  int loops;
  // passed pipe flag to count score
  bool passed_pipe = false;
  // temp var for setAddrWindow
  unsigned char px;

  while (1) {
    loops = 0;
    while( millis() > next_game_tick && loops < MAX_FRAMESKIP) {
      // ===============
      // input
      // ===============
      if (M5.BtnB.wasPressed()) {
        // if the bird is not too close to the top of the screen apply jump force
        if (bird.y > BIRDH2*0.5) bird.vel_y = -JUMP_FORCE;
        // else zero velocity
        else bird.vel_y = 0;
      }
      M5.update();
      
      // ===============
      // update
      // ===============
      // calculate delta time
      // ---------------
      old_time = current_time;
      current_time = millis();
      delta = (current_time-old_time)/1000;

      // bird
      // ---------------
      bird.vel_y += GRAVITY * delta;
      bird.y += bird.vel_y;

      // pipe
      // ---------------
      pipes.x -= SPEED;
      // if pipe reached edge of the screen reset its position and gap
      if (pipes.x < -PIPEW) {
        pipes.x = TFTW;
        pipes.gap_y = random(10, GAMEH-(10+GAPHEIGHT));
      }

      // ---------------
      next_game_tick += SKIP_TICKS;
      loops++;
    }

    // ===============
    // draw
    // ===============
    // pipe
    // ---------------
    // we save cycles if we avoid drawing the pipe when outside the screen
    if (pipes.x >= 0 && pipes.x < TFTW) {
      // pipe color
      M5.Lcd.drawFastVLine(pipes.x+3, 0, pipes.gap_y, PIPECOL);
      M5.Lcd.drawFastVLine(pipes.x+3, pipes.gap_y+GAPHEIGHT+1, GAMEH-(pipes.gap_y+GAPHEIGHT+1), PIPECOL);
      // highlight
      M5.Lcd.drawFastVLine(pipes.x, 0, pipes.gap_y, PIPEHIGHCOL);
      M5.Lcd.drawFastVLine(pipes.x, pipes.gap_y+GAPHEIGHT+1, GAMEH-(pipes.gap_y+GAPHEIGHT+1), PIPEHIGHCOL);
      // bottom and top border of pipe
      drawPixel(pipes.x, pipes.gap_y, PIPESEAMCOL);
      drawPixel(pipes.x, pipes.gap_y+GAPHEIGHT, PIPESEAMCOL);
      // pipe seam
      drawPixel(pipes.x, pipes.gap_y-6, PIPESEAMCOL);
      drawPixel(pipes.x, pipes.gap_y+GAPHEIGHT+6, PIPESEAMCOL);
      drawPixel(pipes.x+3, pipes.gap_y-6, PIPESEAMCOL);
      drawPixel(pipes.x+3, pipes.gap_y+GAPHEIGHT+6, PIPESEAMCOL);
    }
    // erase behind pipe
    if (pipes.x <= TFTW) M5.Lcd.drawFastVLine(pipes.x+PIPEW, 0, GAMEH, BCKGRDCOL);

    // bird
    // ---------------
    tmpx = BIRDW-1;
    do {
          px = bird.x+tmpx+BIRDW;
          // clear bird at previous position stored in old_y
          // we can't just erase the pixels before and after current position
          // because of the non-linear bird movement (it would leave 'dirty' pixels)
          tmpy = BIRDH - 1;
          do {
            drawPixel(px, bird.old_y + tmpy, BCKGRDCOL);
          } while (tmpy--);
          // draw bird sprite at new position
          tmpy = BIRDH - 1;
          do {
            drawPixel(px, bird.y + tmpy, birdcol[tmpx + (tmpy * BIRDW)]);
          } while (tmpy--);
    } while (tmpx--);
    // save position to erase bird on next draw
    bird.old_y = bird.y;

    // grass stripes
    // ---------------
    grassx -= SPEED;
    if (grassx < 0) grassx = TFTW;
    M5.Lcd.drawFastVLine( grassx    %TFTW, GAMEH+1, GRASSH-1, GRASSCOL);
    M5.Lcd.drawFastVLine((grassx+64)%TFTW, GAMEH+1, GRASSH-1, GRASSCOL2);

    // ===============
    // collision
    // ===============
    // if the bird hit the ground game over
    if (bird.y > GAMEH-BIRDH) break;
    // checking for bird collision with pipe
    if (bird.x+BIRDW >= pipes.x-BIRDW2 && bird.x <= pipes.x+PIPEW-BIRDW) {
      // bird entered a pipe, check for collision
      if (bird.y < pipes.gap_y || bird.y+BIRDH > pipes.gap_y+GAPHEIGHT) break;
      else passed_pipe = true;
    }
    // if bird has passed the pipe increase score
    else if (bird.x > pipes.x+PIPEW-BIRDW && passed_pipe) {
      passed_pipe = false;
      // erase score with background color
      M5.Lcd.setTextColor(BCKGRDCOL);
      M5.Lcd.setCursor( TFTW2, 4);
      M5.Lcd.print(score);
      // set text color back to white for new score
      M5.Lcd.setTextColor(TFT_WHITE);
      // increase score since we successfully passed a pipe
      score++;
    }

    // update score
    // ---------------
    M5.Lcd.setCursor( TFTW2, 4);
    M5.Lcd.print(score);
  }
  
  // add a small delay to show how the player lost
  delay(1200);
}


// ---------------
// game start
// ---------------
void game_start() {
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.fillRect(10, TFTH2 - 20, TFTW-20, 1, TFT_WHITE);
  M5.Lcd.fillRect(10, TFTH2 + 32, TFTW-20, 1, TFT_WHITE);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(3);
  // half width - num char * char width in pixels
  M5.Lcd.setCursor( TFTW2-(6*9), TFTH2 - 16);
  M5.Lcd.println("FLAPPY");
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor( TFTW2-(6*9), TFTH2 + 8);
  M5.Lcd.println("-BIRD-");
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor( 10, TFTH2 - 36);
  M5.Lcd.println("M5Stack");
  M5.Lcd.setCursor( TFTW2 - (17*9), TFTH2 + 36);
  M5.Lcd.println("Premi il bottone centrale");
  while (1) {
    // wait for push button
      if(M5.BtnB.wasPressed()) {
        break;
      }
    M5.update();
        
    }
      // init game settings
      game_init();
}

void game_init() {
  // clear screen
  M5.Lcd.fillScreen(BCKGRDCOL);
  // reset score
  score = 0;
  // init bird
  bird.x = 144;
  bird.y = bird.old_y = TFTH2 - BIRDH;
  bird.vel_y = -JUMP_FORCE;
  tmpx = tmpy = 0;
  // generate new random seed for the pipe gape
  randomSeed(analogRead(0));
  // init pipe
  pipes.x = 0;
  pipes.gap_y = random(20, TFTH-60);
}


// ---------------
// game over
// ---------------
void game_over() {
  M5.Lcd.fillScreen(TFT_BLACK);
  EEPROM_Read(&maxScore,0);
  
  if(score>maxScore)
  {
    EEPROM_Write(&score,0);
    maxScore = score;
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.setTextSize(2); 
    M5.Lcd.setCursor( TFTW2 - (13*6), TFTH2 - 26);
    M5.Lcd.println("NEW HIGHSCORE");
  }
  
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(3);
  // half width - num char * char width in pixels
  M5.Lcd.setCursor( TFTW2 - (9*9), TFTH2 - 6);
  M5.Lcd.println("GAME OVER");
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor( 10, 10);
  M5.Lcd.print("score: ");
  M5.Lcd.print(score);
  M5.Lcd.setCursor( TFTW2 - (12*6), TFTH2 + 18);
  M5.Lcd.println("press buttons");
  M5.Lcd.setCursor( 10, 28);
  M5.Lcd.print("Max Score:");
  M5.Lcd.print(maxScore);
  while (1) {
    // wait for push button
      if(M5.BtnB.wasPressed()) {
        break;
      }
    M5.update();
  }
}

void resetMaxScore()
{
  EEPROM_Write(&maxScore,0);
}



void EEPROM_Write(int *num, int MemPos)
{
 byte ByteArray[2];
 memcpy(ByteArray, num, 2);
 for(int x = 0; x < 2; x++)
 {
   EEPROM.write((MemPos * 2) + x, ByteArray[x]);
 }  
}



void EEPROM_Read(int *num, int MemPos)
{
 byte ByteArray[2];
 for(int x = 0; x < 2; x++)
 {
   ByteArray[x] = EEPROM.read((MemPos * 2) + x);    
 }
 memcpy(num, ByteArray, 2);
}

// on message received
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("INFO I Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  Serial.println("INFO I getcode:");
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
  Serial.println("BOOT starting...");  
  Serial.println("TASK starting...");  
  Serial.println("TASK started console...");
  Serial.println("TASK start serial 115200 console...done");
  Serial.println("BOOT ----boot item is bootloader at 0x1000----");
  Serial.println("BOOT init done");
  #include "default.h"
  #include "dark.h"
  #include "deep.h"
  ez.begin();
  SPIFFS.begin();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  ezMenu mainmenu("MChat+ 2.0");
  mainmenu.addItem("chat", chat_menu);
  mainmenu.addItem("option", mainmenu_image);
  mainmenu.addItem("games", game_menu);
  mainmenu.addItem("settings", ez.settings.menu);
  mainmenu.addItem("update", ota_menu);
  mainmenu.run();
}

void flappy_start() {
  game_start();
  game_loop();
  game_over();
}

void game_menu() {
  ezMenu gamemenu("Games");
  gamemenu.addItem("Flappybird", flappy_start);
  gamemenu.addItem("Back");
}

void ota_menu() {
  ezMenu otamenu("OTA options");
  otamenu.addItem("via stable channel", mainmenu_ota);
  otamenu.addItem("via dev-rolling channel", mainmenu_ota_dev);
  otamenu.addItem("current version", version_check);
  otamenu.addItem("Back");
  otamenu.run();
}

void version_check() {
  ez.msgBox("version: v2.0", "latest version: v2.0");
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
  Serial.println("INFO I chat page");
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
    Serial.println("INFO I menuloaded");
    return;
  }
  inchat = true;
  
  ez.buttons.show("up # Back # input # # down #");
  redraw();
  
  while(true) {
    if(!keepMqttConn()) {
      Serial.println("ERROR code:");
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
  if (ez.msgBox("Update confirm", "update software via stable channel with internet now!", "Cancel#OK#") == "OK") {
    ezProgressBar progress_bar("updating...", "Downloading ...", "Abort");
    #include "downloadsftp_us_aldryn_io.h"              // the root certificate is now in const char * root_cert
    if (ez.wifi.update("https://downloadsftp.us.aldryn.io/files/mcupkg/esp32/chatapp/1.3/main.bin", root_cert, &progress_bar)) {                // second address:https://gitee.com/sysdl132/Chat_APP_M5Stack/raw/master/1.bin
      ez.msgBox("success", "Update successful. Reboot to new firmware", "Reboot");
      ESP.restart();
    } else {
      ez.msgBox("OTA error", ez.wifi.updateError(), "OK");
    }
  }
};

void mainmenu_ota_dev() {
  if (ez.msgBox("Update confirm", "update software via dev channel with internet now!", "Cancel#OK#") == "OK") {
    ezProgressBar progress_bar("updating...", "Downloading ...", "Abort");
    //#include "gitee_com.h"    // the root certificate is now in const char * root_cert
    //#include "github-production-release-asset-2e65be_s3_amazonaws_com.h"
    #include "downloadsftp_us_aldryn_io.h"
    if (ez.wifi.update("https://downloadsftp.us.aldryn.io/files/mcupkg/esp32/chatapp/1.3/dev.bin", root_cert, &progress_bar)) {                // second address:https://gitee.com/sysdl132/Chat_APP_M5Stack/raw/master/2.bin  ,same website has the same certificate. 
      ez.msgBox("success", "Update successful. Reboot to new firmware", "Reboot");
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
