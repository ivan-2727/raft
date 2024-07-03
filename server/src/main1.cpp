#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <bits/stdc++.h>

using tcp = boost::asio::ip::tcp;
using http = boost::beast::http;

void handleRequest(http::request<http::string_body>& request, tcp::socket& socket) {
    // Prepare the response message
    http::response<http::string_body> response;
    response.version(request.version());
    response.result(http::status::ok);
    response.set(http::field::server, "My HTTP Server");
    response.set(http::field::content_type, "text/plain");
    response.body() = "Hello, World!";
    response.prepare_payload();
    boost::beast::http::write(socket, response);
}

void runServer() {
    boost::asio::io_context io_context;
    tcp::acceptor acceptor(io_context, {tcp::v4(), 8080});
    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        boost::beast::flat_buffer buffer;
        http::request<http::string_body> request;
        boost::beast::http::read(socket, buffer, request);
        handleRequest(request, socket);
        socket.shutdown(tcp::socket::shutdown_send);
    }
}

int main() {
    try {
        runServer();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}