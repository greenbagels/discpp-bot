/*! \file trigger.hpp
 *  \brief Custom response function header
 */

/*  This file is part of discpp-bot.
 *
 *  discpp-bot is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  discpp-bot is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with discpp-bot. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TRIGGER_HPP
#define TRIGGER_HPP

#include <functional>
#include <string>
#include <boost/json.hpp>

namespace discpp_bot
{
    namespace trigger
    {
        class trigger_function
        {
            public:
                trigger_function(std::string, std::string, std::function<boost::json::value(std::string)>);
                bool matches_trigger(std::string operand);
                boost::json::value run(std::string);
            private:
                std::string id;
                std::string trigger;
                std::function<boost::json::value(std::string)> trigger_fn;
        };

        boost::json::value default_info(std::string arg1);
    }
}

#endif
