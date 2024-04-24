никсик💖, [24. 4. 2024, 16:10:05]:
#include <iostream>
#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

using namespace std;

const int MaxClients = 100;
const int BufferSize = 4096;

struct FoodOrder {
    int preparationTime;
    string confirmationMessage;
    bool isValid;
};

SOCKET mainSocket;
vector<string> orderHistory;
vector<FoodOrder> orderQueue;

void initializeServer();
void processConnections();
FoodOrder processOrder(const string& orderDetails);

int main() {
    cout << "Initializing server...\n";
    initializeServer();
    cout << "Server initialized and ready to accept connections.\n";
    processConnections();
    WSACleanup();
}

void initializeServer() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Failed to initialize Winsock. Error: " << WSAGetLastError() << endl;
        exit(EXIT_FAILURE);
    }

    if ((mainSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cerr << "Socket creation failed. Error: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8888);

    if (bind(mainSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        cerr << "Binding socket failed. Error: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    listen(mainSocket, MaxClients);
}

void processConnections() {
    fd_set readDescriptors;
    SOCKET clientSockets[MaxClients] = {0};

    while (true) {
        FD_ZERO(&readDescriptors);
        FD_SET(mainSocket, &readDescriptors);

        for (auto& socket : clientSockets) {
            if (socket > 0) {
                FD_SET(socket, &readDescriptors);
            }
        }

        if (select(0, &readDescriptors, nullptr, nullptr, nullptr) == SOCKET_ERROR) {
            cerr << "Error selecting readable sockets. Error: " << WSAGetLastError() << endl;
            continue;
        }

        if (FD_ISSET(mainSocket, &readDescriptors)) {
            sockaddr_in clientAddress;
            int addressLength = sizeof(clientAddress);
            SOCKET newSocket = accept(mainSocket, reinterpret_cast<sockaddr*>(&clientAddress), &addressLength);
            if (newSocket == INVALID_SOCKET) {
                cerr << "Accepting new connection failed." << endl;
                continue;
            }

            cout << "New connection from " << inet_ntoa(clientAddress.sin_addr) << ":" << ntohs(clientAddress.sin_port) << endl;
            for (int i = 0; i < MaxClients; ++i) {
                if (clientSockets[i] == 0) {
                    clientSockets[i] = newSocket;
                    break;
                }
            }
        }

        for (int i = 0; i < MaxClients; ++i) {
            SOCKET& sock = clientSockets[i];
            if (FD_ISSET(sock, &readDescriptors)) {
                char buffer[BufferSize];
                int bytesRead = recv(sock, buffer, BufferSize, 0);
                if (bytesRead <= 0) {
                    cout << "Client disconnected or error occurred." << endl;
                    closesocket(sock);
                    sock = 0;
                } else {
                    buffer[bytesRead] = '\0';
                    FoodOrder newOrder = processOrder(string(buffer));
                    send(sock, newOrder.confirmationMessage.c_str(), newOrder.confirmationMessage.length(), 0);
                    if (newOrder.isValid) {
                        Sleep(newOrder.preparationTime * 1000);
                        string readyMessage = "Your order is ready. Enjoy your meal!";
                        send(sock, readyMessage.c_str(), readyMessage.length(), 0);
                    }
                }
            }
        }
    }
}

FoodOrder processOrder(const string& orderDetails) {
    FoodOrder order;
    string response = "Thank you for your order!\n";
    transform(orderDetails.begin(), orderDetails.end(), orderDetails.begin(), ::toupper);
    int timeRequired = 0, totalCost = 0;
    bool foundItem = false;

    if (orderDetails.find("HAMBURGER") != string::npos) {
        foundItem = true;
        timeRequired += 5;
        totalCost += 4;
    }
    // Additional menu items would follow similar pattern
    if (foundItem) {
        response += "Please wait for " + to_string(timeRequired) + " seconds.\n";
        response += "Your total cost is $" + to_string(totalCost) + ".\n";
    } else {
        response = "Please choose something from the menu.\n";
    }

    order.preparationTime = timeRequired;
    order.confirmationMessage = response;
    order.isValid = foundItem;
    return order;
}