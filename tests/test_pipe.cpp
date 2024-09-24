
#include "simple_socket/Pipe.hpp"

#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>

using namespace simple_socket;

namespace {

    const std::string domain{"test_pipe"};

    std::string generateMessage() {

        return "Per";
    }

    std::string generateResponse(const std::string& msg) {

        return "Hello " + msg + "!";
    }

}// namespace

TEST_CASE("NamedPipe read/write") {

    std::thread serverThread([] {
        std::unique_ptr<NamedPipe> conn = NamedPipe::listen(domain);
        REQUIRE(conn);

        std::vector<unsigned char> buffer(1024);
        const auto bytesRead = conn->read(buffer);

        std::string expectedMessage = generateMessage();
        REQUIRE(bytesRead == expectedMessage.size());

        std::string msg(buffer.begin(), buffer.begin() + bytesRead);
        REQUIRE(msg == expectedMessage);

        REQUIRE(conn->write(generateResponse(msg)));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::thread clientThread([] {
        const auto conn = NamedPipe::connect(domain);
        REQUIRE(conn);

        std::string message = generateMessage();
        std::string expectedResponse = generateResponse(message);

        conn->write(message);

        std::vector<unsigned char> buffer(1024);
        const auto bytesRead = conn->read(buffer);
        REQUIRE(bytesRead == expectedResponse.size());
        std::string response(buffer.begin(), buffer.begin() + bytesRead);

        CHECK(response == expectedResponse);
    });

    clientThread.join();
    serverThread.join();
}
