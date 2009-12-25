#ifndef PROXOS_YOUTUBE_VIDEO
#define PROXOS_YOUTUBE_VIDEO

#include "common.h"
#include "proxy_checker.h"

bool YOUTUBE_DEBUG_1 = 1;

namespace youtube
{
    class video
    {
        private: struct variables
        {
            variables()
            {

            }

            ~variables()
            {

            }

            std::string id;
        };

        NEXTGEN_SHARED_DATA(video, variables);
    };


    class client
    {
        public: void view_video(video v, size_t view_max) const
        {
            auto self = *this;

            size_t view_count = 0;

            if(self->agent == 0)
                self->agent = proxos::agent("Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9.1.5) Gecko/20091109 Ubuntu/9.10 (karmic) Firefox/3.5.5");

            if(YOUTUBE_DEBUG_1)
                std::cout << "user agent: " << self->agent->title << std::endl;

            std::string host;
            uint32_t port;

            if(self->proxy != 0)
            {
                host = self->proxy->host;
                port = self->proxy->port;

                switch(self->proxy->type)
                {
                    case proxos::proxy::types::transparent: self->client->proxy = "http"; break;
                    case proxos::proxy::types::distorting: self->client->proxy = "http"; break;
                    case proxos::proxy::types::anonymous: self->client->proxy = "http"; break;
                    case proxos::proxy::types::elite: self->client->proxy = "http"; break;
                    case proxos::proxy::types::socks4: self->client->proxy = "socks4"; break;
                    case proxos::proxy::types::socks5: self->client->proxy = "socks5"; break;
                    case proxos::proxy::types::socks4n5: self->client->proxy = "socks4"; break;
                    default: std::cout << "WTF" << std::endl;
                }
            }
            else
            {
                host = "youtube.com";
                port = 80;
            }

            self->client.connect("youtube.com", 80, nextgen::network::ipv4_address(host, port), [=]()
            {
                if(YOUTUBE_DEBUG_1)
                    std::cout << "[proxos:youtube] Connected." << std::endl;

                nextgen::network::http_message m1;

                m1->method = "GET";
                m1->url = "http://www.youtube.com/watch?v=" + v->id;
                m1->header_list["Host"] = "www.youtube.com";
                m1->header_list["User-Agent"] = self->agent->title;
                m1->header_list["Keep-Alive"] = "300";
                m1->header_list["Connection"] = "keep-alive";

                self->client.send_and_receive(m1, [=](nextgen::network::http_message r1)
                {
                    if(YOUTUBE_DEBUG_1)
                        std::cout << "[proxos:youtube] Received video page response." << std::endl;

                    if(YOUTUBE_DEBUG_1)
                        std::cout << "c_length " << r1->content.length() << std::endl;

                        std::cout << "c_length " << r1->content << std::endl;

                    if(r1->status_code != 200
                    || r1->header_list["set-cookie"].find("youtube.com") == std::string::npos)
                    {
                        if(YOUTUBE_DEBUG_1)
                            std::cout << "[proxos:youtube] Error receiving video page. " << r1->status_code << std::endl;

                        return;
                    }
/*
                    nextgen::network::http_message m2;

                    m2->method = "GET";
                    m2->url = "http://www.youtube.com/get_video_info?video_id=" + v->id;
                    m2->header_list["Host"] = "www.youtube.com";
                    m2->header_list["User-Agent"] = self->agent->title;
                    m2->header_list["Keep-Alive"] = "300";
                    m2->header_list["Connection"] = "keep-alive";
                    m2->header_list["Cookie"] = r1->header_list["set-cookie"];

                    if(YOUTUBE_DEBUG_1)
                        std::cout << "COOKIES: " << r1->header_list["set-cookie"] << std::endl;

                    auto video_info_request = [=](nextgen::network::http_message r2)
                    {
                        if(YOUTUBE_DEBUG_1)
                            std::cout << "[proxos:youtube] Received video info response." << std::endl;

                        if(YOUTUBE_DEBUG_1)
                            std::cout << r2->content << std::endl;

                        if(r2->status_code != 200
                        || r2->content.find("thumbnail_url") == std::string::npos)
                        {
                            if(YOUTUBE_DEBUG_1)
                                std::cout << "[proxos:youtube] Error receiving video info. " << r2->status_code << std::endl;

                            return;
                        }*/

                        nextgen::string token = nextgen::regex_single_match("\"t\"\\: \"(.+?)\"\\,", r1->content);
                        //nextgen::string token = nextgen::regex_single_match("&token=(.+?)&thumbnail_url", r2->content);

                        if(token == "null")
                        {
                            std::cout << "[youtube] error: null token" << std::endl;

                            return;
                        }

                        nextgen::network::http_message m3;

                        m3->method = "GET";
                        m3->url = "http://www.youtube.com/get_video?video_id=" + v->id + "&t=" + token + "&el=detailpage&ps=";//&noflv=1";
                        m3->header_list["Host"] = "www.youtube.com";
                        m3->header_list["User-Agent"] = self->agent->title;
                        m3->header_list["Keep-Alive"] = "300";
                        m3->header_list["Connection"] = "keep-alive";
                        m3->header_list["Cookie"] = r1->header_list["set-cookie"];

std::cout << r1->header_list["proxy-connection"] << std::endl;

                        if(YOUTUBE_DEBUG_1)
                            std::cout << "[youtube] Receiving download" << std::endl;

                        if(r1->header_list["proxy-connection"] == "close"
                        || r1->header_list["connection"] == "close")
                        // reconnect if proxy closes the connection after one HTTP request
                        {
                            self->client.reconnect([=]()
                            {
                                self.video_download_detail(m3, view_count, view_max);
                            });

                            return;
                        }

                        self.video_download_detail(m3, view_count, view_max);
                    /*};

                    std::cout << "pccon: " << r1->header_list["proxy-connection"] << std::endl;

                    if(r1->header_list.find("proxy-connection") != r1->header_list.end()
                    && r1->header_list["proxy-connection"] == "close")
                    // reconnect if proxy closes the connection after one HTTP request
                    {
                        self->client.reconnect([=]()
                        {
                            self->client.send_and_receive(m2, video_info_request);
                        });

                        return;
                    }

                    // wait to avoid spam filter
                    nextgen::timeout(self->network_service, [=]()
                    {
                        self->client.send_and_receive(m2, video_info_request);
                    }, nextgen::random(1000, 3000));*/
                });
            });
        }

