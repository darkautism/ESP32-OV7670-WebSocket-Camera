// Edit by Kautism<darkautism@gmail.com>

#pragma once
#include "I2C.h"
#include "I2SCamera.h"


// camera registers
#define REG_GAIN 0x00
#define REG_BLUE 0x01
#define REG_RED 0x02
#define REG_COM1 0x04
#define REG_VREF 0x03
#define REG_COM4 0x0d
#define REG_COM5 0x0e
#define REG_COM6 0x0f
#define REG_AECH 0x10
#define REG_CLKRC 0x11
#define REG_COM7 0x12
#define COM7_RGB 0x04
#define REG_COM8 0x13
#define COM8_FASTAEC 0x80  // Enable fast AGC/AEC
#define COM8_AECSTEP 0x40  // Unlimited AEC step size
#define COM8_BFILT 0x20    // Band filter enable
#define COM8_AGC 0x04      // Auto gain enable
#define COM8_AWB 0x02      // White balance enable
#define COM8_AEC 0x0
#define REG_COM9 0x14
#define REG_COM10 0x15
#define REG_COM14 0x3E
#define REG_COM11 0x3B
#define COM11_NIGHT 0x80
#define COM11_NMFR 0x60
#define COM11_HZAUTO 0x10
#define COM11_50HZ 0x08
#define COM11_EXP 0x0
#define REG_TSLB 0x3A
#define REG_RGB444 0x8C
#define REG_COM15 0x40
#define COM15_RGB565 0x10
#define COM15_R00FF 0xc0
#define REG_HSTART 0x17
#define REG_HSTOP 0x18
#define REG_HREF 0x32
#define REG_VSTART 0x19
#define REG_VSTOP 0x1A
#define REG_COM3 0x0C
#define REG_MVFP 0x1E
#define REG_COM13 0x3d
#define COM13_UVSAT 0x40
#define REG_SCALING_XSC 0x70
#define REG_SCALING_YSC 0x71
#define REG_SCALING_DCWCTR 0x72
#define REG_SCALING_PCLK_DIV 0x73
#define REG_SCALING_PCLK_DELAY 0xa2
#define REG_BD50MAX 0xa5
#define REG_BD60MAX 0xab
#define REG_AEW 0x24
#define REG_AEB 0x25
#define REG_VPT 0x26
#define REG_HAECC1 0x9f
#define REG_HAECC2 0xa0
#define REG_HAECC3 0xa6
#define REG_HAECC4 0xa7
#define REG_HAECC5 0xa8
#define REG_HAECC6 0xa9
#define REG_HAECC7 0xaa
#define REG_COM12 0x3c
#define REG_GFIX 0x69
#define REG_COM16 0x41
#define COM16_AWBGAIN 0x08
#define REG_EDGE 0x3f
#define REG_REG76 0x76
#define ADCCTR0 0x20

class OV7670 : public I2SCamera {
 public:
  enum Mode {
    QQQVGA_RGB565,
    QQVGA_RGB565,
    QVGA_RGB565,
    VGA_RGB565,
  };
  int xres, yres;

 protected:
  #define ADDR 0x42

  Mode mode;
  I2C i2c;

  void testImage();
  void saturation(int s);
  void frameControl(int hStart, int hStop, int vStart, int vStop);
  void QVGA();
  void QVGARGB565();
  void VGA();
  void VGARGB565();

  void QQVGA();
  void QQVGARGB565();
  void QQQVGA();
  void QQQVGARGB565();
  void inline writeRegister(unsigned char reg, unsigned char data) {
    i2c.writeRegister(ADDR, reg, data);
  }

 public:
  OV7670(OV7670::Mode m, const int SIOD, const int SIOC, const int VSYNC,
         const int HREF, const int XCLK, const int PCLK, const int D0,
         const int D1, const int D2, const int D3, const int D4, const int D5,
         const int D6, const int D7);
};
