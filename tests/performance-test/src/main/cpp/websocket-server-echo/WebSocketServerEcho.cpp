/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#include <boost/program_options.hpp>
#include <iostream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> Server;

int messageCount = 0;

void messageReceived(Server* server, websocketpp::connection_hdl hdl, Server::message_ptr message)
{
    messageCount++;
    std::string payload = message->get_payload();

#ifndef NDEBUG
    std::cout << "received: " << messageCount << std::endl;
#endif

    // killServer message
    if (payload == "killServer") {
        std::cout << "killServer" << std::endl;
        server->stop_listening();
        return;
    }

    // send received message back
    try {
        server->send(hdl, message->get_payload(), message->get_opcode());
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "send failed: " << e << ":" << e.message() << std::endl;
    }
}

int getPort(int argc, char* argv[])
{
    int port;
    namespace po = boost::program_options;
    po::options_description desc("parameters");
    desc.add_options()("help", "show usage")(
            "port,p", po::value<int>(&port)->default_value(4220), "server port");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << "usage: websocket-server-echo port" << std::endl;
        std::_Exit(0);
    }
    return port;
}

int main(int argc, char* argv[])
{
    int port = getPort(argc, argv);
    std::cout << "connecting on port:" << port << std::endl;
    Server server;

    try {
        server.init_asio();
        server.clear_access_channels(websocketpp::log::alevel::all);
        server.set_message_handler(
                std::bind(&messageReceived, &server, std::placeholders::_1, std::placeholders::_2));
        server.listen(port);
        server.start_accept();

        // Start the ASIO io_service run loop
        server.set_reuse_addr(true);
        server.run();
    } catch (websocketpp::exception const& e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}
