#include <algorithm>
#include <string>
#include <charconv>
#include "http.hpp"


void http::Request::parse(void) {
	const std::string_view str_view(reinterpret_cast<char*>(raw.get()));

	_header_size = str_view.find(header_end);
	const size_t body_pos = header_size + header_end.size();

	const std::string_view header = str_view.substr(0, header_size);
	const std::vector<std::string_view> header_lines = parse_str(header, header_sep);

	for (size_t i = header_lines[0].size(); i < header_size; i++) raw[i] = std::tolower(static_cast<char>(raw[i]));

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
}


const std::string_view& http::Request::operator[](const std::string& key) {
	if (fields.find(key) != fields.end()) return fields[key];
	else return empty;
}


void http::Request::repr(const bool show_body) const {
	std::cout
		<< method << status_line_sep
		<< uri << status_line_sep
		<< version << header_sep;

	for (const auto& line : fields) {
		std::cout << line.first << field_sep << line.second << '\n';
	}

	if (show_body) {
		std::cout << '\n';
		for (size_t i = 0; i < _content_length; i++) std::cout << body[i];
		std::cout << std::endl;
	}
}


void http::Response::build(const uint8_t* const data) {
	size_t stride = 0;

	for (const auto& status_line_entity : std::vector<std::string>{ version + status_line_sep, errcode + status_line_sep, reason_phrase }) {
		std::memcpy((void*)(raw.get() + stride), status_line_entity.data(), status_line_entity.size());
		stride += status_line_entity.size();
	}

	fields["content-length"] = std::to_string(content_length);
	std::string header_line_str;
	for (const auto& header_line : fields) {
		header_line_str = header_sep + header_line.first + field_sep + header_line.second;
		std::memcpy((void*)(raw.get() + stride), header_line_str.data(), header_line_str.size());
		stride += header_line_str.size();
	}

	std::memcpy((void*)(raw.get() + stride), header_end.data(), header_end.size());
	stride += header_end.size();
	_header_size = stride;

	if (content_length) {
		body = std::shared_ptr<uint8_t[]>(raw, raw.get() + stride);
		std::memcpy((void*)body.get(), data, content_length);
		stride += content_length;
	}

	_bufsize = stride;
}


void http::Response::repr(const bool show_body) const noexcept {
	size_t c;
	for (c = 0; c < header_size; c++) std::cout << raw[c];
	if (show_body) for (; c < bufsize; c++) std::cout << raw[c];
}


static const std::vector<std::string_view>
http::parse_str(const std::string_view& str, const std::string& delim) {
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

	if (substr_front < str.size()) {
		substr = str.substr(substr_front);
		parsed.push_back(substr);
	}

	return parsed;
}
