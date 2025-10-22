
#include "simple_socket/MQTTClient.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace simple_socket;

int main() {
    try {
        MQTTClient client("test.mosquitto.org", 1883, "SimpleSocketClient");
        client.connect();
        client.subscribe("ecos/test", [](const auto& msg) {
            std::cout << "Got: " << msg << std::endl;
        });

        std::atomic_bool stop = false;
        std::thread([&client, &stop] {
            while (!stop) {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                client.publish("ecos/test", "Hello from SimpleSocket MQTT!");
            }
        }).detach();

        client.run();

        std::cout << "Press Any key to exit..." << std::endl;
        std::cin.get();
        stop = true;
        client.close();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}