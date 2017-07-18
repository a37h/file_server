#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#define buffer_max_size 1024

class CSession {
private:
    boost::asio::ip::tcp::socket socket_;
    char buffer_data[buffer_max_size];
    // parse filename from request
    std::string parse_filename(char *buffer_data) {
        std::string str(buffer_data);
        std::size_t found1 = str.find("/get/");
        if (found1 != std::string::npos) {
            std::size_t  found2 = str.find(" ", found1);
            if (found2 != std::string::npos) {
                str = str.substr(found1+5, found2-found1-5);
                return str;
            }
        }
        return "";
    }
    // creating a response for client
    std::string get_response() {
        std::string response_body, response_header, filename = parse_filename(buffer_data);
        std::ifstream file(filename);
        if (file.good())
        {
            std::stringstream strStream;
            strStream << file.rdbuf();
            response_body = strStream.str();

            response_header = "HTTP/1.1 200 OK\nContent-Length:" + std::to_string(response_body.length()) +
                                          "\nContent-Type: application/octet-stream\nContent-Disposition: attachment; "
                                                  "filename=\"" + filename + "\"\nConnection: close\n\n";
            std::cout << "\n\nLast response header:_\n\n" << response_header;
            return response_header + response_body;
        }
        else
        {
            response_body = "<!DOCTYPE html>\n<html>\n<head>\n<title>404. File not found</title>\n</head>\n"
                    "<body>\n<center><h1>Error 404</h1>\n<p>File \""+ filename + "\" not found</p></center>\n</body>\n</html>";
            response_header = "HTTP/1.1 404 Not Found\nContent-Length:" + std::to_string(response_body.length()) +
                                          "\nContent-Type: text/html\nConnection: close\n\n";
            std::cout << "\n\nLast response header:_\n\n" << response_header;
            return response_header + response_body;
        }
        // need

    }
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
            std::string response = get_response();

            boost::asio::async_write(
                    socket_,
                    boost::asio::buffer(response, response.length()), // This goes into socket
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