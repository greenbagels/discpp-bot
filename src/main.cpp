
#include <iostream>
#include <fstream>
#include "bot.hpp"
int main()
{
    std::cerr << "Initializing bot\n";
    discpp_bot::bot bot;
    std::cerr << "Parsing token\n";
    std::ifstream ifstream("token");
    std::string token;
    ifstream >> token;
    std::cerr << "Setting bot token to " << token << std::endl;
    bot.set_token(token);
    bot.add_trigger("info",
                    "!info",
                    std::function<boost::json::value(std::string)>(discpp_bot::trigger::default_info));
    std::cerr << "Starting bot up\n";
    bot.start();
    std::cin.get();
    std::cin.ignore();
    return 0;
}
