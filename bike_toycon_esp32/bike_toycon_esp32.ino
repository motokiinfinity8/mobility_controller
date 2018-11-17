//#include <ESP8266WiFi.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define MPU6050_TOY_ADDR 0x68
#define MPU6050_BASE_ADDR 0x69
#define MPU6050_AX  0x3B
#define MPU6050_AY  0x3D
#define MPU6050_AZ  0x3F
#define MPU6050_TP  0x41    //  data not used
#define MPU6050_GX  0x43
#define MPU6050_GY  0x45
#define MPU6050_GZ  0x47

#define CMD_SIZE  12

#define DEBUG 1

float base_acc_x, base_acc_y, base_acc_z;
float toy_acc_x,  toy_acc_y,  toy_acc_z;
float base_acc_angle_x, base_acc_angle_y, base_acc_angle_z;
float toy_acc_angle_x, toy_acc_angle_y, toy_acc_angle_z;

int g_stick_pos = 0;    // 0: センター、4: 上、-4：下 に加えて、[+1:右、-1:左]を補正
int g_stick_pos_now = 15;
float linear_x, angular_z, gripper_data = 0.0;
uint32_t g_start_time = 0;


//demo
const char ssid[] = "ESP32_MOBILITY"; // SSID
const char password[] = "esp32_con";  // password
const int WifiPort = 8000;      // ポート番号

const IPAddress HostIP(192, 168, 11, 1);       // IPアドレス
const IPAddress ClientIP(192, 168, 11, 2);       // IPアドレス
const IPAddress subnet(255, 255, 255, 0); // サブネットマスク
const IPAddress gateway(192,168, 11, 0);
const IPAddress dns(192, 168, 11, 0);

WiFiUDP Udp;


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



