#include "httprequest.h"
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index", 
    "/register", 
    "/login",
    "/welcome", 
    "/video", 
    "/picture", 
};

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
    {"/register.html", 0},
    {"/login.html", 1},  
};

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    if (header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" &&
               version_ == "1.1";
    }
    return false;
}

bool HttpRequest::Parse(Buffer &buffer) {
    const char CRLF[] = "\r\n";
    if (buffer.ReadableBytes() <= 0) return false;

    while (buffer.ReadableBytes() && state_ != FINISH) {
        // 搜索第一次出现 CRLF 的位置
        const char *lineEnd = search(buffer.ReadPtrConst(), 
                                     buffer.WritePtrConst(),
                                     CRLF, CRLF+2);
        std::string line(buffer.ReadPtrConst(), lineEnd);
        switch(state_) {
        case REQUEST_LINE:
            // 解析状态行
            if (!ParseRequestLine_(line)) {
                return false;
            }
            // 解析路径资源
            ParsePath_();
            break;
        case HEADERS:
            // 解析请求头
            ParseHeader_(line);
            // 当前缓存区可读的内容少于两个字节，说明 Header 之后没有消息体，将状态改为 FINISH
            if (buffer.ReadableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            // 可读的内容大于两个字节，解析请求正文
            ParseBody_(line);
            break;
        default:
            break;
        }
        // 否则执行下一段内容，以 CRLF 结尾
        if (lineEnd == buffer.WritePtr()) {
            break;
        }
        // 读取 CRLF
        buffer.ReadUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void HttpRequest::ParsePath_() {
    if (path_ == "/") {
        path_ = "/index.html";
    } else {
        for (auto &item : DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

// bool HttpRequest::ParseRequestLine_(const string &line) {
//     regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
//     // 正则表达式匹配，所有匹配结果存储在 smatch 结构体中
//     smatch subMatch;
//     if(regex_match(line, subMatch, patten)) {   
//         method_ = subMatch[1];
//         path_ = subMatch[2];
//         version_ = subMatch[3];
//         // 行解析完毕，状态转移至 HEADERS
//         state_ = HEADERS;
//         return true;
//     }
//     LOG_ERROR("RequestLine Error");
//     return false; 
// }

bool HttpRequest::ParseRequestLine_(const std::string &line) {
    // 找到第一个空格的位置
    size_t method_end = line.find(' ');
    if (method_end == std::string::npos) {
        LOG_ERROR("RequestLine Error: No space found for method");
        return false;
    }

    // 找到第二个空格的位置，从第一个空格后开始查找
    size_t path_end = line.find(' ', method_end + 1);
    if (path_end == std::string::npos) {
        LOG_ERROR("RequestLine Error: No space found for path");
        return false;
    }

    // 解析 Method, Path, 和 HTTP Version
    method_ = line.substr(0, method_end);
    path_ = line.substr(method_end + 1, path_end - method_end - 1);
    version_ = line.substr(path_end + 1);

    // 检查版本号是否以 "HTTP/" 开头
    if (version_.substr(0, 5) != "HTTP/") {
        LOG_ERROR("RequestLine Error: Invalid HTTP version");
        return false;
    }

    // 如果解析成功，状态转移至 HEADERS
    state_ = HEADERS;
    return true;
}


// void HttpRequest::ParseHeader_(const string &line) {
//     regex patten("^([^:]*): ?(.*)$");
//     // regex patten("^([^:]*): ?(.*)$");
//     smatch subMatch;
//     if(regex_match(line, subMatch, patten)) {
//         header_[subMatch[1]] = subMatch[2];
//     }
//     else {
//         // 如果匹配失败则说明已经读完请求头部，将当前状态设置为 body
//         state_ = BODY;
//     }   
// }

void HttpRequest::ParseHeader_(const std::string &line) {
    size_t pos = line.find(':');
    if (pos != std::string::npos) {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // 去掉键和值的前后空格
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        header_[key] = value;
        // 这里添加逻辑处理 key 和 value
        // std::cout << "Key: " << key << ", Value: " << value << std::endl;
    } else {
        state_ = BODY;
    }
}

// 解析 body 等同于解析 post，解析完成状态转移为 Finish
void HttpRequest::ParseBody_(const string &line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

void HttpRequest::ParsePost_() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded_();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            } 
        }
    }
}

// 解析消息体 urlencoded 格式
// title=ahweg&sub%5B%5D=1&sub%5B%5D=2&sub%5B%5D=3
// 非 ascii 字符做百分号编码
void HttpRequest::ParseFromUrlencoded_() {
    if (body_.size() == 0) return;

    string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; ++i) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i-j);
            j = i+1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i-j);
            j = i+1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break; 
        }
    }
    assert(j <= i);
    if (post_.count(key) == 0 && j < i) {
        value = body_.substr(j, i-j);
        post_[key] = value; 
    }
}

bool HttpRequest::UserVerify(const string &name, const string &pwd, bool isLogin) {
    if (name == "" || pwd == "") return false;

    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());  

    // 从连接池中取 sql 连接
    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    if (!sql) {
        return false;
    }
    // assert(sql);

    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!isLogin) {
        flag = true;
    }

    snprintf(order, 256, "SELECT username, passwd FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    // 如果查询成功，返回 0
    if (mysql_query(sql, order)) {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]); 
        string password(row[1]);
        if (isLogin) {
            if (pwd == password) {
                flag = true;
            } else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        } else {
            flag = false;
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    if (!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, passwd) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if (mysql_query(sql, order)) {
            LOG_DEBUG( "Insert error!");
            flag = false;
        }
        flag = true;
    }
    // SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!");
    return flag; 
}


std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}


