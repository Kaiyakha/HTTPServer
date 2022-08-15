#include <iostream>
#include <string>

const std::string empty;

const std::string& foo(void) { return empty; }

int main() {
	const char* str = "Hello\r\n\r\nWorld!";
	const char* substr = "\r\n\r\n";
	if (std::strstr(str, substr)) std::cout << "Yes!" << std::endl;
}