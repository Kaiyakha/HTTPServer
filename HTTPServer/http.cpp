#include <algorithm>
#include <string>
#include <charconv>
#include "http.hpp"

#define BUFSIZE 1024 * 1024 * 32

const std::string status_line_sep(" ");
const std::string header_sep("\r\n");
const std::string field_sep(": ");
const std::string_view empty;

static const std::vector<std::string_view>
parse_str(const std::string_view& str, const std::string& delim);


void Request::parse(void) {
	const std::string_view str_view(reinterpret_cast<char*>(raw.get()));

	_header_size = str_view.find(header_end);
	const size_t body_pos = header_size + header_end.size();

	for (size_t i = 0; i < header_size; i++) raw[i] = std::tolower(static_cast<char>(raw[i]));

	const std::string_view header = str_view.substr(0, header_size);
	const std::vector<std::string_view> header_lines = parse_str(header, header_sep);

	const std::vector<std::string_view> status_line = parse_str(header_lines[0], status_line_sep);
	_method = status_line[0]; _uri = status_line[1]; _version = status_line[2];

	std::vector<std::string_view> line_v;
	for (const auto& line_str : std::vector<std::string_view>(header_lines.begin() + 1, header_lines.end())) {
		line_v = parse_str(line_str, field_sep);
		fields[line_v[0]] = line_v[1];
	}

	const std::string_view& content_length_str = this->operator[]("content-length");
	if (content_length_str != empty) {
		std::from_chars(content_length_str.data(), content_length_str.data() + content_length_str.size(), _content_length);
		body = std::shared_ptr<uint8_t[]>(raw, raw.get() + body_pos);
	}
	else _content_length = 0;
}


const std::string_view& Request::operator[](const std::string& key) {
	if (fields.find(key) != fields.end()) return fields[key];
	else return empty;
}


void Request::repr(const bool show_body) const {
	std::cout
		<< "Method: " << method
		<< "\nURI: " << uri
		<< "\nVersion: " << version << '\n';

	for (const auto& line : fields) {
		std::cout << line.first << field_sep << line.second << '\n';
	}

	if (show_body) {
		std::cout << '\n';
		for (size_t i = 0; i < _content_length; i++) std::cout << body[i];
		std::cout << std::endl;
	}
}

static const std::vector<std::string_view>
parse_str(const std::string_view& str, const std::string& delim) {
	std::vector<std::string_view> parsed;
	size_t substr_front = 0, substr_count = str.find(delim);

	std::string_view substr;
	while (substr_count <= str.size()) {
		if (substr_count) {
			substr = str.substr(substr_front, substr_count);
			parsed.push_back(substr);
		}
		substr_front += substr_count + delim.size();
		substr_count = str.find(delim, substr_front) - substr_front;
	}

	if (substr_front < str.size() - 1) {
		substr = str.substr(substr_front);
		parsed.push_back(substr);
	}

	return parsed;
}


void Response::build_buf(const uint8_t* const data, const size_t size) {
	size_t stride = 0;

	for (const auto& status_line_entity : std::vector<std::string>{ version + status_line_sep, errcode + status_line_sep, reason_phrase }) {
		std::memcpy((void*)(raw.get() + stride), status_line_entity.data(), status_line_entity.size());
		stride += status_line_entity.size();
	}

	fields["content-length"] = std::to_string(size);
	std::string header_line_str;
	for (const auto& header_line : fields) {
		header_line_str = header_sep + header_line.first + field_sep + header_line.second;
		std::memcpy((void*)(raw.get() + stride), header_line_str.data(), header_line_str.size());
		stride += header_line_str.size();
	}

	std::memcpy((void*)(raw.get() + stride), header_end.data(), header_end.size());
	stride += header_end.size();

	if (size) {
		body = std::shared_ptr<uint8_t[]>(raw, raw.get() + stride);
		std::memcpy((void*)body.get(), data, size);
		stride += size;
	}

	_bufsize = stride;
}





//int main() {
//	
//	const char* request_buf =
//		"post / http/1.1\r\n"
//		"host: foo.com\r\n"
//		"content-type: application/x-www-form-urlencoded\r\n"
//		"content-length: 13\r\n\r\n"
//
//		"say = hi & to = mom";
//
//	Request request(BUFSIZE);
//
//	memcpy((void*)request.get_raw(), request_buf, strlen(request_buf) + 1);
//	request.parse();
//	request.repr(false);
//}