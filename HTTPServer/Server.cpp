#include <fstream>
#include <charconv>
#include "http.hpp"
#include "image.hpp"
#include "handle_error.hpp"

#define BUFSIZE 1024 * 1024 * 32
#define ARGC_REQUIRED 3

static void process_request(const http::Request& request, http::Response& response, std::unique_ptr<uint8_t[]>& buffer);


int main(const int argc, const char* const argv[]) {
	if (argc != ARGC_REQUIRED) {
		std::cout << "Invalid argument count!" << std::endl;
		TERMINATE_PROCESS(EXIT_FAILURE);
	}
	const char* IP = argv[1];
	const unsigned short PORT = static_cast<unsigned short>(std::strtoul(argv[2], nullptr, 10));

#	ifdef _WIN32
	const WORD WinSockDLLVersion = MAKEWORD(2, 2);
	WSADATA wsa;
	HANDLE_DLL_LOAD_ERROR(WSAStartup(WinSockDLLVersion, &wsa));
#	endif

	SOCKET tcp_server_socket, tcp_client_socket;
	SOCKADDR_IN tcp_server, tcp_client;
	socklen_t tcp_client_size = sizeof tcp_client;

	tcp_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	HANDLE_INVALID_SOCKET(tcp_server_socket, "Failed to open a TCP socket.");

	tcp_server.sin_family = AF_INET;
	tcp_server.sin_addr.s_addr = inet_addr(IP);
	tcp_server.sin_port = htons(PORT);
	HANDLE_SOCKET_ERROR(bind(tcp_server_socket, (const SOCKADDR*)&tcp_server, sizeof tcp_server), "Failed to bind the socket.");

	HANDLE_SOCKET_ERROR(listen(tcp_server_socket, 0), "Failed to listen.");

	bool listening = true;
	while (listening) {
		std::cout << "\nWaiting for incoming connections...\n";

		tcp_client_socket = accept(tcp_server_socket, (SOCKADDR*)&tcp_client, &tcp_client_size);
		HANDLE_INVALID_SOCKET(tcp_client_socket, "Failed to accept a valid client socket.");
		std::cout << "\nIP " << inet_ntoa(tcp_client.sin_addr) << " has connected to the server.\n";

		http::Request request(BUFSIZE);

		int stride = 0;
		do stride += HANDLE_SOCKET_ERROR(
			recv(tcp_client_socket, (char*)(request.get_raw() + stride), BUFSIZE - stride, 0),
			"Failed to receive data.");
		while (std::strstr((const char*)request.get_raw(), http::header_end.c_str()) == nullptr);

		request.parse();
		//request.repr();

		while (stride < request.header_size + request.content_length)
			stride += HANDLE_SOCKET_ERROR(
				recv(tcp_client_socket, (char*)(request.get_raw() + stride), BUFSIZE - stride, 0),
				"Failed to receive data.");

		http::Response response(BUFSIZE);
		std::unique_ptr<uint8_t[]> buffer(new uint8_t[BUFSIZE]);
		
		std::cout << "Processing request...\n";
		process_request(request, response, buffer);

		response.version = request.version;
		response["server"] = inet_ntoa(tcp_server.sin_addr);
		response["content-type"] = request["content-type"];
		response["connection"] = "closed";

		response.build(buffer.get());
		//response.repr();

		HANDLE_SOCKET_ERROR(
			send(tcp_client_socket, (char*)response.get_raw(), static_cast<int>(response.bufsize), 0),
			"Failed to send data.");

		std::cout << "Done!\n";

		closesocket(tcp_client_socket);
	}

	closesocket(tcp_server_socket);
	
#	ifdef _WIN32
	WSACleanup();
#	endif
}


static void process_request(const http::Request& request, http::Response& response, std::unique_ptr<uint8_t[]>& buffer) {
	if (request.method != "POST") {
		response.errcode = "405";
		response.reason_phrase = "Method Not Allowed";
		return;
	}
	if (request.content_length == 0 || request.content_length + request.header_size > BUFSIZE) {
		response.errcode = "400";
		response.reason_phrase = "Bad Request";
		return;
	}

	const bool success = mirror_jpg(request.get_body(), buffer.get(), request.content_length, response.content_length);

	if (success) { response.errcode = "200"; response.reason_phrase = "OK"; }
	else { response.errcode = "500"; response.reason_phrase = "Internal Server Error"; }
}
