/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial  // BLYNKの詳細情報を表示するか否か

/* Fill-in your Template ID (only if using Blynk.Cloud) */
#define BLYNK_TEMPLATE_ID "TMPL7j6KRdR3"  // テンプレートIDを入力
#define BLYNK_DEVICE_NAME "TLab sense"    // テンプレート上で製作したデバイス名を入力
#define BLYNK_AUTH_TOKEN "I4nsgJTxTMNc0Ua2av4RvkCgcaKZeLMG" //Blynkで発行されたパス (必須)

char auth[] = BLYNK_AUTH_TOKEN;

/* Ethernet LIBRARY */
#include <Ethernet.h>            // Ethernet
#include <BlynkSimpleEthernet.h> // BlynkサーバのEthernet

/* DHT11 PART */
//#include <SPI.h>
#include <DHT.h>          // DHT11のヘッダファイル
#define DHTPIN 3          // What digital pin we're connected to
#define DHTTYPE DHT11     // DHT11を宣言
DHT dht(DHTPIN, DHTTYPE); // DHT11のパスを通す

/* RELAY PART */
#define RELAY 4
int RELAY_VAL = 0;

/* CO2 PART */
#define RESET_DURATION 86400000UL // 1 day
void software_reset() {
  asm volatile ("  jmp 0");
}
// CO2センサの設定
#include <Wire.h>
#include <Narcoleptic.h>
#define CO2_senser_Address (0x31)
// CO2センサのデータ取得関数
//s300のppm値の取得関数
int  getCO2ppm() {
  byte tmpBuf[7];
  sendCommand('R');
  Wire.requestFrom(CO2_senser_Address, 7);
  for (int i = 0; Wire.available(); i++) {    //Wire.available():マスタが受信するデータ数　7から0になる
    tmpBuf[i] = Wire.read();
    delay(1);
  }
  if (tmpBuf[0] != 0x08 ||
      tmpBuf[3] == 0xff ||
      tmpBuf[4] == 0xff ||
      tmpBuf[5] == 0xff ||
      tmpBuf[6] == 0xff) {
        return 0;
  }
  return (int)((tmpBuf[1] << 8) | tmpBuf[2]);
}
//CO2センサ コマンド送信関数
void sendCommand(byte val) {
  Wire.beginTransmission(CO2_senser_Address);
  Wire.write(val);
  Wire.endTransmission();
}
//CO2　センサスリープコマンド
void sleep_cmd() {
  sendCommand('S');
  delay(4000);
}
// CO2 センサ　ウェイクアップコマンド
void wakeup() {
  sendCommand('W');
  delay(6000);
}


BlynkTimer timer; //Blynkにデータ送信する際に干渉させないための宣言
#include <Wire.h>

void setup(){
  Serial.begin(9600);  // Serialの初期設定
  Blynk.begin(auth);   // Blynkの初期設定
  Wire.begin();                     //I2C通信のセンサ

  /* DHT11_setup */
  dht.begin();         // dhtの取得レートの初期設定

  /* RELAY PART */
  pinMode(RELAY, OUTPUT);

  //　時間用変数
  timer.setInterval(60000L,sendSensor);//1分間隔データ所得
}

void loop(){
  Blynk.run();  // Blynkのプログラムを動作
  timer.run();  // 干渉防止のプログラムを動作
}

BLYNK_WRITE(V7){
  RELAY_VAL = param[0].asInt();
  // 本当はここにRELAYのON / OFFを直接ぶっこむ
  /*
  if(RELAY_VAL == 1){
    digitalWrite (RELAY, HIGH); // relay conduction;
  else if(RELAY_VAL == 0){
    digitalWrite (RELAY, LOW); // relay switch is turned off;
  }*/
  digitalWrite(RELAY,RELAY_VAL);
}

void sendSensor(){
  // --- 環境データ取得 ---
  int ppm = getCO2ppm();                     // センサから二酸化炭素濃度を取得
  float t = dht.readTemperature();           // センサから温度を取得
  float h = dht.readHumidity();              // センサから湿度を取得
  float DI = 0.81*t + (0.01*h) * (0.99*t - 14.3) + 46.3; // 不快度指数
  
  /*不快度指数によるケース分け*/
  String DI_COM = "普通な";//不快度指数のメッセージ宣言  
  if(65.0<=DI && DI<70.0){
    DI_COM = "快い";
  }
  else if(70.0<=DI && DI<75.0){
    DI_COM = "暑くない";
  }
  else if(75.0<=DI && DI<80.0){
    DI_COM = "やや暑い";
  }
  else if(80.0<=DI && DI<85.0){
    DI_COM = "暑くて汗が出る";
  }
  else if(85.0<=DI){
    DI_COM = "暑くてたまらない";
  }
  else{
    DI_COM = "熱中症の心配がない";
  }
  
  // --- エラーメッセ ---
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");  // エラーメッセージ
    return;
  }

  // --- OUTPUT ---
  Blynk.virtualWrite(V0, ppm); //　ppmをBlynkサーバのV0(Pin)に送信
  Blynk.virtualWrite(V2, t);   //　TemperatureをBlynkサーバのV2(Pin)に送信
  Blynk.virtualWrite(V3, h);   //　HumidityをBlynkサーバのV3(Pin)に送信
  Blynk.virtualWrite(V5, DI);   //　不快度指数をBlynkサーバのV5(Pin)に送信
  Blynk.virtualWrite(V6, DI_COM);   //　不快度指数をBlynkサーバのV6(Pin)に送信

  
  //Serial.println(ppm);
  
}
