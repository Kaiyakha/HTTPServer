#pragma once
#include <iostream>

#if defined(_WIN32)

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#define GETSOCKETERRNO() WSAGetLastError()
#define TERMINATE_PROCESS(exit_status) ExitProcess(exit_status)
#define socklen_t int

#elif defined(__linux__)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define GETSOCKETERRNO() errno
#define TERMINATE_PROCESS(exit_status) std::exit(exit_status)
#define SOCKET int
#define SOCKADDR_IN sockaddr_in
#define SOCKADDR sockaddr
#define INVALID_SOCKET -1
#define SOCKET_ERROR (SOCKET)(~0)
#define closesocket close

#endif


// A function to handle socket errors
template <typename T>
static T handle_error(T returned_val, T err_val, const std::string& message) {
	if (returned_val == err_val) {
		std::cout << message << " Error code: " << GETSOCKETERRNO() << std::endl;
		TERMINATE_PROCESS(EXIT_FAILURE);
	}
	else return returned_val;
}
#define HANDLE_SOCKET_ERROR(returned_val, message) handle_error<decltype(SOCKET_ERROR)>(returned_val, SOCKET_ERROR, message)
#define HANDLE_INVALID_SOCKET(returned_val, message) handle_error<decltype(INVALID_SOCKET)>(returned_val, INVALID_SOCKET, message)
#ifdef _WIN32
#define HANDLE_DLL_LOAD_ERROR(returned_val) handle_error<decltype(returned_val)>(returned_val, !0, "Failed to load ws2_32.dll")
#endif