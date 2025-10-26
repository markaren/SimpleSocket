
#include "simple_socket/mqtt/MQTTBroker.hpp"
#include "simple_socket/mqtt/MQTTClient.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace simple_socket;

int main() {

    int tcpPort = 1883;
    int wsPort = tcpPort+1;

    MQTTBroker broker(tcpPort, wsPort);
    broker.start();

    try {
        MQTTClient client("127.0.0.1", tcpPort, "SimpleSocketClient");
        client.connect(false);

        std::string topic1 = "simple_socket/topic1";
        std::string topic2 = "simple_socket/topic2";

        client.subscribe(topic1, [topic1](const auto& msg) {
            std::cout << "[" << topic1 << "] Got: " << msg << std::endl;
        });

        client.subscribe(topic2, [topic2](const auto& msg) {
            std::cout << "[" << topic2 << "] Got: " << msg << std::endl;
        });

        client.subscribe("simple_socket/slider", [](const auto& msg) {
            std::cout << "[simple_socket/slider] Got: " << msg << std::endl;
        });

        std::atomic_bool stop;
        auto clientThread = std::thread([&client, topic1, topic2, &stop] {
            while (!stop) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                client.publish(topic1, "Hello from SimpleSocket MQTT!");
                client.publish(topic2, "Another hello from SimpleSocket MQTT!");
            }
        });

        client.run();

        std::thread([&client, topic2] {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            client.unsubscribe(topic2);
        }).detach();

#ifdef _WIN32
        system("start mqtt_client.html");
#endif

        std::cout << "Press any key to exit..." << std::endl;
        std::cin.get();
        stop = true;
        if (clientThread.joinable()) {
            clientThread.join();
        }
        client.close();
        broker.stop();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
