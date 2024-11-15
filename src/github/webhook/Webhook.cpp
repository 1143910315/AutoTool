#include "regex/Pcre2Implementation.h"
#include "utils/Defer.hpp"
#include "Webhook.h"
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <unordered_map>

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
    std::unordered_map<std::string, std::string> structMap;
    auto replaceResult = regex::Pcre2Implementation("(\\n)\\r").replace(content, "$1");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    replaceResult = regex::Pcre2Implementation("^package .*").replace(content, "");
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
    replaceResult = regex::Pcre2Implementation("//.*?$").replace(content, "");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    replaceResult = regex::Pcre2Implementation("^(\\s*)\\S+\\s+(\\S+)\\s+`json:\"(\\S+?)(,\\S*)?\"`").replace(content, "${1}${2} ${3};");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    // replaceResult = regex::Pcre2Implementation("^(\\s*)(\\S+)\\s+struct").replace(content, "${1}struct ${2}");
    // if (replaceResult) {
    // content = replaceResult.value();
    // } else {
    // std::cerr << replaceResult.error() << std::endl;
    // }
    // replaceResult = regex::Pcre2Implementation("^(\\s*)type\\s*(\\S+)\\s+struct").replace(content, "${1}struct ${2}");
    // if (replaceResult) {
    // content = replaceResult.value();
    // } else {
    // std::cerr << replaceResult.error() << std::endl;
    // }
    replaceResult = regex::Pcre2Implementation("(interface\\{\\})|(struct\\{\\})").replace(content, "nlohmann::json");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    std::vector<regex::Pcre2Implementation::ReplaceInfo> replaceInfoList;
    auto extractStructRegex = regex::Pcre2Implementation("^(\\s*)\\S+\\s+struct[^\\{]+\\{(?<body>[^\\{\\}]+)\\}\\s*`json:\"(?<name>\\S+?)(,\\S*)?\"`");
    auto extractArrayStructRegex = regex::Pcre2Implementation("^(\\s*)\\S+\\s+\\[\\]struct[^\\{]+\\{(?<body>[^\\{\\}]+)\\}\\s*`json:\"(?<name>\\S+?)(,\\S*)?\"`");
    auto extractOptionalStructRegex = regex::Pcre2Implementation("^(\\s*)\\S+\\s+\\*struct[^\\{]+\\{(?<body>[^\\{\\}]+)\\}\\s*`json:\"(?<name>\\S+?)(,\\S*)?\"`");
    auto extractOutputStructRegex = regex::Pcre2Implementation("^(\\s*)Output\\s+struct[^\\{]+\\{(?<body>[^\\{\\}]+)\\}\\s*$");
    auto extractPackageStructRegex = regex::Pcre2Implementation("^(\\s*)Package\\s+struct[^\\{]+\\{(?<body>[^\\{\\}]+)\\}\\s*$");
    bool running = false;
    do {
        running = false;
        replaceResult = extractStructRegex.replace(content, "${1}${3} ${3};", replaceInfoList);
        if (replaceResult) {
            content = replaceResult.value();
            for (auto&replaceInfo : replaceInfoList) {
                structMap.emplace(replaceInfo.group[replaceInfo.namedGroup["name"]].text, replaceInfo.group[replaceInfo.namedGroup["body"]].text);
            }
        } else {
            std::cerr << replaceResult.error() << std::endl;
        }
        running = replaceInfoList.size() > 0;
        replaceResult = extractArrayStructRegex.replace(content, "${1}std::vector<${3}> ${3};", replaceInfoList);
        if (replaceResult) {
            content = replaceResult.value();
            for (auto& replaceInfo : replaceInfoList) {
                structMap.emplace(replaceInfo.group[replaceInfo.namedGroup["name"]].text, replaceInfo.group[replaceInfo.namedGroup["body"]].text);
            }
        } else {
            std::cerr << replaceResult.error() << std::endl;
        }
        running = running || replaceInfoList.size() > 0;
        replaceResult = extractOptionalStructRegex.replace(content, "${1}std::optional<${3}> ${3};", replaceInfoList);
        if (replaceResult) {
            content = replaceResult.value();
            for (auto& replaceInfo : replaceInfoList) {
                structMap.emplace(replaceInfo.group[replaceInfo.namedGroup["name"]].text, replaceInfo.group[replaceInfo.namedGroup["body"]].text);
            }
        } else {
            std::cerr << replaceResult.error() << std::endl;
        }
        running = running || replaceInfoList.size() > 0;
        replaceResult = extractOutputStructRegex.replace(content, "${1}std::optional<output> output;", replaceInfoList);
        if (replaceResult) {
            content = replaceResult.value();
            for (auto& replaceInfo : replaceInfoList) {
                structMap.emplace("output", replaceInfo.group[replaceInfo.namedGroup["body"]].text);
            }
        } else {
            std::cerr << replaceResult.error() << std::endl;
        }
        running = running || replaceInfoList.size() > 0;
        replaceResult = extractPackageStructRegex.replace(content, "${1}std::optional<package> package;", replaceInfoList);
        if (replaceResult) {
            content = replaceResult.value();
            for (auto& replaceInfo : replaceInfoList) {
                structMap.emplace("package", replaceInfo.group[replaceInfo.namedGroup["body"]].text);
            }
        } else {
            std::cerr << replaceResult.error() << std::endl;
        }
        running = running || replaceInfoList.size() > 0;
    } while (running);
    // replaceResult = regex::Pcre2Implementation("\\*(\\S+)(\\s+)(\\s+;)").replace(content, "std::optional<${1}>${2}${3}");
    // if (replaceResult) {
    // content = replaceResult.value();
    // } else {
    // std::cerr << replaceResult.error() << std::endl;
    // }
    // replaceResult = regex::Pcre2Implementation("time\\.Time").replace(content, "string");
    // if (replaceResult) {
    // content = replaceResult.value();
    // } else {
    // std::cerr << replaceResult.error() << std::endl;
    // }
    // replaceResult = regex::Pcre2Implementation("(?<!\\S)string").replace(content, "std::string");
    // if (replaceResult) {
    // content = replaceResult.value();
    // } else {
    // std::cerr << replaceResult.error() << std::endl;
    // }
    // replaceResult = regex::Pcre2Implementation("(?<!\\S)[](\\S+)").replace(content, "std::vector<${1}>");
    // if (replaceResult) {
    // content = replaceResult.value();
    // } else {
    // std::cerr << replaceResult.error() << std::endl;
    // }
    // replaceResult = regex::Pcre2Implementation("\\}.*?$").replace(content, "};");
    // if (replaceResult) {
    // content = replaceResult.value();
    // } else {
    // std::cerr << replaceResult.error() << std::endl;
    // }
    //
    // auto findResult = regex::Pcre2Implementation("(string)(\\s+)(\\S+;)(\\s+)//(.*?)$").find(content);
    // if (findResult) {
    // } else {
    // std::cerr << replaceResult.error() << std::endl;
    // }
    // replaceResult = regex::Pcre2Implementation("\\}.*?$").replace(content, "};");
    // if (replaceResult) {
    // content = replaceResult.value();
    // } else {
    // std::cerr << replaceResult.error() << std::endl;
    // }
    return content;
}
