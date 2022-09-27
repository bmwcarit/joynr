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
#include <chrono>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <thread>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>

using Client = websocketpp::client<websocketpp::config::asio_client>;

class BenchmarkTest
{
public:
    BenchmarkTest(Client& client, int numberOfMessagesToSend)
            : client(client),
              numberOfMessagesToSend(numberOfMessagesToSend),
              numberOfReceivedMessages(0),
              payload(R"({)"
                      R"("_typeName":"joynr.types.TestTypes.TStructExtended",)"
                      R"("tDouble":0.123456789,)"
                      R"("tInt64":64,)"
                      R"("tString":"myTestString",)"
                      R"("tEnum":"TLITERALA",)"
                      R"("tInt32":32)"
                      R"(})")
    {
    }

    void onConnectionEstablished(websocketpp::connection_hdl& connection)
    {
        startedTimestamp = std::chrono::high_resolution_clock::now();
        sendMessages(connection);
    }

    void onMessageReceived()
    {
        numberOfReceivedMessages++;

        if (finished()) {
            finishedTimestamp = std::chrono::high_resolution_clock::now();
        }
    }

    bool finished() const
    {
        return numberOfReceivedMessages >= numberOfMessagesToSend;
    }

    std::chrono::microseconds getDurationUs() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(finishedTimestamp -
                                                                     startedTimestamp);
    }

    double getNumMessagesPerSecond() const
    {
        auto durationUs = getDurationUs();

        if (durationUs == std::chrono::microseconds(0)) {
            return 0.0;
        }

        return static_cast<double>(numberOfMessagesToSend) /
               (static_cast<double>(durationUs.count()) / 10e6);
    }

private:
    void sendMessages(websocketpp::connection_hdl& connection)
    {
        websocketpp::lib::error_code errorCode;

        for (int i = 0; i < numberOfMessagesToSend; i++) {
            client.send(connection, payload, websocketpp::frame::opcode::text, errorCode);

            if (errorCode) {
                std::cout << "Failed to send message #" << i << ": " << errorCode.message()
                          << std::endl;
            }
        }
    }

private:
    Client& client;

    int numberOfMessagesToSend;
    int numberOfReceivedMessages;

    std::chrono::high_resolution_clock::time_point startedTimestamp;
    std::chrono::high_resolution_clock::time_point finishedTimestamp;

    std::string payload;
};

int main(int argc, char* argv[])
{
    namespace po = boost::program_options;

    int port = 0;
    std::string hostAddress;
    int numberOfMessages = 0;

    po::options_description desc("parameters");

    desc.add_options()("help", "show usage")(
            "port,p", po::value<int>(&port)->default_value(4220), "server port")(
            "hostaddress,h", po::value<std::string>(&hostAddress)->default_value("localhost"),
            "server address")("numberofmessages,n",
                              po::value<int>(&numberOfMessages)->default_value(10000),
                              "number of messages to transmit");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    try {
        Client client;

        client.init_asio();
        client.clear_access_channels(websocketpp::log::alevel::all);
        client.clear_error_channels(websocketpp::log::alevel::all);

        const bool secureConnection = false;
        websocketpp::lib::error_code errorCode;
        websocketpp::uri hostUri(secureConnection, hostAddress, port, std::string(""));

        BenchmarkTest benchmark(client, numberOfMessages);

        client.set_open_handler([&benchmark](websocketpp::connection_hdl connection) {
            benchmark.onConnectionEstablished(connection);
        });

        client.set_message_handler(
                [&benchmark, &client](websocketpp::connection_hdl connection,
                                      websocketpp::config::asio_client::message_type::ptr message) {
                    std::ignore = message;

                    benchmark.onMessageReceived();

                    if (benchmark.finished()) {
                        websocketpp::lib::error_code errorCode;
                        client.close(connection, websocketpp::close::status::normal,
                                     std::string(""), errorCode);

                        if (errorCode) {
                            std::cout << "Failed to close connection";
                            std::exit(EXIT_FAILURE);
                        }
                    }
                });

        auto connection = client.get_connection(hostUri.str(), errorCode);

        if (errorCode) {
            std::cout << "Failed to create connection: " << errorCode.message() << std::endl;
            std::exit(EXIT_FAILURE1);
        }

        client.connect(connection);

        auto thread = std::thread(&Client::run, &client);
        thread.join();

        std::cout << "Duration: " << static_cast<double>(benchmark.getDurationUs().count()) / 10e6
                  << " sec" << std::endl;
        std::cout << "Msgs/s: " << benchmark.getNumMessagesPerSecond() << std::endl;
    } catch (websocketpp::exception const& e) {
        std::cout << "Websocket++ exception: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return EXIT_SUCCESS;
}
