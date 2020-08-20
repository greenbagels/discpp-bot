/*! \file bot.hpp
 *  \brief Core bot functionality header
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

#ifndef BOT_HPP
#define BOT_HPP

#include <boost/optional.hpp>
#include <boost/json.hpp>
#include <gateway.hpp>
#include "trigger.hpp"

namespace discpp_bot
{
    class bot
    {
        public:
            bot& start();
            bot& stop();
            bot& connect();
            bot& add_trigger(std::string id, std::string trigger, std::function<boost::json::value(std::string)> fn);
            bot& register_callback(std::function<void(discpp::gateway::payload)> fun);
            bot& set_token(std::string);
            std::string get_token() const;

        private:
            void main_loop();
            void heartbeat_loop();
            void parse_message(discpp::gateway::message &message);

            // We let the user shadow the default callbacks via derived classes
            void dispatch(boost::json::object, boost::json::string);
            void heartbeat(boost::json::object);
            void identify(boost::json::object);
            void presence_update(boost::json::object);
            void voice_state_update(boost::json::object);
            void unused(boost::json::object);
            void resume(boost::json::object);
            void reconnect(boost::json::object);
            void request_guild_members(boost::json::object);
            void invalid_session(boost::json::object);
            void hello(boost::json::object);
            void heartbeat_ack();

            // TODO: Consider bumping to C++17 to allow std::hdis/hcis

            std::string token;

            /*! Tracks whether user asked us to stop */
            std::atomic_bool keep_going;
            /*! Tracks latest received message */
            std::atomic_int sequence{-1};

            // Heartbeat-related members
            /*! The thread of execution for sending keepalive heartbeats */
            std::thread heartbeat_thread;
            /*! The amount of time, in milliseconds, to wait between heartbeats */
            std::chrono::milliseconds heartbeat_interval;
            /*! Whether the server requested a heartbeat from us */
            std::atomic_bool heartbeat_requested;
            /*! Whether we received a heartbeat ACK */
            std::atomic_bool heartbeat_received;
            /*! Our condition variable for timing heartbeat intervals */
            std::condition_variable cv_hb;
            /*! Our mutex for timing heartbeat intervals */
            std::mutex mutex_hb;

            /*! The main thread of execution */
            std::thread main_thread;
            std::thread core_thread;

            /*! The underlying context for the connection */
            discpp::context context;
            /*! Our gateway websocket connection */
            std::shared_ptr<discpp::gateway::connection> connection;
            /*! A list of user-supplied trigger phrases */
            std::vector<trigger::trigger_function> trigger_list;
    };
}

#endif
