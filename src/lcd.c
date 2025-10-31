#include <lcd.h>
#include <config.h>
#include <hd44780pcf8574.h>
#include <stdio.h>
#include <stdlib.h>

// initializes the TWI display
void init_twi_display() {
  HD44780_PCF8574_Init(I2C_DISPLAY_ADDR); // already calls TWI_Init();
  HD44780_PCF8574_DisplayClear(I2C_DISPLAY_ADDR);
  HD44780_PCF8574_DisplayOn(I2C_DISPLAY_ADDR);
  HD44780_PCF8574_PositionXY(I2C_DISPLAY_ADDR, 0, 0);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, "Dist (cm): 0.0");
  HD44780_PCF8574_PositionXY(I2C_DISPLAY_ADDR, 0, 1);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, "Freq (hz): 0");
}

// updates the values of the I2C LCD (note: distance and frequency will be rounded)
void update_twi_display(float dist, float freq) {
  char buf[6]; // room for 5 chars

  // distance
  HD44780_PCF8574_PositionXY(I2C_DISPLAY_ADDR, 11, 0);
  dtostrf(dist, 3, 1, buf); // printf doesn't contain support for %f, luckily AVR has "dtostrf" in stdlib.h
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, buf);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, "  "); // 230 is min, so 2 chars needed

  // frequency
  HD44780_PCF8574_PositionXY(I2C_DISPLAY_ADDR, 11, 1);
  snprintf(buf, sizeof(buf), "%i", (int) freq);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, buf);
  HD44780_PCF8574_DrawString(I2C_DISPLAY_ADDR, "  "); // 0.0 is min, so 2 chars needed
}