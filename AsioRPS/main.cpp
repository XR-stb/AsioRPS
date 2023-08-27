#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

using namespace boost::asio;

using ip::tcp;

boost::asio::io_context io_ctx;

class Player {
public:
    Player(tcp::socket&& socket, std::string choice) :
        m_socket(std::move(socket)), choice(choice)
    {
    }
public:
    tcp::socket m_socket;
    std::string choice;
};

std::queue<Player> play_queue;
std::mutex mutex;

std::string determineWinner(const std::string& choice1, const std::string& choice2) {
    if (choice1 == choice2) return "tie";
    std::string res = "win";
    //rock paper lose
    //paper  rock
    std::cout << choice1 << " " << choice1 << std::endl;
    if (choice1 == "rock" && choice2 == "paper") res = "lose";
    if (choice1 == "paper" && choice2 == "scissors") res = "lose";
    if (choice1 == "scissors" && choice2 == "rock") res = "lose";
    std::cout << res << std::endl;
    return res;
}

void trim(std::string& s, char c = ' ') {
    if (!s.empty()) {
        if (s[0] == c) s.erase(0, 1);
        if (s.back() == c) s.pop_back();
    }
}

void handleClient(tcp::socket&& client) {
    try {
        std::string instructions = "Welcome to Rock-Paper-Scissors game!\n"
            "Please enter your choice: rock, paper, or scissors.\n";
        boost::asio::write(client, boost::asio::buffer(instructions));

        char buffer[1024];
        std::size_t bytesReceived = client.read_some(boost::asio::buffer(buffer));
        std::string clientChoice(buffer, bytesReceived);
        std::cout << "recv choice : " << clientChoice << std::endl;
        trim(clientChoice, '\n');
        play_queue.push(Player(std::move(client), clientChoice));
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

void playgame(const boost::system::error_code& ec, boost::asio::steady_timer* t) {
    //std::lock_guard<std::mutex> lock(mutex);
    if (play_queue.size() >= 2) {
        Player player1 = std::move(play_queue.front()); play_queue.pop();
        Player player2 = std::move(play_queue.front()); play_queue.pop();
        std::string player1_res = determineWinner(player1.choice, player2.choice);
        std::string player2_res = determineWinner(player2.choice, player1.choice);
        boost::system::error_code ignored_error;
        std::cout << "player > 2\n";
        boost::asio::write(player1.m_socket, boost::asio::buffer(player1_res), ignored_error);
        boost::asio::write(player2.m_socket, boost::asio::buffer(player2_res), ignored_error);
        std::cout << ignored_error << std::endl;
    }
    t->expires_at(t->expiry() + boost::asio::chrono::seconds(1));
    t->async_wait(boost::bind(&playgame, boost::asio::placeholders::error, t));
}

void handleGame(ip::tcp::acceptor acceptor) {
    while (1) {
        ip::tcp::socket socket(io_ctx);
        acceptor.accept(socket);
        std::cout << "client connect\n";
        std::thread(handleClient, std::move(socket)).detach();
    }
}

int main() {
    try {
        ip::tcp::acceptor acceptor(io_ctx, ip::tcp::endpoint(ip::tcp::v4(), 8080));

        std::thread(handleGame, std::move(acceptor)).detach();

        boost::asio::steady_timer t(io_ctx, boost::asio::chrono::seconds(1));
        t.async_wait(boost::bind(&playgame, boost::asio::placeholders::error, &t));
        io_ctx.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
