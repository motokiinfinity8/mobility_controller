#include <NeoPixelBus.h>
//#include "BluetoothSerial.h"
#include <WiFi.h>
#include <WiFiUdp.h>

const char ssid[] = "ESP32_MOBILITY"; // SSID
const char password[] = "esp32_con";  // password
const int WifiPort = 8000;      // ポート番号

const IPAddress HostIP(192, 168, 11, 1);       // IPアドレス
const IPAddress ClientIP(192, 168, 11, 2);       // IPアドレス
const IPAddress subnet(255, 255, 255, 0); // サブネットマスク
const IPAddress gateway(192,168, 11, 0);
const IPAddress dns(192, 168, 11, 0);

WiFiUDP Udp;

#define UP_STICK_PIN   33
#define LEFT_STICK_PIN   35 
#define RIGHT_STICK_PIN   32 
#define DOWN_STICK_PIN   34 
#define PIXEL_PIN    13

#define PIXEL_COUNT 9

#define CMD_SIZE  12

// Parameter 1 = number of pixels in strip,  neopixel stick has 8
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream, correct for neopixel stick
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip), correct for neopixel stick
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800); 
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PIXEL_COUNT, PIXEL_PIN);
//BluetoothSerial bt;
//const char *bt_name = "led_controller"; // Bluetoothのデバイス名



//GLOBAL変数
int g_stick_pos = 0;    // 0: センター、4: 上、-4：下 に加えて、[+1:右、-1:左]を補正
int g_stick_pos_now = 15;
bool g_stick_pos_is_changed = false;
int wave_dir;
int start_led, end_led;
uint32_t g_start_time = 0;


void setup()
{
  Serial.begin(115200);

  pinMode(PIXEL_PIN, OUTPUT);
  pinMode(UP_STICK_PIN, INPUT_PULLUP);
  pinMode(LEFT_STICK_PIN, INPUT_PULLUP);
  pinMode(RIGHT_STICK_PIN, INPUT_PULLUP);
  pinMode(DOWN_STICK_PIN, INPUT_PULLUP);
  strip.Begin();
  strip.Show();

  rainbow(10, 0, PIXEL_COUNT-1) ;
  //rainbowCycle(2, 0, 8);

  TaskHandle_t th; //マルチタスクハンドル定義
  xTaskCreatePinnedToCore(DisplayLED, "DisplayLED", 4096, NULL, 10, &th, 0); //マルチタスク起動
  //xTaskCreate(DisplayLED, "DisplayLED", 4096, NULL, 10, NULL); //マルチタスク起動

  //Serial.print("setup");
  //bt.begin(bt_name);


  // Wifi設定 (SSID&パス設定)
  //WiFi.mode(WIFI_STA);
  //WiFi.config(ClientIP, WiFi.gatewayIP(), WiFi.subnetMask());  
  WiFi.config(ClientIP, gateway, subnet, dns);
  WiFi.begin(ssid, password);
  Udp.begin(WifiPort);
  //wifi_rcv = 0;
     
  Serial.println("start_connect");
  int i=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(i >= 10){
      break;
    }
    i++;
  }
  
  if( i >= 10){
    Serial.println("Wifi Disconnected...");
  }else{
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}


/////////////////////////////
// LED表示タスク処理
/////////////////////////////
void DisplayLED(void *pvParameters) {
    while(1){
      // もし5秒以上スティックが倒れていたら、フィーバーモード（レインボー)に遷移  
      if(Time_Mesure(g_start_time) > 5000){
        rainbowCycle(2, 0, PIXEL_COUNT-1);
      }else{
        // 5秒以下の場合、スティックの倒れ方に応じてモード変更
        switch(g_stick_pos){
          case  -5: //右後ろ
            RunningLights(255, 0,  0, 80,  2, 8,  0); 
            break;
          case -4:  //後ろ
            RunningLightsToCenter(255, 0,  0, 80,  0, 8,  0); 
            break;
          case  -3: //左後ろ
            RunningLights(255, 0,  0, 80,  0, 6,  1); 
            break;
          case  -1: //右
            RunningLights(0, 255, 255, 80,  2, 8,  0); 
            break;
          case  1:  //左
            RunningLights(0, 255, 0, 80,  0, 6,  1); 
            break;
          case  3:  //右前
            RunningLights(255, 255,  0, 80,  2, 8,  0); 
            break;
          case  4:  //前
            RunningLightsToCenter(255, 255,  0, 80,  0, 8,  1); 
            break;
          case  5:  //左前
            RunningLights(255, 255,  0, 80, 0, 6, 1);
            break;
          case  0:  //なし
           //portDISABLE_INTERRUPTS();
           for(int i=0; i<PIXEL_COUNT; i++) {
              strip.SetPixelColor(i, RgbColor(0, 0, 0));
              delay(1);  //安定待ち
           }
           //
           //delay(10);  //安定待ち
           delay(10);
           strip.Show();
           delay(10);
           //portENABLE_INTERRUPTS();
           break;
        }

      }
      g_stick_pos_is_changed = false;
    }
}


