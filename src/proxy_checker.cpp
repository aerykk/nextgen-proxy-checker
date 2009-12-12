#define FD_SETSIZE 65535//4096

#include "common.h"
#include "proxy_checker.h"

class address
{
    union v4
    {
        int value;

        struct
        {
            uint8_t first;
            uint8_t second;
            uint8_t third;
            uint8_t forth;
        };
    };

    public: v4 value;
    public: bool valid;

    public: bool is_valid()
    {
        return this->valid;
    }

    public: void from_string(std::string const& s)
    {
        boost::regex_error paren(boost::regex_constants::error_paren);

        try
        {
            boost::match_results<std::string::const_iterator> what;
            boost::match_flag_type flags = boost::regex_constants::match_perl | boost::regex_constants::format_perl;

            std::string::const_iterator start = s.begin();
            std::string::const_iterator end = s.end();

            if(boost::regex_search(start, end, what, boost::regex("^([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$"), flags))
            {
                auto first = boost::lexical_cast<uint32_t>(what[1]);
                auto second = boost::lexical_cast<uint32_t>(what[2]);
                auto third = boost::lexical_cast<uint32_t>(what[3]);
                auto forth = boost::lexical_cast<uint32_t>(what[4]);

                if(!(first >= 0 && first <= 255
                && second >= 0 && second <= 255
                && third >= 0 && third <= 255
                && forth >= 0 && forth <= 255))
                {
                    this->valid = false;

                    return;
                }

                this->value.first = boost::numeric_cast<uint8_t>(first);
                this->value.second = boost::numeric_cast<uint8_t>(second);
                this->value.third = boost::numeric_cast<uint8_t>(third);
                this->value.forth = boost::numeric_cast<uint8_t>(forth);
            }
            else
            {
                this->valid = false;

                return;
            }
        }
        catch(boost::regex_error const& e)
        {
            std::cout << "regex error: " << (e.code() == paren.code() ? "unbalanced parentheses" : "?") << std::endl;
        }

        //std::vector<std::string> subs;

        //boost::split(subs, s, boost::is_any_of("."));

    }

    public: address(uint32_t address) : valid(true)
    {
        this->value.value = address;
    }

    public: address(std::string const& address) : valid(true)
    {
        this->from_string(address);
    }
};

class address_range
{
    public: address lower;
    public: address upper;

    public: bool is_within_range(address& a)
    {
        if(a.value.first <= this->upper.value.first && a.value.first >= this->lower.value.first
        && a.value.second <= this->upper.value.second && a.value.second >= this->lower.value.second
        && a.value.third <= this->upper.value.third && a.value.third >= this->lower.value.third
        && a.value.forth <= this->upper.value.forth && a.value.forth >= this->lower.value.forth)
            return true;

        return false;
    }

    public: address_range(address&& lower, address&& upper) : lower(lower), upper(upper)
    {

    }

    public: address_range(std::string const& lower, std::string const& upper) : lower(lower), upper(upper)
    {

    }
};


namespace proxos
{
	class application : public nextgen::singleton<application>
	{
	    public: typedef nextgen::network::service network_service_type;

		public: void initialize()
		{
		    auto self = *this;

        }

        public: void check_proxy(proxy proxy, std::function<void()> callback = 0) const
        {
            auto self = *this;

            std::cout << proxy->host << ":" << proxy->port << std::endl;

            address a(proxy->host);

            if(!a.is_valid())
            {
                std::cout << "INVALID PROXY ####################################" << std::endl;

                proxy.set_check_delay("0001-00-00 00:00:00");

                std::string query = "UPDATE proxies SET proxy_type = \"invalid\", proxy_latency = 0, proxy_last_checked = NOW(), proxy_check_delay = \"" + proxy.get_check_delay() + "\" WHERE proxy_id = " + to_string(proxy.get_id()) + " LIMIT 1";

                self->proxy_database.query(query);

                if(callback != 0)
                    callback();

                return;
            }

            if(self.proxy_is_banned(a))
            // dont check proxy for another month
            {
                std::cout << "BANNED PROXY ####################################" << std::endl;

                proxy.set_check_delay("0000-01-00 00:00:00");

                std::string query = "UPDATE proxies SET proxy_type = \"banned\", proxy_latency = 0, proxy_last_checked = NOW(), proxy_check_delay = \"" + proxy.get_check_delay() + "\" WHERE proxy_id = " + to_string(proxy.get_id()) + " LIMIT 1";

                self->proxy_database.query(query);

                if(callback != 0)
                    callback();

                return;
            }



            self->proxy_checker.check_proxy(proxy, [=]()
            {
                if(proxy.get_type() == "dead")
                {
                    proxy.set_check_delay("0000-00-01 00:00:00");
                    proxy->rating -= 1;
                }
                else if(proxy.get_type() == "broken")
                {
                    proxy.set_check_delay("0000-00-01 00:00:00");
                    proxy->rating -= 1;
                }
                else if(proxy.get_type() == "transparent")
                {
                    proxy.set_check_delay("0000-00-00 05:00:00");
                    proxy->rating += 1;
                }
                else if(proxy.get_type() == "anonymous")
                {
                    proxy.set_check_delay("0000-00-00 05:00:00");
                    proxy->rating += 1;
                }
                else if(proxy.get_type() == "elite")
                {
                    proxy.set_check_delay("0000-00-00 05:00:00");
                    proxy->rating += 1;
                }
                   else if(proxy.get_type() == "socks4")
                {
                    proxy.set_check_delay("0000-00-00 05:00:00");
                    proxy->rating += 1;
                }
                else if(proxy.get_type() == "socks5")
                {
                    proxy.set_check_delay("0000-00-00 05:00:00");
                    proxy->rating += 1;
                }
                else if(proxy.get_type() == "codeen")
                {
                    proxy.set_check_delay("0000-00-07 00:00:00");
                }
                // todo(daemn) check mysql table check table proxies.proxies; for status OK
                {
                    std::string query = "UPDATE proxies SET proxy_type = \"" + proxy.get_type() + "\", proxy_latency = " + to_string(proxy->latency) + ", proxy_state = " + to_string(proxy->state) + ", proxy_rating = " + to_string(proxy->rating) + ", proxy_last_checked = NOW(), proxy_check_delay = \"" + proxy.get_check_delay() + "\" WHERE proxy_id = " + to_string(proxy.get_id()) + " LIMIT 1";

                    std::cout << query << " after " << to_string(proxy.get_latency()) << " seconds. " << std::endl;

                    std::cout << "state: " << proxy.get_state() << std::endl;

                    self->proxy_database.query(query);
                }

                if(callback != 0)
                    callback();
            });
        }

