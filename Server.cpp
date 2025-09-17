// server.cpp
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <algorithm>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

vector<SOCKET> clients;
map<SOCKET, string> usernames; // simpan username tiap socket
mutex mtx;

// broadcast ke semua client
void Broadcast(const string& msg, SOCKET sender = INVALID_SOCKET) {
    lock_guard<mutex> lock(mtx);
    for (SOCKET client : clients) {
        if (client != sender) {
            send(client, msg.c_str(), (int)msg.size(), 0);
        }
    }
}

// kick user tertentu
void KickUser(const string& targetName) {
    lock_guard<mutex> lock(mtx);
    SOCKET targetSocket = INVALID_SOCKET;
    for (auto& u : usernames) {
        if (u.second == targetName) {
            targetSocket = u.first;
            break;
        }
    }

    if (targetSocket != INVALID_SOCKET) {
        string msg = "Anda telah di-kick oleh admin.";
        send(targetSocket, msg.c_str(), (int)msg.size(), 0);
        closesocket(targetSocket);

        clients.erase(remove(clients.begin(), clients.end(), targetSocket), clients.end());
        usernames.erase(targetSocket);

        string announce = targetName + " telah di-kick oleh admin.";
        cout << announce << endl;
        Broadcast(announce);
    }
    else {
        cout << "User " << targetName << " tidak ditemukan." << endl;
    }
}

void HandleClient(SOCKET clientSocket) {
    char buffer[1024];
    string username;

    // terima username pertama kali
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }
    buffer[bytesReceived] = '\0';
    username = buffer;

    {
        lock_guard<mutex> lock(mtx);
        usernames[clientSocket] = username;
    }

    string joinMsg = username + " bergabung ke chat.";
    cout << joinMsg << endl;
    Broadcast(joinMsg, clientSocket);

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) break;

        buffer[bytesReceived] = '\0';
        string msg = buffer;

        // cek command
        if (msg == "/list") {
            string listMsg = "Online: ";
            {
                lock_guard<mutex> lock(mtx);
                for (auto& u : usernames) {
                    listMsg += u.second + " ";
                }
            }
            send(clientSocket, listMsg.c_str(), (int)listMsg.size(), 0);
        }
        else if (msg.rfind("/msg ", 0) == 0) {
            size_t spacePos = msg.find(' ', 5);
            if (spacePos != string::npos) {
                string target = msg.substr(5, spacePos - 5);
                string privateMsg = msg.substr(spacePos + 1);
                bool found = false;

                lock_guard<mutex> lock(mtx);
                for (auto& u : usernames) {
                    if (u.second == target) {
                        string formatted = "[PM dari " + username + "] " + privateMsg;
                        send(u.first, formatted.c_str(), (int)formatted.size(), 0);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    string err = "User " + target + " tidak ditemukan.";
                    send(clientSocket, err.c_str(), (int)err.size(), 0);
                }
            }
        }
        else {
            string formatted = username + ": " + msg;
            cout << formatted << endl;
            Broadcast(formatted, clientSocket);
        }
    }

    {
        lock_guard<mutex> lock(mtx);
        clients.erase(remove(clients.begin(), clients.end(), clientSocket), clients.end());
        string leaveMsg = username + " keluar dari chat.";
        cout << leaveMsg << endl;
        Broadcast(leaveMsg, clientSocket);
        usernames.erase(clientSocket);
    }

    closesocket(clientSocket);
}

// thread untuk command admin (server-side input)
void AdminConsole() {
    string cmd;
    while (true) {
        getline(cin, cmd);
        if (cmd.rfind("/kick ", 0) == 0) {
            string target = cmd.substr(6);
            KickUser(target);
        }
        else if (cmd == "/quit") {
            cout << "Server dimatikan oleh admin." << endl;
            exit(0);
        }
    }
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cout << "WSAStartup gagal." << endl;
        return 1;
    }

    string ip;
    int port;

    cout << "Masukkan IP server (misal 0.0.0.0 untuk semua interface): ";
    cin >> ip;
    cout << "Masukkan port server: ";
    cin >> port;
    cin.ignore(); // buang newline biar getline di AdminConsole lancar

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Gagal membuat socket server." << endl;
        WSACleanup();
        return 1;
    }

    // biar bisa restart tanpa tunggu TIME_WAIT
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &(serverAddr.sin_addr));

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Gagal bind ke " << ip << ":" << port << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Listen gagal." << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "Server berjalan di " << ip << ":" << port << " ...\n";

    // jalankan thread admin console
    thread(AdminConsole).detach();

    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            cout << "Gagal menerima koneksi client." << endl;
            continue; // jangan tutup server, tetap lanjut
        }

        {
            lock_guard<mutex> lock(mtx);
            clients.push_back(clientSocket);
        }

        cout << "Client baru terhubung." << endl;
        thread(HandleClient, clientSocket).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
