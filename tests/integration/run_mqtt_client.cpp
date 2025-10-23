
#include "simple_socket/mqtt/MQTTBroker.hpp"
#include "simple_socket/mqtt/MQTTClient.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace simple_socket;

int main() {


    try {
        MQTTClient client("test.mosquitto.org", 1883, "SimpleSocketClient");
        client.connect(false);

        std::string topic1 = "simple_socket/topic1";
        std::string topic2 = "simple_socket/topic2";

        client.subscribe(topic1, [topic1](const auto& msg) {
            std::cout << "[" << topic1 << "] Got: " << msg << std::endl;
        });

        client.subscribe(topic2, [topic2](const auto& msg) {
            std::cout << "[" << topic2 << "] Got: " << msg << std::endl;
        });

        std::atomic_bool stop = false;
        std::thread([&client, &stop, topic1, topic2] {
            while (!stop) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                client.publish(topic1, "Hello from SimpleSocket MQTT!");
                client.publish(topic2, "Another hello from SimpleSocket MQTT!");
            }
        }).detach();

        client.run();

        std::thread([&client, topic2] {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            client.unsubscribe(topic2);
        }).detach();

        std::cout << "Press any key to exit..." << std::endl;
        std::cin.get();
        stop = true;
        client.close();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
