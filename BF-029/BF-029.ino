// copyright 2021 BotanicFields, Inc.
// Demonstration of DFPlayerMini
//
#include <M5Atom.h>
#include "BF_DfplayerMini.h"

DfplayerMini Dfpm;

const int loop_ms = 10;
int loop_last_ms  =  0;
int loop_count    =  0;

void setup()
{
  const bool serial_enable(true);
  const bool i2c_enable(true);
  const bool display_enable(true);
  M5.begin(serial_enable, !i2c_enable, display_enable);
  delay(3000);
  Serial.println("M5Stack begin");

  // Port UART
  const int serial2_rx(32);  // GPIO22
  const int serial2_tx(26);  // GPIO19
  Serial2.begin(9600, SERIAL_8N1, serial2_rx, serial2_tx);

  // for DFPlayer-Mini
  const bool feedback_enable(true);
  Dfpm.Begin(Serial2, !feedback_enable);
  delay(3000);
  Dfpm.Reset();
  delay(3000);

  loop_last_ms = millis();
  loop_count = 0;
}

void loop()
{
  M5.update();
  Dfpm.Update();

  static int dfpm_select(0);
  if (M5.Btn.wasReleased())
    switch (dfpm_select) {
      case  0: Dfpm.SelectDevice(0x01);  dfpm_select = 1;  break;
      case  1: Dfpm.SelectDevice(0x02);  dfpm_select = 0;  break;
      default: dfpm_select = 0;  break;
    }

  if (loop_count % 500 == 0)
    Dfpm.Next();

  delay(loop_ms + loop_last_ms - millis());
  loop_last_ms = millis();
  ++loop_count;
}
