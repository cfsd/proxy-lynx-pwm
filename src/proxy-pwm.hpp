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

#ifndef PWM
#define PWM

#include "opendlv-standard-message-set.hpp"

#include <memory>
#include <string>
#include <vector>
#include <utility>

class Pwm {
   private:
    //Pwm(const Pwm &) = delete;
    //Pwm(Pwm &&)      = delete;
    //Pwm &operator=(const Pwm &) = delete;
    //Pwm &operator=(Pwm &&) = delete;

   public:
    Pwm();
    ~Pwm();

   public:
    float decode(const std::string &data) noexcept;
    void callOnReceive(cluon::data::Envelope data);
    void body(cluon::OD4Session &od4);

   private:
    void setUp();
    void tearDown();

    void OpenPwm();
    void ClosePwm();
    void Reset();
    void SetEnabled(uint16_t const, bool const);
    bool GetEnabled(uint16_t const) const;
    void SetDutyCycleNs(uint16_t const, uint32_t const);
    uint32_t GetDutyCycleNs(uint16_t const) const;
    void SetPeriodNs(uint16_t const, uint32_t const);
    uint32_t GetPeriodNs(uint16_t const) const;

    bool m_debug;
    bool m_initialised;
    std::string m_path;
    std::vector<uint16_t> m_pins;
    std::vector<uint32_t> m_periodsNs;
    std::vector<uint32_t> m_dutyCyclesNs;
};

#endif

