/*! \file trigger.cpp
 *  \brief Trigger function class implementation
 */

/*  This file is part of discpp-bot.
 *
 *  discpp-bot is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  discpp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with discpp-bot. If not, see <https://www.gnu.org/licenses/>.
 */

#include "trigger.hpp"

namespace discpp_bot
{
    namespace trigger
    {
        trigger_function::trigger_function(std::string identifier, std::string trig,
                                           std::function<boost::json::value(std::string)> fn) :
                                           id(identifier), trigger(trig), trigger_fn(fn)
        {
        }

        bool trigger_function::matches_trigger(std::string operand)
        {
            return !operand.find(trigger);
        }

        boost::json::value trigger_function::run(std::string arg1)
        {
            return trigger_fn(arg1);
        }

        boost::json::value default_info(std::string arg1)
        {
            std::array<char, 32> buf;
            std::string uname, uptime;
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen("uname -rsv", "r"), pclose);

            if (!pipe)
                throw std::runtime_error("popen() failed!");

            while (std::fgets(buf.data(), buf.size(), pipe.get()) != nullptr)
                uname += buf.data();

            buf.fill(0);
            pipe = std::unique_ptr<FILE, decltype(&pclose)>(popen("uptime -p", "r"), pclose);

            if (!pipe)
                throw std::runtime_error("popen() failed!");

            while (std::fgets(buf.data(), buf.size(), pipe.get()) != nullptr)
                uname += buf.data();

            boost::json::object resp
            ({
                {"content", boost::json::string(("**System info:**\n" + uname + uptime).c_str())},
                {"tts", false}
            });

            return boost::json::value(resp);
        }
    }
}
