/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"
#include "proxy-pwm.hpp"

// #include <cmath>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include <ctime>
#include <chrono>

float Pwm::decode(const std::string &data) noexcept {
    std::cout << "[PROXY-PWM-UDP] Got data:" << data << std::endl;
    float temp = std::stof(data);
    return temp;
}

Pwm::Pwm(bool verbose, uint32_t id)
    : m_debug(verbose)
    , m_bbbId(id)
    , m_senderStampOffsetPwm(id*1000+300)
    , m_initialised()
    , m_path()
    , m_pins()
    , m_periodsNs()
    , m_dutyCyclesNs()
{
	Pwm::setUp();
}

Pwm::~Pwm() 
{
  Pwm::tearDown();
}

void Pwm::setUp()
{

  m_path = "/sys/class/pwm/pwmchip";
  std::vector<std::string> pinsVector = {"00","20","21","40","41"};
  std::vector<std::string> periodNsStringVector = {"50000000","50000000","50000000","50000","50000"};
  std::vector<std::string> dutyCycleNsStringVector = {"0","0","0","0","0"};// Duty cycle of the direction PWM channel in nano seconds

  if (pinsVector.size() == periodNsStringVector.size() 
      && pinsVector.size() == dutyCycleNsStringVector.size()) {
    for (uint32_t i = 0; i < pinsVector.size(); i++) {
      uint16_t pin = std::stoi(pinsVector.at(i));
      int32_t periodNs = std::stoi(periodNsStringVector.at(i));
      int32_t dutyCycleNs = std::stoi(dutyCycleNsStringVector.at(i));
      m_pins.push_back(pin);
      m_periodsNs.push_back(periodNs);
      m_dutyCyclesNs.push_back(dutyCycleNs);
    }
    std::cout << "[PROXY-PWM] " << "Initialised pins:";
    for (uint32_t i = 0; i < pinsVector.size(); i++) {
      std::cout << "|Pin " << m_pins.at(i) << " Period " << m_periodsNs.at(i) 
          << " Duty cycle" << m_dutyCyclesNs.at(i);
    }
    std::cout << "." << std::endl;
  
  } else {
    std::cerr << "[PROXY-PWM] Number of pins do not equals to number of periods or duty cycles." 
        << std::endl;
  }

  OpenPwm();

  m_initialised = true;
}

void Pwm::tearDown()
{
//  ClosePwm();
}

void Pwm::OpenPwm()
{
  for (auto pinAndChip : m_pins) {

    int16_t pin = pinAndChip%10;
    std::string chip = std::to_string(pinAndChip/10);
    std::string filename = m_path + chip + "/export";
    std::ofstream exportFile(filename, std::ofstream::out);
    
    if (exportFile.is_open()) {
        exportFile << pin;
        exportFile.flush();
    } else {
      std::cout << "[PROXY-PWM] Could not open " << filename << "." 
          << std::endl;
    }
    exportFile.close();
  }
  Reset();
}

void Pwm::ClosePwm()
{
  for (auto pinAndChip : m_pins) {

    int16_t pin = pinAndChip%10;
    std::string chip = std::to_string(pinAndChip/10);
    std::string filename = m_path + chip + "/unexport";
    std::ofstream unexportFile(filename, std::ofstream::out);
    
    if (unexportFile.is_open()) {
        SetEnabled(pin, 0);
        unexportFile << pin;
        unexportFile.flush();
    } else {
      std::cout << "[PROXY-PWM] Could not open " << filename << "." 
          << std::endl;
    }
    unexportFile.close();
  }
}

void Pwm::Reset()
{
  for (uint8_t i = 0; i < m_pins.size(); i++) {
    SetEnabled(m_pins.at(i), false);
    SetPeriodNs(m_pins.at(i), m_periodsNs.at(i));
    SetDutyCycleNs(m_pins.at(i), m_dutyCyclesNs.at(i));
    SetEnabled(m_pins.at(i), true);
  }
}

void Pwm::SetEnabled(uint16_t const a_pin, bool const a_value)
{
  int16_t pin = a_pin%10;
  std::string chip = std::to_string(a_pin/10);
  std::string filename = m_path + chip + "/pwm" + std::to_string(pin) + "/enable";
  std::ofstream file(filename, std::ofstream::out);
  if (file.is_open()) {
    file << std::to_string((static_cast<int32_t>(a_value)));
    file.flush();
  } else {
    std::cerr << "[PROXY-PWM] Could not open " << filename 
        << "." << std::endl;
  }
  file.close();
}

bool Pwm::GetEnabled(uint16_t const a_pin) const
{
    int16_t pin = a_pin%10;
  std::string chip = std::to_string(a_pin/10);
  std::string filename = m_path + chip + "/pwm" + std::to_string(pin) + "/enable";
  std::string line;

  std::ifstream file(filename, std::ifstream::in);
  if (file.is_open()) {
    std::getline(file, line);
    bool value = (line.compare("1") == 0);
    file.close();
    return value;
  } else {
    std::cerr << "[PROXY-PWM] Could not open " << filename 
        << "." << std::endl;
    file.close();
    return NULL;
  }
}

void Pwm::SetDutyCycleNs(uint16_t const a_pin, uint32_t const a_value)
{
  int16_t pin = a_pin%10;
  std::string chip = std::to_string(a_pin/10);
  std::string filename = m_path + chip + "/pwm" + std::to_string(pin) + "/duty_cycle";

  std::ofstream file(filename, std::ofstream::out);
  if (file.is_open()) {
    file << std::to_string(a_value);
    file.flush();
  } else {
    std::cerr << "[PROXY-PWM] Could not open " << filename 
        << "." << std::endl;
  }

  file.close();
}

uint32_t Pwm::GetDutyCycleNs(uint16_t const a_pin) const
{
  int16_t pin = a_pin%10;
  std::string chip = std::to_string(a_pin/10);
  std::string filename = m_path + chip + "/pwm" + std::to_string(pin) + "/duty_cycle";
  std::string line;

  std::ifstream file(filename, std::ifstream::in);
  if (file.is_open()) {
    std::getline(file, line);
    uint32_t value = std::stoi(line);
    file.close();
    return value;
  } else {
    std::cerr << "[PROXY-PWM] Could not open " << filename 
        << "." << std::endl;
    file.close();
    return 0;
  }
}

void Pwm::SetPeriodNs(uint16_t const a_pin, uint32_t const a_value)
{
  int16_t pin = a_pin%10;
  std::string chip = std::to_string(a_pin/10);
  std::string filename = m_path + chip + "/pwm" + std::to_string(pin) + "/period";

  std::ofstream file(filename, std::ofstream::out);
  if (file.is_open()) {
    file << std::to_string(a_value);
    file.flush();
  } else {
    std::cerr << "[PROXY-PWM] Could not open " << filename 
        << "." << std::endl;
  }
  file.close();
}

uint32_t Pwm::GetPeriodNs(uint16_t const a_pin) const
{
  int16_t pin = a_pin%10;
  std::string chip = std::to_string(a_pin/10);
  std::string filename = m_path + chip + "/pwm" + std::to_string(pin) + "/period";
  std::string line;

  std::ifstream file(filename, std::ifstream::in);
  if (file.is_open()) {
    std::getline(file, line);
    uint32_t value = std::stoi(line);
    file.close();
    return value;
  } else {
    std::cerr << "[PROXY-PWM] Could not open " << filename 
        << "." << std::endl;
    file.close();
    return 0;
  }
}

uint32_t Pwm::getSenderStampOffsetPwm(){
  return m_senderStampOffsetPwm;
}
bool Pwm::getInitialised(){
  return m_initialised;
}