        private: void video_download_detail(nextgen::network::http_message m3, size_t view_count, size_t view_max) const
        {
            auto self = *this;

            if(view_count < view_max)
            {
                nextgen::timeout(self->network_service, [=]()
                {
                    self->client.send_and_receive(m3, [=](nextgen::network::http_message r3)
                    {
                        if((r3->status_code != 204 && r3->status_code != 303)
                        || to_int(r3->header_list["content-length"]) != 0)
                        {
                            if(YOUTUBE_DEBUG_1)
                                std::cout << "[proxos:youtube] Error receiving video download. " << r3->status_code << std::endl;

                            return;
                        }


                        if(YOUTUBE_DEBUG_1)
                            std::cout << "[proxos::youtube] VIEWED " << view_count+1 << " TIMES" << std::endl;

                        if(r3->header_list["proxy-connection"] == "close"
                        || r3->header_list["connection"] == "close")
                        // reconnect if proxy closes the connection after one HTTP request
                        {
                            self->client.reconnect([=]()
                            {
                                self.video_download_detail(m3, view_count+1, view_max);
                            });

                            return;
                        }

                        self.video_download_detail(m3, view_count+1, view_max);
                    });
                }, nextgen::random(1000, 3000));
            }
        }

        private: struct variables
        {
            variables(nextgen::network::service network_service) : network_service(network_service), client(network_service)
            {

            }

            ~variables()
            {

            }

            nextgen::network::service network_service;
            nextgen::network::http_client client;

            proxos::proxy proxy;
            proxos::agent agent;
        };

        NEXTGEN_SHARED_DATA(client, variables);
    };
}

#endif