void setup() {
  Serial.begin(115200);
  
  Serial.print("start i2c");
  //  i2c as a master
  Wire.begin();
  //  wake it up
  Wire.beginTransmission(MPU6050_BASE_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();

  Wire.beginTransmission(MPU6050_TOY_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();

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


void loop() {
    //  send MPU6050_BASE_ADDR
    //  send start address
    Wire.beginTransmission(MPU6050_BASE_ADDR);
    Wire.write(MPU6050_AX);
    Wire.endTransmission();  
    //  request 14bytes (int16 x 7)
    Wire.requestFrom(MPU6050_BASE_ADDR, 14);

    //  get 14bytes
    short int AccX, AccY, AccZ;
    AccX = Wire.read() << 8;  AccX |= Wire.read();
    AccY = Wire.read() << 8;  AccY |= Wire.read();
    AccZ = Wire.read() << 8;  AccZ |= Wire.read();

    //  X/Y方向を反転
    AccX = AccX * -1.0;
    AccY = AccY * -1.0;

    // 取得した加速度値を分解能で割って加速度(G)に変換する
    base_acc_x = AccX / 16384.0; //FS_SEL_0 16,384 LSB / g
    base_acc_y = AccY / 16384.0;
    base_acc_z = AccZ / 16384.0;

    // 加速度からセンサ対地角を求める
    base_acc_angle_x = atan2(base_acc_x, base_acc_z) * 360 / 2.0 / PI;
    base_acc_angle_y = atan2(base_acc_y, base_acc_z) * 360 / 2.0 / PI;
    base_acc_angle_z = atan2(base_acc_x, base_acc_y) * 360 / 2.0 / PI;


    //  send MPU6050_TOY_ADDR
    //  send start address
    Wire.beginTransmission(MPU6050_TOY_ADDR);
    Wire.write(MPU6050_AX);
    Wire.endTransmission();  
    //  request 14bytes (int16 x 7)
    Wire.requestFrom(MPU6050_TOY_ADDR, 14);

    //  get 14bytes
    AccX = Wire.read() << 8;  AccX |= Wire.read();
    AccY = Wire.read() << 8;  AccY |= Wire.read();
    AccZ = Wire.read() << 8;  AccZ |= Wire.read();

    // 取得した加速度値を分解能で割って加速度(G)に変換する
    toy_acc_x = AccX / 16384.0; //FS_SEL_0 16,384 LSB / g
    toy_acc_y = AccY / 16384.0;
    toy_acc_z = AccZ / 16384.0;

    // 加速度からセンサ対地角を求める
    toy_acc_angle_x = atan2(toy_acc_x, toy_acc_z) * 360 / 2.0 / PI;
    toy_acc_angle_y = atan2(toy_acc_y, toy_acc_z) * 360 / 2.0 / PI;
    toy_acc_angle_z = atan2(toy_acc_x, toy_acc_y) * 360 / 2.0 / PI;

    //  debug monitor
    if(DEBUG){ 
      Serial.print("BASE ["); Serial.print(base_acc_angle_x);
      Serial.print("  "); Serial.print(base_acc_angle_y);
      Serial.print("  "); Serial.print(base_acc_angle_z);
      Serial.print("]  TOY ["); Serial.print(toy_acc_angle_x);
      Serial.print("  "); Serial.print(toy_acc_angle_y);
      Serial.print("  "); Serial.print(toy_acc_angle_z);
      Serial.println("]");
    }

    // cmd_vel判定① : linear.x
    // TOY y角とBASE y角に対してTOY y角が90°以上になった場合、Baseとの差分角分の前進命令を送る
    float max_linear = 0.5;
    float start_angle = 92;
    float max_angle = 145;
    float diff_angle;
    if(toy_acc_angle_y >= 0)    diff_angle = toy_acc_angle_y - base_acc_angle_y;
    else diff_angle = 360 + toy_acc_angle_y - base_acc_angle_y;
    //if(diff_angle < 0 )  diff_angle = 360 + diff_angle;
    
    if(diff_angle  < start_angle || base_acc_angle_y > 90 || base_acc_angle_y < -90){
      linear_x = 0.0;
      gripper_data = 0.0;
    }else if(diff_angle  > max_angle){
      linear_x = max_linear;
      gripper_data = 1.0;
    }else{
      linear_x = max_linear * ( diff_angle - start_angle) / (max_angle - start_angle); 
      gripper_data = ( diff_angle - start_angle) / (max_angle - start_angle); 
    }

    // BASE y角が25°以上の場合は停止、35°以上の場合は後退
    if(base_acc_angle_y >= 20 && base_acc_angle_y < 30){
       linear_x = 0.0;
    }else if(base_acc_angle_y >= 30){
       linear_x = linear_x * -1;
    }

    // cmd_vel判定② :angular_z
    //　BASE x角に応じて、左右方向の命令を送る
    if(linear_x == 0.0){
      angular_z = 0.0;
    }else if(base_acc_angle_x >= 25){
      angular_z =0.5 * linear_x /  max_linear;
      linear_x = 0.0;  
    }else if(base_acc_angle_x <= -25){
      angular_z =-0.5 * linear_x /  max_linear;
      linear_x = 0.0;  
    }else if(abs(base_acc_angle_x) < 35){
      angular_z = base_acc_angle_x / 35.0;
    }
    Serial.print("diff_angle:");
    Serial.print(diff_angle);
    Serial.print("linear_x:");
    Serial.print(linear_x);
    Serial.print("angular_z:");
    Serial.println(angular_z);

    // JOY_STICK値に変換出力（新規作成)
    if(linear_x > 0.1){
      g_stick_pos = 4;
    }else if(linear_x < -0.1){
      g_stick_pos = -4;
    }else{
      g_stick_pos = 0;
    }

    if(angular_z  > 0.1){
      g_stick_pos += 1;
    }else if(angular_z < -0.1 ){
      g_stick_pos -= 1;
    }


  //スティックが倒されている間の時間測定
  if(g_stick_pos == 0)  g_start_time = millis();  //時間リセット
  //Serial.println(Time_Mesure(g_start_time));
  Serial.print("g_stick_pos:");
  Serial.println(g_stick_pos);

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
  
  m_buf.concat("1");  // 音声予告モード 
  //m_buf.concat(0x3);  // ETX設定 

  //byte [] wifi_buf = m_buf.getBytes();
  byte wifi_buf[CMD_SIZE-2];
  m_buf.getBytes(wifi_buf, CMD_SIZE-2);
  sendWiFi(wifi_buf);
  //Serial.print(m_buf);
//  int i;
//  for(i=0;i<10;i++){
//    Serial.print(wifi_buf[i]);
//    Serial.print(" ");
//  }
//  Serial.println(" ");
  
  g_stick_pos_now = g_stick_pos;
  delay(100);  //安定待ち
  
}
