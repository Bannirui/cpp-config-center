#pragma once

#include "config-center/api/api_types.h"

#include <boost/asio/io_context.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace config_center {

class HttpServer {
public:
    HttpServer(boost::asio::io_context& ioc, const std::string& host, uint16_t port, ApiContext context);
    ~HttpServer();

    void start();
    void stop();

private:
    void doAccept();
    void handleRequest(class HttpSession& session);

    boost::asio::io_context& ioc_;
    std::string host_;
    uint16_t port_;
    ApiContext context_;
    class tcp::acceptor* acceptor_;
    std::vector<std::thread> threads_;
};

} // namespace config_center
