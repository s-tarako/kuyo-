#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);
#define line(x, y, x2, y2) \
  ;                        \
  display.drawLine(x, y, x2, y2, 2);
#define upd display.display();
#define getAmp ((getVolt() - offset) * 1000 / 40)
#define conf_num 4
#define C_MPS 0
#define C_HIST 1
#define C_SAMP 2
#define C_LOG 3
#define C_HIST_MAX 120
#define st(p) (!digitalRead(p))
/*
pinmap:
I2C:A4,A5
Button:3,4,5
SPI(SD):13,12,11,10
*/
//button
char bt_pin[3] = {6, 8, 7};
bool bt_state[3] = {0, 0, 0};
bool bt_trig[3] = {0, 0, 0};


//config
uint8_t conf_list[conf_num] = {
    20, //fps
    60, //hist
    20, //samples
    2   //log
};
String unit="A";
const unsigned char conf_min[conf_num] PROGMEM = {
    1, 2, 1, 0};
const unsigned char conf_max[conf_num] PROGMEM = {
    30, 120, 100, 20};

bool conf_flag = false;
float offset = 0;

//SD/;
bool sd_ok;
bool sd_logflag;
int sd_logno = 0;

//values
double rangeB = 0;
double rangeU = 0;
double avg = 0;
float valhist[C_HIST_MAX];
unsigned long btupdate = 0;
unsigned long update = 0;
unsigned long hupdate = 0;
unsigned long a_hupdate = 0;
unsigned long lupdate = 0;
unsigned long log_offset = 0;

float getVolt()
{
  double sum = 0;
  for (int i = 0; i < 10; i++)
  {
    sum += (float)analogRead(A0) * 5.0 / 1024;
  }
  return sum / 10;
}
float getValue(){
  return getAmp;
}
void text(String text, int posx = -1, int posy = -1)
{
  if (posx != -1 && posy != -1)
  {
    display.setCursor(posx, posy);
  }
  display.print(text);
}
bool conf_load()
{
  uint8_t t_value = 0;
  if (EEPROM.read(0) == 254)
  {
    for (int i = 0; i < conf_num; i++)
    {
      t_value = EEPROM.read(i + 1);
      if (t_value <= conf_min[i] && t_value >= conf_max[i] && t_value == 0)
        return 0;
      conf_list[i] = t_value - 1;
    }
  }
  else
  {
    return 0;
  }
  return 1;
}
void conf_save()
{
  EEPROM.write(0, 254);
  for (int i = 0; i < conf_num; i++)
  {
    EEPROM.write(i + 1, conf_list[i] + 1);
  }
}
void conf_reset()
{
  display.clearDisplay();
  text("reset?",0,0);
  text("no(left):yes(right)",0,10);
  upd
  delay(500);
  while(1){
    if(st(bt_pin[1])){
      while(st(bt_pin[1])){}
      break;
    }
    if(st(bt_pin[2])){
      while(st(bt_pin[2])){}
       EEPROM.write(0, 0);
  for (int i = 0; i < conf_num; i++)
  {
    EEPROM.write(i + 1,0);
  } text("reset success.\nplease reboot",0,20);
  upd
  asm volatile ("  jmp 0");  
  break;
    }
  }

  EEPROM.write(0, 0);
  for (int i = 0; i < conf_num; i++)
  {
    EEPROM.write(i + 1,0);
  }
}
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  text("offset:", 0, 0);
  offset = getVolt();
  text(String(offset, 3));
  digitalWrite(LED_BUILTIN, HIGH);
  text("config loading:", 0, 7 * 1);
  text(conf_load() ? "OK" : "failed");
  text("SD initialize:", 0, 7 * 2);
  Serial.begin(115200);
  sd_ok = SD.begin(10);
  text(sd_ok ? "OK" : "failed");
  upd
      delay(2000);
  display.clearDisplay();
  pinMode(bt_pin[0], INPUT_PULLUP);
  pinMode(bt_pin[1], INPUT_PULLUP);
  pinMode(bt_pin[2], INPUT_PULLUP);
}
void SD_log(String text)
{
  File dataFile = SD.open("log" + String(sd_logno) + ".csv", FILE_WRITE);
  if (dataFile)
  {
    dataFile.println(text);
    dataFile.close();
  }
}
void histupdate()
{
  avg = 0;
  rangeB = 0;
  rangeU = 0;

  for (int i = 0; i < conf_list[C_HIST] - 1; i++)
  {
    valhist[i] = valhist[i + 1];
    rangeB = min(valhist[i], rangeB);
    rangeU = max(valhist[i], rangeU);
    avg += valhist[i];
  }
  valhist[conf_list[C_HIST] - 1] = getValue();
  avg += valhist[conf_list[C_HIST] - 1];
  rangeB = min(valhist[conf_list[C_HIST] - 1], rangeB);
  rangeU = max(valhist[conf_list[C_HIST] - 1], rangeU);
  avg /= conf_list[C_HIST];
  a_hupdate=millis();
}
void logupdate()
{
  if (sd_ok && sd_logflag)
    SD_log(String((float)(millis()-log_offset) / 1000, 2)+ "," + String(valhist[conf_list[C_HIST-1]], 2));

  Serial.println(String(valhist[conf_list[C_HIST-1]],2));
}

