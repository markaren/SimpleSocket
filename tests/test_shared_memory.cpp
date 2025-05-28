
#include "simple_socket/SharedMemoryConnection.hpp"

#include <catch2/catch_test_macros.hpp>

#include <thread>
#include <vector>
#include <iostream>

using namespace simple_socket;

namespace {
    const std::string sharedMemName{"test_shared_mem"};
    constexpr size_t bufferSize = 1024;

    std::string generateMessage() {
        return "Per";
    }

    std::string generateResponse(const std::string& msg) {
        return "Hei " + msg + "!";
    }

    void memoryHandler(std::unique_ptr<SimpleConnection> conn) {
        std::vector<unsigned char> buffer(bufferSize);
        const auto bytesRead = conn->read(buffer);

        std::string expectedMessage = generateMessage();
        REQUIRE(bytesRead == expectedMessage.size());

        std::string msg(buffer.begin(), buffer.begin() + bytesRead);
        REQUIRE(msg == expectedMessage);

        std::cout << "Received message: " << msg << std::endl;

        const auto response = generateResponse(msg);
        REQUIRE(conn->write(response));

        std::cout << "Sent response: " << response << std::endl;
    }
}// namespace

TEST_CASE("Shared Memory basic read/write") {
    std::thread serverThread([] {
        auto conn = std::make_unique<SharedMemoryConnection>(sharedMemName, bufferSize, true);
        memoryHandler(std::move(conn));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::thread clientThread([] {
        auto conn = std::make_unique<SharedMemoryConnection>(sharedMemName, bufferSize, false);
        REQUIRE(conn);

        std::string message = generateMessage();
        std::string expectedResponse = generateResponse(message);

        REQUIRE(conn->write(message));

        std::vector<unsigned char> buffer(bufferSize);
        const auto bytesRead = conn->read(buffer);
        REQUIRE(bytesRead == expectedResponse.size());

        std::string response(buffer.begin(), buffer.begin() + bytesRead);
        CHECK(response == expectedResponse);
    });

    clientThread.join();
    serverThread.join();
}

TEST_CASE("Shared Memory readExact/write") {
    std::thread serverThread([] {
        auto conn = std::make_unique<SharedMemoryConnection>(sharedMemName, bufferSize, true);
        memoryHandler(std::move(conn));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread clientThread([] {
        auto conn = std::make_unique<SharedMemoryConnection>(sharedMemName, bufferSize, false);
        REQUIRE(conn);

        std::string message = generateMessage();
        std::string expectedResponse = generateResponse(message);

        REQUIRE(conn->write(message));

        std::vector<unsigned char> buffer(expectedResponse.size());
        REQUIRE(conn->readExact(buffer));

        std::string response(buffer.begin(), buffer.end());
        CHECK(response == expectedResponse);
    });

    clientThread.join();
    serverThread.join();
}

TEST_CASE("Shared Memory large data transfer") {
    const size_t largeSize = bufferSize - 100;// leave some space for control data
    std::vector<uint8_t> largeData(largeSize);

    // fill with testdata
    for (size_t i = 0; i < largeSize; ++i) {
        largeData[i] = static_cast<uint8_t>(i % 256);
    }

    std::thread serverThread([&largeData] {
        auto conn = std::make_unique<SharedMemoryConnection>(sharedMemName, bufferSize, true);
        std::vector<uint8_t> receivedData(largeSize);

        const auto bytesRead = conn->read(receivedData);
        REQUIRE(bytesRead == largeSize);
        CHECK(receivedData == largeData);

        REQUIRE(conn->write(receivedData));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread clientThread([&largeData] {
        auto conn = std::make_unique<SharedMemoryConnection>(sharedMemName, bufferSize, false);
        REQUIRE(conn);

        REQUIRE(conn->write(largeData));

        std::vector<uint8_t> receivedData(largeSize);
        REQUIRE(conn->readExact(receivedData));
        CHECK(receivedData == largeData);
    });

    clientThread.join();
    serverThread.join();
}

TEST_CASE("Shared Memory buffer overflow handling") {
    const size_t smallBufferSize = 64;
    auto serverConn = std::make_unique<SharedMemoryConnection>(sharedMemName, smallBufferSize, true);
    auto clientConn = std::make_unique<SharedMemoryConnection>(sharedMemName, smallBufferSize, false);

    std::vector<uint8_t> tooLargeData(smallBufferSize + 1);
    CHECK_FALSE(clientConn->write(tooLargeData));
}
