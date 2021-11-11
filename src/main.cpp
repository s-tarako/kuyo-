#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <Wire.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);
#define line(x, y, x2, y2); display.drawLine(x, y, x2, y2, 2);
#define upd display.display();
#define getAmp ((getVolt() - offset) * 1000 / 50)
#define histnum 50
#define BT1 1
#define BT2 2
#define BT3 3
#define st(p) !digitalRead(p)

int cps = 10;
void text(String text, int posx = -1, int posy = -1)
{
  if (posx != -1 && posy != -1)
    display.setCursor(posx, posy);
  display.print(text);
}
float offset=0;
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
  offset=getVolt();
  text(String(offset));
  upd
  delay(500);
  display.clearDisplay();
  pinMode(BT1,INPUT_PULLUP);
  pinMode(BT2,INPUT_PULLUP);
  pinMode(BT3,INPUT_PULLUP);

}
double rangeB = 0;
double rangeU = 0;
double avg = 0;
double amphist[histnum];
void histupdate()
{
  rangeB = 0;
  rangeU = 0;

  for (int i = 0; i < histnum - 1; i++)
  {
    amphist[i] = amphist[i + 1];
    rangeB = min(amphist[i], rangeB);
    rangeU = max(amphist[i], rangeU);
    avg += amphist[i];
  }
  amphist[histnum - 1] = getAmp;
  avg += amphist[histnum - 1];
  rangeB = min(amphist[histnum - 1], rangeB);
  rangeU = max(amphist[histnum - 1], rangeU);
  avg /= histnum;
}
long update = 0;
#define Gheight 51
#define GSX 25
void Plot(){
      histupdate();
    display.clearDisplay();
    line(GSX-1, 0, GSX-1, 55);
    line(GSX-1, 55, 127, 55);
    for (int i = 0; i < histnum; i++)
    {
      if (i != 0)
      {
        display.drawLine(
            round(map(i - 1, 0, histnum - 1, GSX, 127)),
            Gheight - round(map(amphist[i - 1] *100, rangeB *100, rangeU *100, 0, Gheight)),
            round(map(i, 0, histnum - 1, GSX, 127)),
            Gheight - round(map(amphist[i] *100, rangeB *100, rangeU *100, 0, Gheight)),
            INVERSE);
      }
      else
      {
        display.drawPixel(
            round(map(i, 0, histnum - 1, GSX, 127)),
            Gheight - round(map(amphist[i] *100, rangeB *100, rangeU *100, 0, Gheight)),
            INVERSE);
      }
    }
    line(0, 0, GSX-1, 0);
    line(0, Gheight, GSX-1, Gheight);
    text(String(rangeU, rangeU >= 10 || rangeU <= -10 ? 0 : 1), 0, 1);
    double centerline = ((rangeB + rangeU) / 2);
    text(String(centerline, centerline >= 10 || centerline <= -10 ? 0 : 1), 0, map(centerline *100, rangeB *100, rangeU *100, 0, Gheight) - 3);
    line(GSX, map(centerline *100, rangeB *100, rangeU *100, 0, Gheight), 127, map(centerline *100, rangeB *100, rangeU *100, 0, Gheight));
    text(String(rangeB,rangeB >= 10 || rangeB <= -10 ? 0 : 1), 0, Gheight - 8);
    text("Val:", 64, 56);
    text(String(amphist[histnum - 1], 2));
    text("A");
    text("Avg:", 0, 56);
    text(String(avg, avg >= 10 || avg <= -10 ? 1 : 2));
    text("A");
    upd
}
void loop()
{
  if (update <= millis())
  {
    update = millis() + 1000 / cps;
    Plot();
  }
}
