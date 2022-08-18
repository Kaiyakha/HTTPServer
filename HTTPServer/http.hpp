#pragma once
#include <iostream>
#include <string_view>
#include <vector>
#include <map>
#ifdef __linux__
#include <memory>
#include <cstring>
#endif


namespace http {

const std::string status_line_sep(" ");
const std::string header_sep("\r\n");
const std::string field_sep(": ");
const std::string header_end("\r\n\r\n");
const std::string_view empty;


template<typename string_t>
class HTTP_base {
protected:
	std::map<const string_t, string_t> fields;
	std::shared_ptr<uint8_t[]> raw, body;
	size_t _header_size = 0, _content_length = 0;

public:
	const decltype(_header_size)& header_size = _header_size;

	HTTP_base(const size_t bufsize);
	inline const char* get_raw(void) const { return reinterpret_cast<const char*>(raw.get()); }

	virtual const string_t& operator[](const std::string& key) = 0;
	virtual void repr(const bool show_body) const = 0;
};

template<typename string_t>
HTTP_base<string_t>::HTTP_base(const size_t bufsize) {
	raw = std::shared_ptr<uint8_t[]>(new uint8_t[bufsize]);
	std::memset(raw.get(), '\0', bufsize);
}


class Request : public HTTP_base<std::string_view> {
private:
	std::string_view _method, _uri, _version;

public:
	const decltype(_method)& method = _method;
	const decltype(_uri)& uri = _uri;
	const decltype(_version)& version = _version;
	const decltype(_content_length)& content_length = _content_length;

	using HTTP_base::HTTP_base;
	void parse(void);
	const std::string_view& operator[](const std::string& key) override;
	inline const uint8_t* const get_body(void) const { return reinterpret_cast<const uint8_t* const>(body.get()); }
	void repr(const bool show_body = false) const override;
};


class Response : public HTTP_base<std::string> {
private:
	size_t _bufsize;

public:
	std::string version, errcode, reason_phrase;
	const decltype(_bufsize)& bufsize = _bufsize;
	decltype(_content_length)& content_length = _content_length;

	using HTTP_base::HTTP_base;
	inline std::string& operator[](const std::string& key) override { return fields[key]; };
	void build(const uint8_t* const data);
	void repr(const bool show_body = false) const noexcept override;
};


static const std::vector<std::string_view>
parse_str(const std::string_view& str, const std::string& delim);


} // namespace http
