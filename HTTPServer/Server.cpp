#include <fstream>
#include <charconv>
#include "http.hpp"
#include "image.hpp"
#include "handle_error.hpp"

// Define buffer size (32Mb) and expected argument count (IP and PORT)
#define BUFSIZE 1024 * 1024 * 32
#define ARGC_REQUIRED 3

// A function to process request and fill in response fields accordingly
static void process_request(const http::Request& request, http::Response& response, std::unique_ptr<uint8_t[]>& buffer);


int main(const int argc, const char* const argv[]) {
	if (argc != ARGC_REQUIRED) {
		std::cout << "Invalid argument count!" << std::endl;
		TERMINATE_PROCESS(EXIT_FAILURE);
	}
	const char* IP = argv[1];
	const unsigned short PORT = static_cast<unsigned short>(std::strtoul(argv[2], nullptr, 10));

	// If built on Windows, load ws2_32.dll
#	ifdef _WIN32
	const WORD WinSockDLLVersion = MAKEWORD(2, 2);
	WSADATA wsa;
	HANDLE_DLL_LOAD_ERROR(WSAStartup(WinSockDLLVersion, &wsa));
#	endif

	// Define sockets and address structures
	SOCKET tcp_server_socket, tcp_client_socket;
	SOCKADDR_IN tcp_server, tcp_client;
	socklen_t tcp_client_size = sizeof tcp_client;

	// Open a TCP socket
	tcp_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	HANDLE_INVALID_SOCKET(tcp_server_socket, "Failed to open a TCP socket.");

	// Bind the socket to IP and PORT
	tcp_server.sin_family = AF_INET;
	tcp_server.sin_addr.s_addr = inet_addr(IP);
	tcp_server.sin_port = htons(PORT);
	HANDLE_SOCKET_ERROR(bind(tcp_server_socket, (const SOCKADDR*)&tcp_server, sizeof tcp_server), "Failed to bind the socket.");

	// Start listening
	HANDLE_SOCKET_ERROR(listen(tcp_server_socket, 0), "Failed to listen.");

	bool listening = true;
	while (listening) {
		std::cout << "\nWaiting for incoming connections...\n";

		// Accept a client socket
		tcp_client_socket = accept(tcp_server_socket, (SOCKADDR*)&tcp_client, &tcp_client_size);
		HANDLE_INVALID_SOCKET(tcp_client_socket, "Failed to accept a valid client socket.");
		std::cout << "\nIP " << inet_ntoa(tcp_client.sin_addr) << " has connected to the server.\n";

		// HTTP request parser
		http::Request request(BUFSIZE);

		// Keep receiving byte stream until HTTP header is received
		int stride = 0;
		do stride += HANDLE_SOCKET_ERROR(
			recv(tcp_client_socket, (char*)(request.get_raw() + stride), BUFSIZE - stride, 0),
			"Failed to receive data.");
		while (std::strstr((const char*)request.get_raw(), http::header_end.c_str()) == nullptr);

		// Parse HTTP request
		request.parse();

		// Output the parsed request to console (I used it for debugging)
		// request.repr();

		// If the content is present and hasn't been received yet, proceed to receiving byte stream
		while (stride < request.header_size + request.content_length)
			stride += HANDLE_SOCKET_ERROR(
				recv(tcp_client_socket, (char*)(request.get_raw() + stride), BUFSIZE - stride, 0),
				"Failed to receive data.");

		// Empty HTTP response
		http::Response response(BUFSIZE);

		// This buffer stores processed data (mirrored JPEG image)
		std::unique_ptr<uint8_t[]> buffer(new uint8_t[BUFSIZE]);
		
		std::cout << "Processing request...\n";
		process_request(request, response, buffer);

		// Fill in response fields
		response.version = request.version;
		response["server"] = inet_ntoa(tcp_server.sin_addr);
		response["content-type"] = request["content-type"];
		response["connection"] = "closed";

		// Build a contiguous buffer with all response data (header and body)
		response.build(buffer.get());
		//response.repr();

		// Send response buffer via TCP
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
	// The server rejects any methods but POST
	if (request.method != "POST") {
		response.errcode = "405";
		response.reason_phrase = "Method Not Allowed";
		return;
	}
	// If the request has no body or the body is too big, issue Bad Request
	if (request.content_length == 0 || request.content_length + request.header_size > BUFSIZE) {
		response.errcode = "400";
		response.reason_phrase = "Bad Request";
		return;
	}

	// Interpret the received body as a JPEG image, try to reflect
	const bool success = mirror_jpg(request.get_body(), buffer.get(), request.content_length, response.content_length);

	// Fill in response fields accordingly
	if (success) { response.errcode = "200"; response.reason_phrase = "OK"; }
	else { response.errcode = "500"; response.reason_phrase = "Internal Server Error"; }
}
