#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <bits/stdc++.h>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

class Connection {
private:
    const int httpVersion = 11;
    std::mutex mtxReq_, mtxRes_;
    std::condition_variable cvReq_;
    std::deque<std::string> reqStrs_;
    std::deque<std::string> resStrs_;
    std::thread loop_;
public:
    Connection(const std::string& host, const std::string& port, const std::string& target) {
        loop_ = std::thread([this, host, port, target]() -> void {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            auto const results = resolver.resolve(host, port);
            while(true) {
                std::string reqStr;
                {
                    std::unique_lock<decltype(mtxReq_)> lock(mtxReq_);
                    cvReq_.wait(lock, [this] {
                        return !reqStrs_.empty();
                    });
                    if (reqStrs_.empty()) {
                        continue;
                    }
                    reqStr = reqStrs_[0];
                    reqStrs_.pop_front();
                }
                try {
                    beast::tcp_stream stream(ioc);
                    stream.connect(results);
                    http::request<http::string_body> httpRequest{http::verb::post, target, httpVersion};
                    httpRequest.set(http::field::host, host);
                    httpRequest.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
                    httpRequest.set(http::field::content_type, "text/plain");
                    httpRequest.body() = reqStr;
                    httpRequest.prepare_payload();
                    http::write(stream, httpRequest);
                    beast::flat_buffer buffer;
                    http::response<http::dynamic_body> res;
                    http::read(stream, buffer, res);
                    std::cout << res << std::endl;
                    beast::error_code ec;
                    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
                    if(ec && ec != beast::errc::not_connected) {
                        throw beast::system_error{ec};
                    }
                } catch(const std::exception& e) {
                    std::cout << e.what() << "\n";
                }
            }
        });
    }
    void send(const std::string& reqStr) {
        std::unique_lock<decltype(mtxReq_)> lock(mtxReq_);
        reqStrs_.push_back(reqStr);
        cvReq_.notify_one();
    }
    ~Connection() {
        if (loop_.joinable()) {
            loop_.join();
        }
    }
};

int main(void) {
    auto conn = Connection("127.0.0.1", "5000", "/");  
    conn.send("Bla");    
    return 0;
}