        public: bool proxy_is_banned(address& a) const
        {
            auto self = *this;

            for(auto i = self->banlist.begin(), l = self->banlist.end(); i != l; ++i)
            {
                if((*i).is_within_range(a))
                    return true;
            }

            return false;
        }

		public: void run();

        private: struct variables
        {
            variables() : proxy_checker("www.proxyprobe.com", 8080, network_service)
            {

            }

            ~variables()
            {

            }

            network_service_type network_service;
            nextgen::database::link proxy_database;
            proxos::proxy_checker proxy_checker;
            std::vector<address_range> banlist;
        };

        NEXTGEN_SHARED_DATA(application, variables);
	};
}


void proxos::application::run()
{
    auto self = *this;

    self->proxy_database.connect("localhost", "root", "swoosh", "proxies");

    std::string query("SELECT * FROM banlist"); //ORDER BY proxy_rating DESC

    std::cout << query << std::endl;

    auto list = *self->proxy_database.get_row_list(query);

    // fill the banlist
    std::for_each(list.begin(), list.end(), [=](nextgen::database::row& row)
    {
        auto host = (*row)["ban_host"];
        auto description = (*row)["ban_description"];

        std::vector<std::string> ip;

        boost::split(ip, host, boost::is_any_of("-"));

        address_range s(ip[0], ip[1]);

        self->banlist.push_back(s);
    });

    auto refill = [=](size_t amount)
    {
        std::cout << "Attempting to load " << amount << " proxies..." << std::endl;

        static uint32_t start = 0;
        static nextgen::timer timer;

        if(start > 0 || timer.stop() > 5)
        // load proxies if we're in the middle of processing the list or checking every 5 seconds for more
        {
            std::string query("SELECT proxy_host, proxy_port, proxy_id, proxy_rating FROM proxies WHERE proxy_last_checked < (NOW() - proxy_check_delay) ORDER BY proxy_id LIMIT " + to_string(start) + ", " + to_string(amount)); //ORDER BY proxy_rating DESC

            std::cout << query << std::endl;

            auto list = *self->proxy_database.get_row_list(query);

            std::for_each(list.begin(), list.end(), [=](nextgen::database::row& row)
            {
                std::cout << (*row)["proxy_host"] << " " << (*row)["proxy_port"] << " " << (*row)["proxy_rating"] << std::endl;
                proxy proxy((*row)["proxy_host"], to_int((*row)["proxy_port"]), to_int((*row)["proxy_id"]));
                proxy->rating = to_int((*row)["proxy_rating"]);

                // check the proxy against banlist
                self.check_proxy(proxy);
            });

            start += amount;

            if(list.size() < amount)
            // we've hit the end of the proxy list, loop back around and check for changes
            {
                start = 0;
                amount = list.size();
            }

            std::cout << "Loaded " << amount << " proxies." << std::endl;
            //proxy_checker.add_list(); //proxy_rating DESC, proxy_hits DESC, proxy_latency ASC,
        }
    };

    self->proxy_checker->refill_event += refill;

    nextgen::timer timer;

    while(true)
    {
        //try
        //{

            if(timer.stop() > 1)
            {
                timer.start();

                std::cout << "[proxos:application:run] Updating services..." << std::endl;
                std::cout << "C" << self->proxy_checker->job_list.size() << std::endl;
                std::cout << "D" << self->proxy_checker->server->client_list.size() << std::endl;
                //std::cout << "e" << proxy_checker->client_count << std::endl;
            }

            self->proxy_checker.update();

            self->network_service.update();

            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        //}
        //catch(boost::exception& e)
        //{
        //    std::cout << "[proxos:application:run] " << "Unexpected exception caught in " << BOOST_CURRENT_FUNCTION << std::endl << boost::current_exception_diagnostic_information();
        //}
    }
}

int main()
{
    proxos::application::instance().run();
}
