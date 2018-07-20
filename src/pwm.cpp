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
#include "opendlv-standard-message-set.hpp"

#include "proxy-pwm.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <cmath>
#include <ctime>
#include <chrono>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{0};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("port")) || (0 == commandlineArguments.count("cid")) ) {
        std::cerr << argv[0] << " testing unit and publishes it to a running OpenDaVINCI session using the OpenDLV Standard Message Set." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --port=<udp port>--cid=<OpenDaVINCI session> [--id=<Identifier in case of multiple beaglebone units>] [--verbose]" << std::endl;
        std::cerr << "Example: " << argv[0] << " --port=8882 --cid=222 --id=1 --verbose=1" << std::endl;
        retCode = 1;
    } else {
        const uint32_t ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
        std::cout << "Micro-Service ID:" << ID << std::endl;

        // Interface to a running OpenDaVINCI session.
        Pwm pwm(VERBOSE, ID);

        //cluon::data::Envelope data;
        cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};
        

        // Interface to OxTS.
        if (VERBOSE){
            const std::string ADDR("0.0.0.0");
            const std::string PORT(commandlineArguments["port"]);
            
            cluon::UDPReceiver UdpSocket(ADDR, std::stoi(PORT),
                [&od4Session = od4, &decoder=pwm, VERBOSE, ID](std::string &&d, std::string &&/*from*/, std::chrono::system_clock::time_point &&tp) noexcept {
                
                cluon::data::TimeStamp sampleTime = cluon::time::convert(tp);
                std::time_t epoch_time = std::chrono::system_clock::to_time_t(tp);
                std::cout << "[PROXY-PWM-UDP] Time: " << std::ctime(&epoch_time) << std::endl;
                // decoder = pwm
                int16_t senderStamp = (int16_t) decoder.decode(d);
                int32_t pinState = (int32_t) round((decoder.decode(d)- ((float) senderStamp))*100000);
                senderStamp += (int16_t) ID*1000 + 300;
                // if (retVal.first) {

                opendlv::proxy::PulseWidthModulationRequest msg;
                msg.dutyCycleNs(pinState);
                od4Session.send(msg, sampleTime, senderStamp);
            });
        }

        auto onPulseWidthModulationRequest{[&pwm, &VERBOSE](cluon::data::Envelope &&envelope)
        {   
            if (!pwm.getInitialised()){
                return;
            }
            auto const pwmState = cluon::extractMessage<opendlv::proxy::PulseWidthModulationRequest>(std::move(envelope));
            uint16_t pin = envelope.senderStamp()-pwm.getSenderStampOffsetPwm();
            uint32_t dutyCycleNs = pwmState.dutyCycleNs();
            if (pin == 41 || pin == 40){
                dutyCycleNs = dutyCycleNs*5;
            }else{
	   	dutyCycleNs = dutyCycleNs/20;
	    }
            pwm.SetDutyCycleNs(pin, dutyCycleNs);

            if (VERBOSE){
                std::cout << "[PROXY-PWM-RECEIVE] Pwm duty:" << dutyCycleNs << " Pin:" << pin << std::endl;
            }
        }};
        od4.dataTrigger(opendlv::proxy::PulseWidthModulationRequest::ID(), onPulseWidthModulationRequest);

        // Just sleep as this microservice is data driven.
        using namespace std::literals::chrono_literals;
        while (od4.isRunning()) {
            std::this_thread::sleep_for(1s);
        }
    }
    return retCode;
}

        


