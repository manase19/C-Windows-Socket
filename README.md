# Chat Server & Client (C++ / WinSock2)

Proyek ini adalah implementasi sederhana **chat server dan client** menggunakan **C++ dengan WinSock2**.  
Program mendukung:
- Multi-client (banyak pengguna sekaligus).
- Chat dengan nama pengguna (username).
- Command admin (`/kick`, `/quit`).
- Command client (`/list`, `/msg <user> <pesan>`).
- Kompatibel dengan **localhost**, **LAN**, dan **internet** (via port forwarding atau ngrok).

- Bisa di run dengan .exe secara langsung, terdapat transparansi *source code* bila perlu debugging

- Rekomendasi pribadi adalah supaya pengguna server menggunakan ngrok untuk membuat tcp yang memiliki addres yang dapat diterima oleh client
- Copy paste IP dan Port supaya dapat diakses client
