/**
 * @file Nanoshield_Termopar.cpp
 * This is the library to access the Termopar Nanoshield version 2
 * 
 * Copyright (c) 2015 Circuitar
 * This software is released under the MIT license. See the attached LICENSE file for details.
 */

#include "Nanoshield_Termopar.h"

// MAX31856 registers
#define MAX31856_REG_CR0    0x00
#define MAX31856_REG_CR1    0x01
#define MAX31856_REG_MASK   0x02
#define MAX31856_REG_CJHF   0x03
#define MAX31856_REG_CJLF   0x04
#define MAX31856_REG_LTHFTH 0x05
#define MAX31856_REG_LTHFTL 0x06
#define MAX31856_REG_LTLFTH 0x07
#define MAX31856_REG_LTLFTL 0x08
#define MAX31856_REG_CJTO   0x09
#define MAX31856_REG_CJTH   0x0A
#define MAX31856_REG_CJTL   0x0B
#define MAX31856_REG_LTCBH  0x0C
#define MAX31856_REG_LTCBM  0x0D
#define MAX31856_REG_LTCBL  0x0E
#define MAX31856_REG_SR     0x0F

// MAX31856 register read/write masks
#define MAX31856_REG_READ   0x00
#define MAX31856_REG_WRITE  0x80

SPISettings Nanoshield_Termopar::spiSettings = SPISettings(1000000, MSBFIRST, SPI_MODE1);

Nanoshield_Termopar::Nanoshield_Termopar(uint8_t cs, TcType type, TcAveraging avg) {
  this->cs = cs;
  this->type = type;
  this->avg = avg;
	this->internal = 0;
	this->external = 0;
	this->fault = 0;
}

void Nanoshield_Termopar::begin() {
  pinMode(cs, OUTPUT);
  digitalWrite(cs, HIGH);
  SPI.begin();

  // Initialize MAX31856
  SPI.beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  SPI.transfer(MAX31856_REG_CR0 | MAX31856_REG_WRITE);
  SPI.transfer(0x90); // Setup CR0 register:
                      //   Automatic conversion mode
                      //   Enable fault detection mode 1 (OCFAULT)
                      //   Cold juntion sensor enabled
                      //   Fault detection in comparator mode
                      //   Noise rejection = 60Hz
  SPI.transfer(((uint8_t)avg << 4) | ((uint8_t)type & 0x0F)); // Setup CR1 register:
                                                              //   Setup selected averaging mode
                                                              //   Setup selected thermocouple type
  SPI.transfer(0x03); // Setup MASK register:
                      //   Enable overvoltage/undervoltage detection
                      //   Enable open circuit detection
  digitalWrite(cs, HIGH);
  SPI.endTransaction();
}

void Nanoshield_Termopar::read() {
	uint16_t cj = 0;
	uint32_t ltc = 0;

  SPI.beginTransaction(spiSettings);
  digitalWrite(cs, LOW);
  SPI.transfer(MAX31856_REG_CJTH | MAX31856_REG_READ);
  cj |= (uint16_t)SPI.transfer(0) << 8;
  cj |= SPI.transfer(0);
  ltc |= (uint32_t)SPI.transfer(0) << 24;
  ltc |= (uint32_t)SPI.transfer(0) << 16;
  ltc |= (uint32_t)SPI.transfer(0) << 8;
	fault = SPI.transfer(0);
  digitalWrite(cs, HIGH);
  SPI.endTransaction();

  internal = (cj / 4) * 0.015625;
  external = (ltc / 8192) * 0.0078125;
}

double Nanoshield_Termopar::getInternal() {
	return internal;
}

double Nanoshield_Termopar::getExternal() {
	return external;
}

bool Nanoshield_Termopar::isExternalOutOfRange() {
	return (fault & 0x40) != 0;
}

bool Nanoshield_Termopar::isInternalOutOfRange() {
	return (fault & 0x80) != 0;
}

bool Nanoshield_Termopar::isOverUnderVoltage() {
	return (fault & 0x02) != 0;
}

bool Nanoshield_Termopar::isOpen() {
	return (fault & 0x01) != 0;
}

bool Nanoshield_Termopar::hasError() {
	return (fault & 0xC3) != 0;
}
