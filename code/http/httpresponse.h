#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
private:
	int code_;
	bool isKeepAlive_;

	std::string path_, srcDir_;
	// 内存映射需求
	char *mmFile_;
	struct stat mmFileStat_;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
	
	void AddStateLine_(Buffer &buffer);
	void AddHeader_(Buffer &buffer);
	void AddContent_(Buffer &buffer);

	void ErrorHtml_();
	std::string GetFileType_();	    

public:
	HttpResponse();
	~HttpResponse();

    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer &buffer);
    void UnmapFile();
    //	返回文件映射到内存中的起始地址
    char* File();
    size_t FileLen() const;
    void ErrorContent(Buffer &buffer, std::string message);
    int Code() const { return code_; }
};

#endif //HTTP_RESPONSE_H