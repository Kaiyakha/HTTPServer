#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <fstream>
#include <charconv>
#include "http.hpp"
#include "image.hpp"

#define BUFSIZE 1024 * 1024 * 8


static void process_request(const http::Request& request, http::Response& response, std::unique_ptr<uint8_t[]>& buffer);


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

		http::Request request(BUFSIZE);

		int stride = 0;
		do stride += recv(tcp_client_socket, (char*)(request.get_raw() + stride), BUFSIZE - stride, 0);
		while (std::strstr((const char*)request.get_raw(), http::header_end.c_str()) == nullptr);

		request.parse();
		request.repr(false);

		while (stride < request.header_size + request.content_length)
			stride += recv(tcp_client_socket, (char*)(request.get_raw() + stride), BUFSIZE - stride, 0);

		http::Response response(BUFSIZE);
		response.version = request.version;
		response["server"] = inet_ntoa(tcp_server.sin_addr);
		response["content-type"] = request["content-type"];

		std::unique_ptr<uint8_t[]> buffer(new uint8_t[BUFSIZE]);
		
		process_request(request, response, buffer);

		response.build_buf(buffer.get());

		send(tcp_client_socket, (char*)response.get_raw(), static_cast<int>(response.bufsize), 0);

		closesocket(tcp_client_socket);
	}
}


static void process_request(const http::Request& request, http::Response& response, std::unique_ptr<uint8_t[]>& buffer) {
	if (request.method != "POST") {
		response.errcode = "405";
		response.reason_phrase = "Method Not Allowed";
		return;
	}
	if (request.content_length == 0) {
		response.errcode = "400";
		response.reason_phrase = "Bad Request";
		return;
	}

	const bool success = mirror_jpg(request.get_body(), buffer.get(), request.content_length, response.content_length);

	if (success) { response.errcode = "200"; response.reason_phrase = "OK"; }
	else { response.errcode = "500"; response.reason_phrase = "Internal Server Error"; }
}