#define _WIN32_WINNT 0x0600
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <thread>
#include <string>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

// inisialisasi WinSock
bool Initialize() {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

// menerima pesan dari server
void ReceiveMessages(SOCKET s) {
    char buffer[4096];
    int bytesReceived;

    while (true) {
        ZeroMemory(buffer, sizeof(buffer));
        bytesReceived = recv(s, buffer, sizeof(buffer), 0);

        if (bytesReceived > 0) {
            cout << "\n" << string(buffer, 0, bytesReceived) << endl;
        }
        else if (bytesReceived == 0) {
            cout << "Server menutup koneksi." << endl;
            break;
        }
        else {
            cout << "Terjadi error saat menerima pesan." << endl;
            break;
        }
    }

    closesocket(s);
    WSACleanup();
    exit(0); // hentikan program
}

// mengirim pesan ke server
void SendMessages(SOCKET s, const string& chatName) {
    string message;

    // pertama kirim username ke server
    int sent = send(s, chatName.c_str(), (int)chatName.length(), 0);
    if (sent == SOCKET_ERROR) {
        cout << "Gagal mengirim username ke server." << endl;
        return;
    }

    while (true) {
        cout << ">> ";
        getline(cin, message);

        if (message.empty()) continue;

        int bytesSent = send(s, message.c_str(), (int)message.length(), 0);
        if (bytesSent == SOCKET_ERROR) {
            cout << "Gagal mengirim pesan." << endl;
            break;
        }
    }
}

int main() {
    if (!Initialize()) {
        cout << "Inisialisasi Winsock gagal." << endl;
        return 1;
    }

    string serverIP;
    int port;

    cout << "Enter server address (IP or hostname): ";
    cin >> serverIP;

    cout << "Enter port: ";
    cin >> port;
    cin.ignore(); // buang newline

    // resolve alamat server (mendukung IPv4/IPv6 + hostname)
    addrinfo hints{}, * res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int rv = getaddrinfo(serverIP.c_str(), to_string(port).c_str(), &hints, &res);
    if (rv != 0) {
        cout << "getaddrinfo gagal: " << rv << endl;
        WSACleanup();
        return 1;
    }

    SOCKET s = INVALID_SOCKET;
    for (addrinfo* p = res; p != nullptr; p = p->ai_next) {
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == INVALID_SOCKET) continue;

        if (connect(s, p->ai_addr, (int)p->ai_addrlen) == SOCKET_ERROR) {
            closesocket(s);
            s = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(res);

    if (s == INVALID_SOCKET) {
        cout << "Gagal terhubung ke server." << endl;
        WSACleanup();
        return 1;
    }

    cout << "Berhasil terhubung ke server." << endl;

    // minta username
    string chatName;
    cout << "Masukkan nama Anda: ";
    getline(cin, chatName);

    // jalankan thread terima & kirim
    thread recvThread(ReceiveMessages, s);
    thread sendThread(SendMessages, s, chatName);

    sendThread.join();
    recvThread.join();

    closesocket(s);
    WSACleanup();
    return 0;
}
