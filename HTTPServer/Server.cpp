#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <fstream>
#include <charconv>
#include "http.hpp"

#define BUFSIZE 1024 * 1024 * 32

int main() {
	const WORD WinSockDLLVersion = MAKEWORD(2, 2);
	WSADATA wsa;
	WSAStartup(WinSockDLLVersion, &wsa);

	SOCKET tcp_server_socket, tcp_client_socket;
	SOCKADDR_IN tcp_server, tcp_client;
	int tcp_client_size = sizeof tcp_client;

	tcp_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	tcp_server.sin_family = AF_INET;
	tcp_server.sin_addr.s_addr = inet_addr("127.0.0.1");
	tcp_server.sin_port = htons(80);
	bind(tcp_server_socket, (const SOCKADDR*)&tcp_server, sizeof tcp_server);

	listen(tcp_server_socket, 1);


	bool listening = true;
	while (listening) {
		std::cout << "\nWaiting for incoming connections...\n";

		tcp_client_socket = accept(tcp_server_socket, (SOCKADDR*)&tcp_client, &tcp_client_size);
		std::cout << "IP " << inet_ntoa(tcp_client.sin_addr) << " has connected to the server.\n\n";

		Request request(BUFSIZE);

		int stride = 0;
		do stride += recv(tcp_client_socket, (char*)(request.get_raw() + stride), BUFSIZE - stride, 0);
		while (std::strstr((const char*)request.get_raw(), header_end.c_str()) == nullptr);

		request.parse();
		request.repr(false);

		while (stride < request.header_size + request.content_length)
			stride += recv(tcp_client_socket, (char*)(request.get_raw() + stride), BUFSIZE - stride, 0);
		
		Response response(BUFSIZE);

		response.version = "HTTP/1.1";
		response.errcode = "200";
		response.reason_phrase = "OK";			
		response["Content-Type"] = request["content-type"];
		response["Connection"] = "closed";

		response.build_buf(request.get_body(), request.content_length);

		send(tcp_client_socket, (char*)response.get_raw(), static_cast<int>(response.bufsize), 0);

		closesocket(tcp_client_socket);
	}
}