/////////////////////////////
// Main処理
/////////////////////////////
void loop(){
  
  // スティック検出
  if(digitalRead(UP_STICK_PIN)==1){
    g_stick_pos = 4;
    wave_dir = 1;
  }else if(digitalRead(DOWN_STICK_PIN)==1){
    g_stick_pos = -4;
    wave_dir = 0;
  }else{
    g_stick_pos = 0;
    wave_dir = 0;
  }

  if(digitalRead(LEFT_STICK_PIN)==1){
    g_stick_pos += 1;
  }else if(digitalRead(RIGHT_STICK_PIN)==1){
    g_stick_pos -= 1;
  }


  //スティックが倒されている間の時間測定
  if(g_stick_pos == 0)  g_start_time = millis();  //時間リセット
  Serial.println(Time_Mesure(g_start_time));
  Serial.print("g_stick_pos:");
  Serial.println(g_stick_pos);

  if(g_stick_pos != g_stick_pos_now){
     g_stick_pos_is_changed = true;
  }
  //bt.println(g_stick_pos);
  // UDP通信プロトコル (10byte固定)
  // STX(0x02)[1byte] + CMD[3byte] + ペイロード[5byte] + ETX(0x03)[1byte] 
  // 　①モビリティ制御方向
  //　　　[CMD]  MDC (Mobility Direction Control) 
  //      [VEL]       2Byte(ASCII) 前方方向速度  -9:後退 ～+0:停止～+9:前進
  //                  2Byte(ASCII) 旋回方向速度  -9:右旋回～+0:停止～+9:左旋回
  //                  1Byte(ASCII) 音声予告モード 0:予告モードあり、1:予告モードなし
  //      [Other] 予備2Byte (0埋め) ※末尾1バイトは0埋めになるため注意
  String  m_buf = "";
  //m_buf.concat(0x2);  // STX設定
  m_buf.concat("MDC");  // CMD命令設定

  //前方方向速度
  if(g_stick_pos >= 3) m_buf.concat("+9");
  else if(g_stick_pos >= -1) m_buf.concat("+0");
  else m_buf.concat("-9");

  //旋回方向速度
  if((g_stick_pos+8) % 4 == 1) m_buf.concat("+9");
  else if((g_stick_pos+8) % 4 == 0) m_buf.concat("+0");
  else m_buf.concat("-9");  
  
  m_buf.concat("0");  // 音声予告モード 
  //m_buf.concat(0x3);  // ETX設定 

  //byte [] wifi_buf = m_buf.getBytes();
  byte wifi_buf[CMD_SIZE-2];
  m_buf.getBytes(wifi_buf, CMD_SIZE-2);
  sendWiFi(wifi_buf);
  Serial.print(m_buf);
  
  g_stick_pos_now = g_stick_pos;
  delay(100);  //安定待ち

}


/////////////////////////////
// Wifi送信処理
/////////////////////////////
void sendWiFi(uint8_t byteData[]) {
//  void sendWiFi(String byteData[]){
  uint8_t stx = 0x2;
  uint8_t etx = 0x3;
  if (Udp.beginPacket(HostIP,  WifiPort)) {
    Udp.write(&stx,1); //STX送信  
    Udp.write(byteData,CMD_SIZE-2);
    Udp.write(&etx,1); //ETX送信
    Udp.endPacket();
    //Serial.println(byteData);
  }
}



uint32_t Time_Mesure( uint32_t st_time ){
  return millis() - st_time;
}

