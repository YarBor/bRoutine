#include "../src/bMutex_Cond.h"
#include "../src/bRoutine.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
bMutex mtx;
bCond cv;

void* clientRoutine(void*)
{
    // 创建客户端套接字
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Failed to create client socket." << std::endl;
        return 0;
    }

    // 设置服务器地址
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8888); // 服务器端口号
    if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) <= 0) {
        std::cerr << "Invalid server address." << std::endl;
        return 0;
    }

    // 连接服务器
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to connect to the server." << std::endl;
        return 0;
    }

    // 模拟客户端发送消息

    char buffer[1024] = { 0 };
    for (int i = 0, t = rand(); 1; i++,t = rand()) {
        std::string msg = "Hello, server! -- id:" + std::to_string(t);
        // 向服务器发送消息
        if (write(clientSocket, msg.c_str(), msg.length()) <= 0) {
            std::cerr << "Failed to send message to the server." << std::endl;
            return 0;
        } else {
            std::cout << "Client send :>" << msg << std::endl;
        }

        pollfd fdSet = { clientSocket, POLLIN | POLLHUP | POLLERR | POLLRDHUP, 0 };

        if (poll(&fdSet, 1, 1000) == 0) {
            std::cerr << "Fatal : Client cannot recv msg" << std::endl;
            exit(EXIT_FAILURE);
        }

        ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));
        if (bytesRead < 0) {
            perror("Failed to read client message.");
        }
        std::cout << "Client Recv :>" << std::string(buffer, bytesRead) << std::endl;
        putchar('\n');
        poll(nullptr, 0, 1000);
    }
    // 关闭客户端套接字
    close(clientSocket);
    return 0;
}
void* serverRoutine(void* i)
{
    bRoutine::getSelf()->enableHook();
    auto listenSuccess = (int*)i;
    mtx.lock();
    // 创建服务器套接字
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create server socket." << std::endl;
        return 0;
    }

    // 设置服务器地址
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8888); // 服务器端口号
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // 绑定服务器地址
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind server socket." << std::endl;
        return 0;
    }

    // 监听连接
    if (listen(serverSocket, 1) < 0) {
        std::cerr << "Failed to listen for connections." << std::endl;
        return 0;
    }

    *listenSuccess = true;
    cv.signalAll();
    mtx.unlock();

    std::vector<struct pollfd> pollfds;
    pollfds.push_back({ serverSocket, POLLIN | POLLERR, 0 });

    char buffer[1024] = { 0 };

    while (1) {
        pollfd* pollfdsp = pollfds.data();
        nfds_t size = (nfds_t)pollfds.size();
        int n = poll(pollfdsp, size, 100);
        int CloseCount = 0;
        for (size_t i = 0; n && i < pollfds.size(); i++) {
            if (pollfds[i].revents & POLLIN) {
                if (pollfds[i].fd == serverSocket) {
                    int clientSocket = accept(serverSocket, nullptr, nullptr);
                    std::cout << "clientSocket Link!" << std::endl;
                    if (clientSocket < 0) {
                        std::cerr << "Failed to accept client connection." << std::endl;
                        return 0;
                    }
                    pollfds.push_back({ clientSocket, POLLIN | POLLERR | POLLRDHUP | POLLHUP, 0 });
                } else {

                    int clientSocket = pollfds[i].fd;
                    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));

                    if (bytesRead < 0) {
                        std::cerr << "Failed to read client message." << std::endl;
                        break;
                    } else {
                        std::cout << "Server Recv:> " << std::string(buffer, bytesRead) << std::endl;
                    }

                    if (send(clientSocket, buffer, bytesRead, 0) < 0) {
                        std::cerr << "Failed to send message to the client." << std::endl;
                        break;
                    } else {
                        std::cout << "Server Send:> " << std::string(buffer, bytesRead) << std::endl;
                    }
                }
            } else if (pollfds[i].revents & (POLLERR | POLLRDHUP | POLLHUP)) {
                close(pollfds[i].fd);
                CloseCount++;
            } else {
                continue;
            }
            n--;
        }
        if (n) {
            std::cerr << "undefine behavior" << std::endl;
            abort();
        }
        pollfds.reserve(pollfds.size() - CloseCount);
    }
    return 0;
}

int main()
{
    int IsListening = 0;
    bRoutine::New(StackLevel::MediumStack, serverRoutine, &IsListening)->Resume();
    auto i = bRoutine::New(StackLevel::MediumStack, clientRoutine, nullptr);

    while (mtx.lock() && !IsListening) {
        cv.wait(&mtx);
    }

    i->Resume();

    bRoutine::deleteSelf();
    return 0;
}