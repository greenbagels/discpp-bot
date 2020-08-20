/*! \file bot.cpp
 *  \brief Core bot functionality implementation
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

#include "bot.hpp"
#include <http.hpp>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#define BOOST_LOG_DYN_LINK 1
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
namespace discpp_bot
{

    bot& bot::start()
    {
        std::cerr << "Connecting to the gateway\n";
        this->connect();
        core_thread = std::thread(&discpp::gateway::connection::main_loop, this->connection);
        keep_going = true;
        std::cerr << "Launching main thread\n";
        main_thread = std::thread(&bot::main_loop, this);
        return *this;
    }

    bot& bot::stop()
    {
        keep_going = false;
        return *this;
    }

    bot& bot::connect()
    {
        std::string gateway_url = discpp::http::get_gateway(context);
        std::cerr << "Obtained gateway url " << gateway_url << std::endl;
        std::cerr << "Constructing gateway connection now!\n";
        this->connection = std::make_shared<discpp::gateway::connection>(context, gateway_url);
        return *this;
    }

    void bot::main_loop()
    {
        while (keep_going)
        {
            std::cerr << "Waiting for read queue to fill...\n";
            connection->read_queue.wait_until_nonempty();

            discpp::gateway::message msg;
            *connection >> msg;

            std::cerr << "Parsing message now!\n";
            parse_message(msg);
        }
        // TODO: shutdown the connection
    }

    void bot::parse_message(discpp::gateway::message &msg)
    {
        using namespace discpp::gateway;
        auto val = std::get<0>(msg).as_object();

        auto op = val["op"]; //.as_int64();
        auto d = val["d"]; //.as_object();

        // TODO: verification of non-null entries
        switch(static_cast<discpp::gateway::opcode>(op.as_int64()))
        {
            case opcode::dispatch:
            {
                sequence = val["s"].as_int64();
                auto t = val["t"].as_string();
                dispatch(d.as_object(), val["t"].as_string());
            }
                break;
            case opcode::heartbeat:
                heartbeat(d.as_object());
                break;
            case opcode::identify:
                identify(d.as_object());
                break;
            case opcode::presence_update:
                presence_update(d.as_object());
                break;
            case opcode::voice_state_update:
                voice_state_update(d.as_object());
                break;
            case opcode::resume:
                resume(d.as_object());
                break;
            case opcode::reconnect:
                reconnect(d.as_object());
                break;
            case opcode::request_guild_members:
                request_guild_members(d.as_object());
                break;
            case opcode::invalid_session:
                invalid_session(d.as_object());
                break;
            case opcode::hello:
                hello(d.as_object());
                break;
            case opcode::heartbeat_ack:
                heartbeat_ack();
                break;
            case opcode::unused:
            default:
                break;
        }
    }

    bot& bot::add_trigger(std::string id, std::string trigger, std::function<boost::json::value(std::string)> fun)
    {
        trigger_list.push_back(trigger::trigger_function(id, trigger, fun));
        return *this;
    }

    bot& bot::set_token(std::string token)
    {
        this->token = token;
        return *this;
    }

    std::string bot::get_token() const
    {
        return token;
    }

    void bot::dispatch(boost::json::object d, boost::json::string t)
    {
        using namespace std::chrono;

        if (t == boost::json::string("READY"))
        {
            std::cerr << "Received event ready!" << std::endl;
        }

        else if (t == boost::json::string("MESSAGE_CREATE"))
        {
            std::cerr << "Received event message create!" << std::endl;
            std::string channel_id = std::string(d["channel_id"].as_string().c_str());
            for (auto i = trigger_list.begin(); i != trigger_list.end(); i++)
            {
                auto content = std::string(d["content"].as_string().c_str());
                if (i->matches_trigger(content))
                {
                    std::cerr << "Trigger matches!\n";
                    auto response = std::string(boost::json::to_string(i->run(content)).c_str());
                    discpp::http::http_post(context, "discord.com", "/api/channels/" +
                            channel_id + "/messages", get_token(), response);
                }
            }
        }
    }

    void bot::heartbeat(boost::json::object d)
    {
        std::lock_guard<std::mutex> lg(mutex_hb);
        heartbeat_requested = true;
    }

    void bot::identify(boost::json::object d)
    {
    }

    void bot::presence_update(boost::json::object d)
    {
    }

    void bot::voice_state_update(boost::json::object d)
    {
    }

    void bot::unused(boost::json::object d)
    {
    }

    void bot::resume(boost::json::object d)
    {
    }

    void bot::reconnect(boost::json::object d)
    {
    }

    void bot::request_guild_members(boost::json::object d)
    {
    }

    void bot::invalid_session(boost::json::object d)
    {
    }

    void bot::hello(boost::json::object d)
    {
        using namespace std::chrono;
        auto heartbeat_interval = d["heartbeat_interval"].as_int64();
        heartbeat_requested = true;
        heartbeat_received = true;
        this->heartbeat_interval = milliseconds(heartbeat_interval);
        this->heartbeat_thread = std::thread(&bot::heartbeat_loop, this);

        // Now we send an opcode identify:
        boost::json::object identify_payload
        ({
            {"op", static_cast<std::uint64_t>(discpp::gateway::opcode::identify)},
            {"d", boost::json::object
                  ({
                      {"token", boost::json::string(token.c_str())},
                      {"properties", boost::json::object
                                     ({
                                         {"$os", "linux"},
                                         {"$browser", "discpp 0.1"},
                                         {"$device", "discpp 0.1"}
                                     })}
                  })}
        });
        std::cerr << "Pushing onto write queue...\n";

        connection->write_queue.push(
            discpp::gateway::message(
                boost::json::value(identify_payload),
                boost::optional<time_point<steady_clock>>(steady_clock::now())
            )
        );
        std::cerr << "Write queue has been pushed onto. Now waiting...\n";

    }

    void bot::heartbeat_ack()
    {
        heartbeat_received = true;
    }

    void bot::heartbeat_loop()
    {
        using namespace std::chrono;
        while (keep_going)
        {
            auto hb_deadline = steady_clock::now() + heartbeat_interval;
            std::unique_lock<std::mutex> ulock(mutex_hb);

            cv_hb.wait_until
            (
                ulock,
                hb_deadline,
                [&]()
                {
                    return (steady_clock::now() >= hb_deadline) || heartbeat_requested;
                }
            );

            // TODO: make this more consistent
            if (!heartbeat_received)
                throw std::runtime_error("Zombie connection found in heartbeat loop!");

            // TODO: optional, optimize
            boost::json::object hb({{"op", static_cast<std::uint64_t>(discpp::gateway::opcode::heartbeat)}});
            if (sequence.load() == -1)
            {
                hb.emplace("d", nullptr);
            }
            else
            {
                hb.emplace("d", sequence.load());
            }

            connection->write_queue.push(
                discpp::gateway::message(
                    boost::json::value(hb),
                    boost::optional<time_point<steady_clock>>(steady_clock::now())
                )
            );

            heartbeat_requested = false;
            heartbeat_received = false;
        }
    }
} // namespace discpp

