#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>     
#include <mysql/mysql.h>  // mysql

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
private:
	// PARSE_STATE state_;
	std::string method_, path_, version_, body_;
	std::unordered_map<std::string, std::string> header_;
	std::unordered_map<std::string, std::string> post_;

	static const std::unordered_set<std::string> DEFAULT_HTML;
	static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
	
    bool ParseRequestLine_(const std::string &line);
    void ParseHeader_(const std::string &line);
    void ParseBody_(const std::string &line);

    void ParsePath_();
    void ParsePost_();
    void ParseFromUrlencoded_();

    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);
    static int ConverHex(char ch);

public:
	enum PARSE_STATE {
		REQUEST_LINE,
		HEADERS,
		BODY,
		FINISH,
	};

	enum HTTP_CODE {
		NO_REQUEST = 0,
		GET_REQUEST,
		BAD_REQUEST,
		NO_RESOURSE,
		FORBIDDENT_REQUEST,
		FILE_REQUEST,
		INTERNAL_ERROR,
		CLOSED_CONNECTION,
	};
	// Init() 并没有多的操作，所以 HttpRequest 等同于 Init
	HttpRequest() { Init(); }
	~HttpRequest() = default;

	void Init();
	bool Parse(Buffer &buffer);

	std::string& Path() { return path_; }
	std::string Path() const { return path_; }
	std::string Method() const { return method_; }
	std::string Version() const { return version_; }
	std::string GetPost(const std::string &key) const;
	std::string GetPost(const char *key) const;

	bool IsKeepAlive() const;

private:
	PARSE_STATE state_;
};

#endif //HTTP_REQUEST_H