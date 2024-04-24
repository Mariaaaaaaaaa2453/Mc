#include <iostream>
#include <string>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

constexpr size_t BufferLength = 4096;
const char* ServerAddress = "127.0.0.1";
const char* ServerPort = "8888";

SOCKET clientSocket;

void DisplayMenu() {
    cout << "Menu Options:\n";
    cout << "1. Hamburger - $5\n";
    cout << "2. Coke - $2\n";
    cout << "3. Onion Rings - $3\n";
    cout << "4. Sundae - $2\n";
    cout << "5. Chicken Sandwich - $6\n";
    cout << "6. McDouble - $4\n";
    cout << "7. Chicken Wrap - $5\n";
    cout << "8. Quarter Pounder - $7\n";
    cout << "9. Fish Fillet - $5\n";
    cout << "10. Hot Sauce - $1\n";
    cout << "Enter your order (e.g., Quarter Pounder, Coke, Onion Rings, Hot Sauce):\n";
}

DWORD WINAPI SendOrder(LPVOID param) {
    while (true) {
        char orderDetails[BufferLength];
        cin.getline(orderDetails, BufferLength);
        if (send(clientSocket, orderDetails, static_cast<int>(strlen(orderDetails)), 0) == SOCKET_ERROR) {
            cerr << "Error sending the order: " << WSAGetLastError() << "\n";
            continue;
        }
    }
    return 0;
}

DWORD WINAPI ReceiveResponse(LPVOID param) {
    while (true) {
        char serverResponse[BufferLength];
        int receivedBytes = recv(clientSocket, serverResponse, BufferLength, 0);
        if (receivedBytes > 0) {
            serverResponse[receivedBytes] = '\0';
            cout << "Server: " << serverResponse << "\n";
        } else if (receivedBytes == 0) {
            cout << "Connection closed by server.\n";
            break;
        } else {
            cerr << "recv failed: " << WSAGetLastError() << "\n";
            break;
        }
    }
    return 0;
}

int InitializeConnection() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed.\n";
        return -1;
    }

    struct addrinfo *result = nullptr, hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int status = getaddrinfo(ServerAddress, ServerPort, &hints, &result);
    if (status != 0) {
        WSACleanup();
        cerr << "getaddrinfo failed: " << status << "\n";
        return -2;
    }

    for (struct addrinfo *ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        clientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (clientSocket == INVALID_SOCKET) {
            WSACleanup();
            cerr << "Error at socket(): " << WSAGetLastError() << "\n";
            return -3;
        }

        if (connect(clientSocket, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) == SOCKET_ERROR) {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (clientSocket == INVALID_SOCKET) {
        WSACleanup();
        cerr << "Unable to connect to server!\n";
        return -4;
    }

    return 0;
}

int main() {
    if (InitializeConnection() != 0) {
        return 1;
    }

    DisplayMenu();

    CreateThread(NULL, 0, SendOrder, NULL, 0, NULL);
    CreateThread(NULL, 0, ReceiveResponse, NULL, 0, NULL);

    WaitForSingleObject(GetCurrentThread(), INFINITE);

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}