#include <ESP8266WiFi.h>
#include <ros.h>
#include <Wire.h>
#include <geometry_msgs/Twist.h>
//#include <tf/transform_broadcaster.h>

#define MPU6050_TOY_ADDR 0x68
#define MPU6050_BASE_ADDR 0x69
#define MPU6050_AX  0x3B
#define MPU6050_AY  0x3D
#define MPU6050_AZ  0x3F
#define MPU6050_TP  0x41    //  data not used
#define MPU6050_GX  0x43
#define MPU6050_GY  0x45
#define MPU6050_GZ  0x47

#define DEBUG 1

float base_acc_x, base_acc_y, base_acc_z;
float toy_acc_x,  toy_acc_y,  toy_acc_z;
float base_acc_angle_x, base_acc_angle_y, base_acc_angle_z;
float toy_acc_angle_x, toy_acc_angle_y, toy_acc_angle_z;


// WiFi configuration. Replace '***' with your data

// hirose_home
//const char* ssid     = "Buffalo-G-6FD0";
//const char* password = "5s3rhewh3uytn";
//IPAddress server(192,168,0,21);      // Set the rosserial socket ROSCORE SERVER IP address

//demo
const char* ssid     = "0024A5B643EE";
const char* password = "mup6255v0fwev";
IPAddress server(192,168,11,3);      // Set the rosserial socket ROSCORE SERVER IP address
const uint16_t serverPort = 11411;    // Set the rosserial socket server port

void setupWiFi() {                    // connect to ROS server as as a client
  if(DEBUG){
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  if(DEBUG){
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}


// ROS nodes //
ros::NodeHandle nh;
//tf::TransformBroadcaster broadcaster;
geometry_msgs::Twist cmdvel_msg;        // 

ros::Publisher pub_cmdvel("/toycon/cmd_vel", &cmdvel_msg); 

void setup() {
  Serial.begin(115200);
  setupWiFi();
  delay(2000);
// Ros objects constructors   
  nh.getHardware()->setConnection(server, serverPort);
  nh.initNode();
//  broadcaster.init(nh);
  nh.advertise(pub_cmdvel);
  
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

  //set cmd_vel initial
  cmdvel_msg.linear.x = 0.0;
  cmdvel_msg.linear.y = 0.0;
  cmdvel_msg.linear.z = 0.0;
  cmdvel_msg.angular.x = 0.0;
  cmdvel_msg.angular.y = 0.0;
  cmdvel_msg.angular.z = 0.0;
  pub_cmdvel.publish( &cmdvel_msg );
}

void loop() {
  if (nh.connected()) {
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
    float start_angle = 90;
    float max_angle = 180;
    float diff_angle_handle;
    diff_angle_handle = toy_acc_angle_y - base_acc_angle_y;
    if(diff_angle_handle < 0 )  diff_angle_handle = 360 + diff_angle_handle;
    
    if(diff_angle_handle  < start_angle ){
      cmdvel_msg.linear.x = 0.0;
    }else if(diff_angle_handle  > max_angle){
      cmdvel_msg.linear.x = max_linear;
    }else{
      cmdvel_msg.linear.x = max_linear * ( diff_angle_handle - start_angle) / (max_angle - start_angle); 
    }

    // cmd_vel判定② :angular.z
    //　BASE x角に応じて、左右方向の命令を送る
    if(cmdvel_msg.linear.x == 0.0){
      cmdvel_msg.angular.z = 0.0;
    }else if(base_acc_angle_x >= 30){
      cmdvel_msg.angular.z =0.5 * cmdvel_msg.linear.x /  max_linear;
      cmdvel_msg.linear.x = 0.0;  
    }else if(base_acc_angle_x <= -30){
      cmdvel_msg.angular.z =-0.5 * cmdvel_msg.linear.x /  max_linear;
      cmdvel_msg.linear.x = 0.0;  
    }else if(abs(base_acc_angle_x) < 30){
      cmdvel_msg.angular.z = base_acc_angle_x / 30.0;
    }

    // cmd_vel publish
    pub_cmdvel.publish( &cmdvel_msg );
  } else {
    if(DEBUG) Serial.println("Not Connected");
  }
  nh.spinOnce();
  // Loop aprox. every  
  delay(50);  // milliseconds
}
