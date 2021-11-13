#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Wire.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);
#define line(x, y, x2, y2); display.drawLine(x, y, x2, y2, 2);
#define upd display.display();
#define getAmp ((getVolt() - offset) * 1000 / 50)
#define conf_num 4
#define C_MPS 0
#define C_HIST 1
#define C_SAMP 2
#define C_HIST_MAX 100
#define st(p) !digitalRead(p)
//button
int bt_pin[3] = {1, 2, 3};
long bt_state[3] = {0, 0, 0};
bool bt_trig[3] = {0, 0, 0};

//config
int conf_list[conf_num] = {
    10, //mps
    40, //hist
    10  //samples
};
bool conf_flag=false;
float offset = 0;

void text(String text, int posx = -1, int posy = -1)
{
  if (posx != -1 && posy != -1)
    display.setCursor(posx, posy);
  display.print(text);
}

float getVolt()
{
  double sum = 0;
  for (int i = 0; i < 10; i++)
  {
    sum += analogRead(A0) * 5.0 / 1024;
  }
  return sum / 10;
}
int maxv, minv;
void setup()
{
  delay(100);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  delay(100);
  text("offset:", 0, 0);
  offset = getVolt();
  text(String(offset));
  upd
  delay(500);
  display.clearDisplay();
  pinMode(bt_pin[0], INPUT_PULLUP);
  pinMode(bt_pin[1], INPUT_PULLUP);
  pinMode(bt_pin[2], INPUT_PULLUP);
}
double rangeB = 0;
double rangeU = 0;
double avg = 0;
double amphist[C_HIST_MAX];
void histupdate()
{
  rangeB = 0;
  rangeU = 0;

  for (int i = 0; i < conf_list[C_HIST] - 1; i++)
  {
    amphist[i] = amphist[i + 1];
    rangeB = min(amphist[i], rangeB);
    rangeU = max(amphist[i], rangeU);
    avg += amphist[i];
  }
  amphist[conf_list[C_HIST] - 1] = getAmp;
  avg += amphist[conf_list[C_HIST] - 1];
  rangeB = min(amphist[conf_list[C_HIST] - 1], rangeB);
  rangeU = max(amphist[conf_list[C_HIST] - 1], rangeU);
  avg /= conf_list[C_HIST];
}
long update = 0;
#define GH 51
#define GSX 25
void gPlot()
{
  histupdate();
  display.clearDisplay();
  line(GSX - 1, 0, GSX - 1, 55);
  line(GSX - 1, 55, 127, 55);
  for (int i = 0; i < conf_list[C_HIST]; i++)
  {
    if (i != 0)
    {
      display.drawLine(
          round(map(i - 1, 0, conf_list[C_HIST] - 1, GSX, 127)),
          GH - round(map(amphist[i - 1] * 100, rangeB * 100, rangeU * 100, 0, GH)),
          round(map(i, 0, conf_list[C_HIST] - 1, GSX, 127)),
          GH - round(map(amphist[i] * 100, rangeB * 100, rangeU * 100, 0, GH)),
          INVERSE);
    }
    else
    {
      display.drawPixel(
          round(map(i, 0, conf_list[C_HIST] - 1, GSX, 127)),
          GH - round(map(amphist[i] * 100, rangeB * 100, rangeU * 100, 0, GH)),
          INVERSE);
    }
  }
  line(0, 0, GSX - 1, 0);
  line(0, GH, GSX - 1, GH);
  text(String(rangeU, rangeU >= 10 || rangeU <= -10 ? 0 : 1), 0, 1);
  double centerline = ((rangeB + rangeU) / 2);
  text(String(centerline, centerline >= 10 || centerline <= -10 ? 0 : 1), 0, map(centerline * 100, rangeB * 100, rangeU * 100, 0, GH) - 3);
  line(GSX, map(centerline * 100, rangeB * 100, rangeU * 100, 0, GH), 127, map(centerline * 100, rangeB * 100, rangeU * 100, 0, GH));
  text(String(rangeB, rangeB >= 10 || rangeB <= -10 ? 0 : 1), 0, GH - 8);
  text("Val:", 64, 56);
  text(String(amphist[conf_list[C_HIST] - 1], 2));
  text("A");
  text("Avg:", 0, 56);
  text(String(avg, avg >= 10 || avg <= -10 ? 1 : 2));
  text("A");
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
    }
    else if (st(bt_pin[i]) && !bt_state[i])
    {
      bt_state[i] = false;
    }
  }
}
void config()
{
  text("config", 0, 0);
  display.drawRect(8, (conf_index + 1) * 7 + 3, 127, (conf_index + 1) * 7 + 10, INVERSE);
  if (bt_trig[0])
  {
    conf_index < 4 ? conf_index++ : conf_index = 0;

    bt_trig[0] = 0;
  }
  if (bt_trig[1])
  {
    switch (conf_index)
    {
    case 0: //mps
      if (conf_list[C_MPS] > 1)
        conf_list[C_MPS] -= 1;
      break;

    case 1: //hist
      if (conf_list[C_HIST] > 2)
        conf_list[C_HIST] -= 1;

      break;

    case 2:

      break;

    case 3:
      offset = getVolt();

      break;
    case 4:
      conf_flag = false;
      conf_index = 0;
      break;
    }
  

    bt_trig[1] = false;
  }
  if (bt_trig[2])
  {
    switch (conf_index)
    {
    case 0: //mps
      if (conf_list[C_MPS] < 30)
        conf_list[C_MPS] += 1;
      break;

    case 1: //hist
      if (conf_list[C_HIST] < 100)
        conf_list[C_HIST] += 1;

      break;

    case 2:

      break;

    case 3:
      offset = getVolt();
      break;
    case 4:
      conf_flag = 0;
      conf_index = 0;
      break;
    }
    bt_trig[2] = false;
  }
  text("measure/sec:", 10, 7 + 3);
  text(String(conf_list[C_MPS]));
  text("history shows:", 10, 7 * 2 + 3);
  text(String(conf_list[C_HIST]));
  //text("show points:",10,7*3+3);
  text("offset:reset ...", 10, 7 * 4 + 3);
  text(String(offset));
  text("V");
  text("exit", 10, 7 * 5 + 3);
  upd
}

long btupdate = 0;
void loop()
{
  if (btupdate <= millis())
  {
    btupdate = millis() + 50;
    bt_update();
  }
  if (conf_flag)
  {
    config();
  }
  else
  {
    if (bt_trig[0])
    {
      bt_trig[0] = false;
      conf_flag = true;
    }
    if (update <= millis())
    {
      update = millis() + 1000 / conf_list[C_MPS];
      gPlot();
    }
  }
}