void RunningLights(uint8_t red, uint8_t green, uint8_t blue, int WaveDelay, int start_led, int end_led, int wave_dir) {
  int Position=100;
  
  set_led_out_of_range(start_led, end_led);
  
  for(int j=0; j<=(end_led-start_led)*2; j++)
  {
      if(wave_dir == 1)    Position++;
      else  Position--;
      
      for(int i=start_led; i<=end_led; i++) {
        // sine wave, 3 offset waves make a rainbow!

        //double level = (sin((float)(i+Position)) * 127 + 128);
        double level;
        // sinを使うと先頭LEDに緑ノイズが発生する対策 (sinを使わない）
        // 周期 3.5LED分
        switch((i+Position)%7){
          case 0: level=0.5; break;
          case 1: level=0.99; break;
          case 2: level=0.30; break;
          case 3: level=0.10; break;
          case 4: level=0.87; break;
          case 5: level=0.75; break;
          case 6: level=0.05; break;
        }
        //更に進行方向に応じて光量補正を行う (20%～100%)
        //if(wave_dir == 1) level = level * (0.2 + 0.8*(end_led - i)/(end_led - start_led)); 
        //else     level = level * (0.2 + 0.8 * (i-start_led)/(end_led - start_led));
        
        uint8_t tmp_r = pow(level,2)*red;
        uint8_t tmp_g = pow(level,2)*green;
        uint8_t tmp_b = pow(level,2)*blue;                
        //strip.SetPixelColor(i, RgbColor((uint8_t)(level*red), (uint8_t)(level*green), (uint8_t)(level*blue)));
        //strip.SetPixelColor(i,RgbColor(red, green, blue));
        //strip.SetPixelColor(i,RgbColor(red/2+10*(i+Position), green/2+10*(i+Position), blue/2+10*(i+Position)));
        strip.SetPixelColor(i,RgbColor(tmp_r, tmp_g, tmp_b));
        delay(2);
        //strip.SetPixelColor(i,
        //  RgbColor(((sin(i+Position) * 127 + 128)/255)*red,
        //              ((sin(i+Position) * 127 + 128)/255)*green,
        //              ((sin(i+Position) * 127 + 128)/255)*blue));
      }
      //portDISABLE_INTERRUPTS();
      delay(1);  //安定待ち
      strip.Show();
      //portENABLE_INTERRUPTS();
    
      if(g_stick_pos_is_changed == true) return;
      delay(WaveDelay);
  }
}

void RunningLightsToCenter(uint8_t red, uint8_t green, uint8_t blue, int WaveDelay, int start_led, int end_led, int wave_dir) {
  int Position=100;
  
  set_led_out_of_range(start_led, end_led);
  
  for(int j=0; j<=(end_led-start_led)*2; j++)
  {
      if(wave_dir == 1)    Position++;
      else  Position--;
             
      for(int i=start_led; i<=(end_led-start_led); i++) {
        // sine wave, 3 offset waves make a rainbow!
        //float level = sin(i+Position) * 127 + 128;
        //setPixel(i,level,0,0);
        //double level = (sin((float)(i+Position)) * 127 + 128);
        double level;
        // sinを使うと先頭LEDに緑ノイズが発生する対策 (sinを使わない）
        // 周期 3LED分
        //switch((i+Position)%3){
        //  case 0: level=0.5; break;
        //  case 1: level=0.95; break;
        //  case 2: level=0.10; break;
        // 周期 3.5LED分
        switch((i+Position)%7){
          case 0: level=0.5; break;
          case 1: level=0.99; break;
          case 2: level=0.30; break;
          case 3: level=0.10; break;
          case 4: level=0.87; break;
          case 5: level=0.75; break;
          case 6: level=0.05; break;        
        }
        uint8_t tmp_r = pow(level,2)*red;
        uint8_t tmp_g = pow(level,2)*green;
        uint8_t tmp_b = pow(level,2)*blue;                
        //strip.SetPixelColor(i, RgbColor((uint8_t)(level*red), (uint8_t)(level*green), (uint8_t)(level*blue)));
        //strip.SetPixelColor(i,RgbColor(red, green, blue));
        //strip.SetPixelColor(i,RgbColor(red/2+10*(i+Position), green/2+10*(i+Position), blue/2+10*(i+Position)));
        strip.SetPixelColor(i,RgbColor(tmp_r, tmp_g, tmp_b));
        strip.SetPixelColor(end_led-(i-start_led), RgbColor(tmp_r, tmp_g, tmp_b));
        delay(2);
        //strip.SetPixelColor(i,
        //  RgbColor(((sin(i+Position) * 127 + 128)/255)*red,
        //              ((sin(i+Position) * 127 + 128)/255)*green,
        //              ((sin(i+Position) * 127 + 128)/255)*blue));
      }
      strip.SetPixelColor((end_led-start_led)/2,RgbColor(red, green, blue));
      //portDISABLE_INTERRUPTS();
      delay(2);  //安定待ち
      strip.Show();
      //portENABLE_INTERRUPTS();

      if(g_stick_pos_is_changed == true) return;
      delay(WaveDelay);
  }
}



