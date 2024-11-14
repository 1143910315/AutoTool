#include "regex/Pcre2Implementation.h"
#include "utils/Defer.hpp"
#include "Webhook.h"
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>

std::string github::webhook::Webhook::transform(std::string fileName) {
    // 创建ifstream对象并打开文件
    auto inputFile = std::ifstream(fileName);

    if (!inputFile.is_open()) {
        std::cerr << "无法打开文件: " << fileName << std::endl;
        return "";
    }
    utils::Defer defer = [&inputFile]() {
            inputFile.close();
        };
    std::string content((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());

    auto replaceResult = regex::Pcre2Implementation("^package .*").replace(content, "");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    replaceResult = regex::Pcre2Implementation("^import .*").replace(content, "");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    replaceResult = regex::Pcre2Implementation("^// .*").replace(content, "");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    auto findResult = regex::Pcre2Implementation("^((?![\\n\\r])\\s*)\\S+((?![\\n\\r])\\s)+(\\S+)((?![\\n\\r])\\s)+`json:\"(\\S*)\"`").find(content, 1);
    if (findResult) {
    } else {
        std::cerr << findResult.error() << std::endl;
    }
    findResult = regex::Pcre2Implementation("^(\\s*)\\S+(\\s)+(\\S+)(\\s)+`json:\"(\\S*)\"`").find(content, 1);
    if (findResult) {
    } else {
        std::cerr << findResult.error() << std::endl;
    }
    findResult = regex::Pcre2Implementation("(\\n)^(\\s*).+(\\s)+(.+)(\\s)+`json:\"(.*)\"`").find(content, 10);
    if (findResult) {
    } else {
        std::cerr << findResult.error() << std::endl;
    }
    replaceResult = regex::Pcre2Implementation("^((?![\\n\\r])\\s*)\\S+((?![\\n\\r])\\s)+(\\S+)((?![\\n\\r])\\s)+`json:\"(\\S*)\"`").replace(content, "${1}${3} ${5};");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    replaceResult = regex::Pcre2Implementation("^((?![\\n\\r])\\s*)\\S+((?![\\n\\r])\\s)+(\\S+)((?![\\n\\r])\\s)+`json:\"(\\S*)\"`").replace(content, "${1}${3} ${5};");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    replaceResult = regex::Pcre2Implementation("^((?![\\n\\r])\\s*)\\S+((?![\\n\\r])\\s)+(\\S+)((?![\\n\\r])\\s)+`json:\"(\\S*)\"`").replace(content, "${1}${3} ${5};");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    return content;
}