#define GH 51
#define GSX 25
void gPlot()
{


  display.clearDisplay();
    int period=conf_list[C_HIST]*(1000/conf_list[C_SAMP]);
  float period_c=period/1000;
  for(int i=0;i<period_c;i++){
      
       display.drawLine(
          127-round(map(i*100,0,period_c*100,0,127-GSX)),
          GH,
         127-round(map(i*100,0,period_c*100,0,127-GSX)),
          GH+1,
          WHITE);

  }
  int goffset=-map((millis()-a_hupdate),0,(1000 / conf_list[C_SAMP]),0, 120/(conf_list[C_HIST]-1));
  for (int i = 0; i < conf_list[C_HIST]; i++)
  {
    if (i != 0)
    {
      display.drawLine(
          round(map(i - 1, 0, conf_list[C_HIST] - 1, GSX, 127))+goffset,
          GH - round(map(valhist[i - 1] * 100, rangeB * 100, rangeU * 100, 0, GH)),
          round(map(i, 0, conf_list[C_HIST] - 1, GSX, 127))+goffset,
          GH - round(map(valhist[i] * 100, rangeB * 100, rangeU * 100, 0, GH)),
          WHITE);
    }
  }
  display.fillRect(0,0,GSX,GH,BLACK);
  line(GSX - 1, 0, GSX - 1, 55);
  line(GSX - 1, 55, 127, 55);
  line(0, 0, GSX - 1, 0);
  line(0, GH, GSX - 1, GH);
  text(String(rangeU, rangeU >= 10 || rangeU <= -10 ? 0 : 1), 0, 1);
  double centerline = ((rangeB + rangeU) / 2);
  text(String(centerline, centerline >= 10 || centerline <= -10 ? 0 : 1), 0, map(centerline * 100, rangeB * 100, rangeU * 100, 0, GH) - 3);
  line(GSX,GH/2, 127, GH/2);
  text(String(rangeB, rangeB >= 10 || rangeB <= -10 ? 0 : 1), 0, GH - 8);
  text("Val:", 64, 56);
  text(String(valhist[conf_list[C_HIST] - 1], 2));
  text(unit);
  text("Avg:", 0, 56);
  text(String(avg, avg >= 10 || avg <= -10 ? 1 : 2));
  text(unit);
  display.drawPixel(127, 63, sd_ok && sd_logflag);
  upd
}
char conf_index = 0;
void bt_update()
{
  for (int i = 0; i < 3; i++)
  {
    if (st(bt_pin[i]) && !bt_state[i])
    {
      bt_state[i] = true;
      bt_trig[i] = true;
      digitalWrite(LED_BUILTIN, HIGH);
    }
    else if (!st(bt_pin[i]) && bt_state[i])
    {
      bt_state[i] = false;
    }
  }
}
void config()
{
  display.clearDisplay();
  text("config", 0, 0);

  if (bt_trig[0])
  {
    conf_index < 5 ? conf_index++ : conf_index = 0;

    bt_trig[0] = 0;
  }
  if (bt_trig[1])
  {
    switch (conf_index)
    {
    case 0: //mps
      if (conf_list[C_MPS] > conf_min[C_MPS])
      {
        conf_list[C_MPS] -= 1;
      }
      break;

    case 1: //hist
      if (conf_list[C_HIST] > conf_min[C_HIST])
      {
        conf_list[C_HIST] -= 1;
      }

      break;

    case 2:
      if (conf_list[C_SAMP] > conf_min[C_SAMP])
      {
        conf_list[C_SAMP] -= 1;
      }
      break;
    case 3:
      if (conf_list[C_LOG] > conf_min[C_LOG])
      {
        conf_list[C_LOG] -= 1;
      }
      break;
    case 4:
      offset = getVolt();

      break;
    case 5:
      conf_flag = false;
      conf_index = 0;
      conf_save();
      break;
    }

    bt_trig[1] = false;
  }
  if (bt_trig[2])
  {
    switch (conf_index)
    {
    case 0: //mps
      if (conf_list[C_MPS] < conf_max[C_MPS])
      {
        conf_list[C_MPS] += 1;
      }
      break;

    case 1: //hist
      if (conf_list[C_HIST] < conf_max[C_HIST])
      {
        conf_list[C_HIST] += 1;
      }
      break;

    case 2:
      if (conf_list[C_SAMP] < conf_max[C_SAMP])
      {
        conf_list[C_SAMP] += 1;
      }
      break;
    case 3:
      if (conf_list[C_LOG] < conf_max[C_LOG])
      {
        conf_list[C_LOG] += 1;
      }
      break;
    case 4:
      offset = getVolt();
      break;
    case 5:
      conf_reset();
      break;
    }
    bt_trig[2] = false;
  }
  text("frame/sec:", 10, 8 + 3);
  text(String(conf_list[C_MPS]));
  text("history shows:", 10, 8 * 2 + 3);
  text(String(conf_list[C_HIST]));
  text("sampling/sec:", 10, 8 * 3 + 3);
  text(String(conf_list[C_SAMP]));
  text("log/sec:", 10, 8 * 4 + 3);
  text(String(conf_list[C_LOG]));
  //text("show points:",10,7*3+3);
  text("offset: ...", 10, 8 * 5 + 3);
  text(String(offset));
  text("V");
  text("exit&save / reset", 10, 8 * 6 + 3);
  display.fillRect(8, (conf_index + 1) * 8 + 2, 127 - 8, 9, INVERSE);
  upd
}
void loop()
{
  unsigned long time = millis();
  if (btupdate <= time)
  {
    btupdate = time + 20;
    bt_update();
  }

  if (hupdate <= time)
  {
    hupdate = time + 1000 / conf_list[C_SAMP];
    histupdate();
  }
  if (lupdate <= time)
  {
    if (conf_list[C_LOG] != 0)
    {
      lupdate = time + 1000 / conf_list[C_LOG];
      logupdate();
    }
  }
  if (conf_flag)
  {
    config();
  }
  else
  {
    if (bt_trig[1]&&sd_ok)
    {
      sd_logflag = !sd_logflag;
      if (sd_logflag)
      {
        log_offset=millis();
        while (SD.exists("log" + String(sd_logno) + ".csv"))
        {
          sd_logno++;
        }

        File dataFile = SD.open("log" + String(sd_logno) + ".csv", FILE_WRITE);
        if (dataFile)
        {
          dataFile.println("sec,"+unit);
          dataFile.close();
        }
      }
      bt_trig[1] = 0;
    }
    if (bt_trig[0])
    {
      bt_trig[0] = false;
      conf_flag = true;
      bt_trig[1] = false;
      bt_trig[2] = false;
    }

    if (update <= time)
    {
      update = time + 1000 / conf_list[C_MPS];

      gPlot();
    }
  }
}