void theaterChase(uint8_t red, uint8_t green, uint8_t blue, int SpeedDelay, int start_led, int end_led) {

  set_led_out_of_range(start_led, end_led);
       
  for (int j=0; j < 5; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=start_led; i<= end_led-q; i=i+3) {
          strip.SetPixelColor(i+q,  RgbColor(red, green, blue));
        }
      //portDISABLE_INTERRUPTS();
      delay(2);  //安定待ち
      strip.Show();
      //portENABLE_INTERRUPTS();
        if(g_stick_pos_is_changed == true) return;
        delay(SpeedDelay);
       
        for (int i=start_led; i <=  end_led; i=i+3) {
          strip.SetPixelColor(i+q, RgbColor(0, 0, 0)); //turn every third pixel off 
        }
    }
  }
}

void theaterChaseRainbow(int SpeedDelay,  int start_led, int end_led) {
  byte *c;

  set_led_out_of_range(start_led, end_led);
       
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=start_led; i<= end_led-q; i=i+3) {
          strip.SetPixelColor(i+q,  Wheel( (i+j) % 255));
        }

      //portDISABLE_INTERRUPTS();
      delay(2);  //安定待ち
      strip.Show();
      //portENABLE_INTERRUPTS();

        if(g_stick_pos_is_changed == true) return;
        delay(SpeedDelay);
       
        for (int i=start_led; i <=  end_led; i=i+3) {
          strip.SetPixelColor(i+q, RgbColor(0, 0, 0)); //turn every third pixel off 
        }
    }
  }
}



void rainbowCycle(int SpeedDelay,  int start_led, int end_led) {
  uint16_t i, j;
  
  set_led_out_of_range(start_led, end_led);

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for (int i=start_led; i<= end_led; i++) {
      strip.SetPixelColor(i, Wheel(((i * 256 / (end_led-start_led)) + j) & 255));
    }
    //portDISABLE_INTERRUPTS();
    delay(2);  //安定待ち
    strip.Show();
    //portENABLE_INTERRUPTS();
      
    if(g_stick_pos_is_changed == true) return;
    delay(SpeedDelay);
  }
}


void rainbow(uint8_t wait, int start_led, int end_led) {
  uint16_t i, j;

  set_led_out_of_range(start_led, end_led);

  for(j=0; j<256; j++) {
    for(i=start_led; i<=end_led; i++) {
      strip.SetPixelColor(i, Wheel((i+j) & 255));
    }
      //portDISABLE_INTERRUPTS();
      delay(2);  //安定待ち
      strip.Show();
      //portENABLE_INTERRUPTS();
      
    if(g_stick_pos_is_changed == true) return;
    delay(wait);
  }
    Serial.print("rainbow");
}




// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
RgbColor Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void set_led_out_of_range(int start_led, int end_led){
  int i;
  
  for(i=0; i<start_led ;i++){
     strip.SetPixelColor(i, RgbColor(0, 0, 0));
  }
  if(end_led < PIXEL_COUNT){
    for(i=(end_led+1) ; i<PIXEL_COUNT;i++){
      strip.SetPixelColor(i, RgbColor(0, 0, 0));
    }
  }
      //portDISABLE_INTERRUPTS();
      delay(1);  //安定待ち
      strip.Show();
      //portENABLE_INTERRUPTS();
}
