#pragma once
#include <iostream>
#include <string_view>
#include <vector>
#include <map>

const std::string header_end("\r\n\r\n");


template<typename string_t>
class HTTP_base {
protected:
	std::map<const string_t, string_t> fields;
	std::shared_ptr<uint8_t[]> raw, body;
	size_t _header_size, _content_length;

public:
	HTTP_base(const size_t bufsize);
	inline const char* get_raw(void) const { return reinterpret_cast<const char*>(raw.get()); }

	virtual const string_t& operator[](const std::string& key) = 0;

	const decltype(_header_size)& header_size = _header_size;
	const decltype(_content_length)& content_length = _content_length;
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
	using HTTP_base::HTTP_base;
	void parse(void);
	const std::string_view& operator[](const std::string& key) override;
	inline const uint8_t* const get_body(void) const { return reinterpret_cast<const uint8_t* const>(body.get()); }
	void repr(const bool show_body) const;

	const decltype(_method)& method = _method;
	const decltype(_uri)& uri = _uri;
	const decltype(_version)& version = _version;
};


class Response : public HTTP_base<std::string> {
private:
	size_t _bufsize;

public:
	std::string version, errcode, reason_phrase;
	const decltype(_bufsize)& bufsize = _bufsize;

	using HTTP_base::HTTP_base;
	inline std::string& operator[](const std::string& key) override { return fields[key]; };
	void build_buf(const uint8_t* const data, const size_t size);
};