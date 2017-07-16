#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#define buffer_max_size 1024

// Класс отвечающий за одно соединение с сервером
class CSession {
private:
    boost::asio::ip::tcp::socket socket_;
    char buffer_data[buffer_max_size];
public:
    // Constructor which takes single io_service pointer.
    CSession(boost::asio::io_service &io_service): socket_(io_service) {}
    // Function which returns current session socket (used by server)
    boost::asio::ip::tcp::socket& socket() { return socket_; }
    // Starting session
    void start() {
        socket_.async_read_some (
                // One or more buffers into which the data will be read.
                boost::asio::buffer(buffer_data, buffer_max_size), // There will be text from socket
                // The handler to be called when the read operation completes.
                boost::bind(&CSession::handle_read, this, boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred)
        );
    }
    // Function which is called after io_service read from socket
    void handle_read(const boost::system::error_code &error, size_t bytes_transferred) {
        if (!error) {
            std::cout << buffer_data;
            boost::asio::async_write(
                    socket_,
                    boost::asio::buffer(buffer_data, bytes_transferred), // This goes into socket
                    boost::bind(&CSession::handle_write, this, boost::asio::placeholders::error)
            );
        }
        else {
            delete this;
        }
    }
    // Function which is called after io_service wrote into socket
    void handle_write(const boost::system::error_code &error) {
        if (!error) {
            socket_.async_read_some(
                    boost::asio::buffer(buffer_data, buffer_max_size),
                    boost::bind(&CSession::handle_read, this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred)
            );
        } else {
            delete this;
        }
    }
};

class CServer {
private:
    boost::asio::io_service &io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;
public:
    CServer(boost::asio::io_service &io_service, short port): io_service_(io_service), acceptor_(io_service,
                      boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    {
        CSession *new_CSession = new CSession(io_service_);
        //
        acceptor_.async_accept(new_CSession->socket(),
                               boost::bind(&CServer::handle_accept, this, new_CSession,
                                           boost::asio::placeholders::error)
        );
    }
    void handle_accept(CSession *new_CSession, const boost::system::error_code &error_)
    {
        if (!error_) {
            new_CSession->start();
            new_CSession = new CSession(io_service_);
            acceptor_.async_accept(new_CSession->socket(),
                                   boost::bind(&CServer::handle_accept, this, new_CSession,
                                               boost::asio::placeholders::error));
        } else {
            delete new_CSession;
        }
    }
};

void CServer_start(int argc, char **argv)
{
    int port = 0;
    if (argc < 2) {
        std::cout << "\nEnter a port to start server on: ";
        std::cin >> port;
    }
    else
    {
        port = atoi(argv[1]);
    }
    std::cout << "\nServer log:\n\n";
    boost::asio::io_service io_service;
    CServer s(io_service, port);
    io_service.run();
}

int main(int argc, char **argv) {
    CServer_start(argc, argv);

    return 0;
}