#include "regex/Pcre2Implementation.h"
#include "utils/Defer.hpp"
#include "Webhook.h"
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <ranges>
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
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> structMap;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> eventMap;
    auto findResult = regex::Pcre2Implementation("").find(content, 1);
    if (findResult) {
    } else {
        std::cerr << findResult.error() << std::endl;
    }
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
    auto extractNamedStructRegex = regex::Pcre2Implementation("^(\\s*)(?<name>\\S+?)\\s+struct[^\\{]+\\{(?<body>[^\\{\\}]+)\\}\\s*$");
    auto structFieldRegex = regex::Pcre2Implementation("^\\s*(?<type>\\S+)\\s*(?<name>\\S+);\\s*$");
    auto goStringTypeRegex = regex::Pcre2Implementation("string");
    auto goTimeTypeRegex = regex::Pcre2Implementation("time\\.Time");
    auto goOptionalTypeRegex = regex::Pcre2Implementation("(\\S*)\\*(\\S+)");
    auto goArrayTypeRegex = regex::Pcre2Implementation("\\[\\](\\S+)");
    auto decodeFieldFunction = [&](const std::string& name, const std::string& body, std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& structFieldMap) {
            auto [iterator, success] = structFieldMap.try_emplace(name, std::unordered_map<std::string, std::string>());
            auto& [nameKey, fieldMap] = *iterator;
            auto findResult = structFieldRegex.find(body);
            if (findResult) {
                for (auto& findInfo : findResult.value()) {
                    auto& typeString = findInfo.group[findInfo.namedGroup["type"]].text;
                    typeString = goStringTypeRegex.replace(typeString, "std::string").value_or(typeString);
                    typeString = goTimeTypeRegex.replace(typeString, "std::string").value_or(typeString);
                    typeString = goOptionalTypeRegex.replace(typeString, "std::optional<${1}${2}>").value_or(typeString);
                    typeString = goArrayTypeRegex.replace(typeString, "std::vector<${1}>").value_or(typeString);
                    fieldMap.try_emplace(findInfo.group[findInfo.namedGroup["name"]].text, typeString);
                }
            } else {
                std::cerr << findResult.error() << std::endl;
            }
        };
    bool running = false;
    do {
        running = false;
        replaceResult = extractStructRegex.replace(content, "${1}${3} ${3};", replaceInfoList);
        if (replaceResult) {
            content = replaceResult.value();
            for (auto& replaceInfo : replaceInfoList) {
                decodeFieldFunction(replaceInfo.group[replaceInfo.namedGroup["name"]].text, replaceInfo.group[replaceInfo.namedGroup["body"]].text, structMap);
            }
        } else {
            std::cerr << replaceResult.error() << std::endl;
        }
        running = replaceInfoList.size() > 0;
        replaceResult = extractArrayStructRegex.replace(content, "${1}std::vector<${3}> ${3};", replaceInfoList);
        if (replaceResult) {
            content = replaceResult.value();
            for (auto& replaceInfo : replaceInfoList) {
                decodeFieldFunction(replaceInfo.group[replaceInfo.namedGroup["name"]].text, replaceInfo.group[replaceInfo.namedGroup["body"]].text, structMap);
            }
        } else {
            std::cerr << replaceResult.error() << std::endl;
        }
        running = running || replaceInfoList.size() > 0;
        replaceResult = extractOptionalStructRegex.replace(content, "${1}std::optional<${3}> ${3};", replaceInfoList);
        if (replaceResult) {
            content = replaceResult.value();
            for (auto& replaceInfo : replaceInfoList) {
                decodeFieldFunction(replaceInfo.group[replaceInfo.namedGroup["name"]].text, replaceInfo.group[replaceInfo.namedGroup["body"]].text, structMap);
            }
        } else {
            std::cerr << replaceResult.error() << std::endl;
        }
        running = running || replaceInfoList.size() > 0;
        findResult = extractNamedStructRegex.find(content);
        if (findResult) {
            running = running || findResult.value().size() > 0;
            for (auto& findInfo : findResult.value() | std::views::reverse) {
                auto& lowerString = findInfo.group[findInfo.namedGroup["name"]].text;
                for (char& c : lowerString) {  // 遍历字符串中的每个字符
                    c = (char)std::tolower(c); // 将字符转换为小写
                }
                content.replace(findInfo.group[0].start, findInfo.group[0].text.length(), std::format("{}{} {};", findInfo.group[1].text, lowerString, lowerString));
                decodeFieldFunction(lowerString, findInfo.group[findInfo.namedGroup["body"]].text, structMap);
            }
        } else {
            std::cerr << findResult.error() << std::endl;
        }
    } while (running);
    findResult = regex::Pcre2Implementation("\\s*type\\s+(?<name>\\S+)\\s+struct\\s*\\{(?<body>[^\\{\\}]+)\\}").find(content);
    if (findResult) {
        for (auto& findInfo : findResult.value()) {
            decodeFieldFunction(findInfo.group[findInfo.namedGroup["name"]].text, findInfo.group[findInfo.namedGroup["body"]].text, eventMap);
        }
    } else {
        std::cerr << findResult.error() << std::endl;
    }
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
    auto wirteNamespace = [&](std::ofstream& outFile, const std::string& namespaceString, std::function<void()>&& f) {
            outFile << "namespace " << namespaceString << "\n{\n";
            f();
            outFile << "}\n";
        };
    auto unwarpTypeRegex = regex::Pcre2Implementation("^(\\S*<)?(\\S*?)(>*)?$");
    auto optionalTypeRegex = regex::Pcre2Implementation("std::optional");
    auto vectorTypeRegex = regex::Pcre2Implementation("std::vector");
    std::string eventNamespaceString = "SkyDreamBeta::Network::Github::Event";
    std::string structureNamespaceString = "SkyDreamBeta::Network::Github::Structure";
    std::unordered_map<std::string, int> typeNameMap;
    // 创建目录
    std::filesystem::create_directories("github/event");
    std::filesystem::create_directories("github/structure");
    for (auto& [structName, fieldMap] : structMap) {
        // 创建或打开文件用于写入
        std::ofstream outFile(std::format("github/structure/{}.h", structName));
        // 检查文件是否成功打开
        if (outFile.is_open()) {
            outFile << "#pragma once\n#include \"Global.h\"\n#include \"utils/JsonUtils.h\"\n";
            typeNameMap.clear();
            bool existOptional = false;
            bool existVector = false;
            for (auto& [fieldName, typeName] : fieldMap) {
                auto unwarpTypeName = unwarpTypeRegex.replace(typeName, "$2").value_or(typeName);
                if (structMap.contains(unwarpTypeName)) {
                    typeNameMap.try_emplace(std::format("{}", unwarpTypeName), 0);
                } else if (eventMap.contains(unwarpTypeName)) {
                    typeNameMap.try_emplace(std::format("../event/{}", unwarpTypeName), 0);
                }
                if (!existOptional) {
                    if (optionalTypeRegex.exist(typeName).value_or(false)) {
                        typeNameMap.emplace("optional", 1);
                        existOptional = true;
                    }
                }
                if (!existVector) {
                    if (vectorTypeRegex.exist(typeName).value_or(false)) {
                        typeNameMap.emplace("vector", 1);
                        existVector = true;
                    }
                }
            }
            for (auto& [typeName, type] : typeNameMap) {
                if (type == 0) {
                    outFile << std::format("#include \"{}.h\"\n", typeName);
                } else {
                    outFile << std::format("#include <{}>\n", typeName);
                }
            }
            outFile << "\n";
            wirteNamespace(outFile, structureNamespaceString, [&]() {
                outFile << "struct " << structName << "\n{\n";
                for (auto& [fieldName, typeName] : fieldMap) {
                    auto unwarpTypeName = unwarpTypeRegex.replace(typeName, "$2", replaceInfoList).value_or(typeName);
                    if (structMap.contains(unwarpTypeName)) {
                        outFile << std::format("    {} {};\n", typeName.replace(replaceInfoList[0].group[2].start, 0, "Structure::"), fieldName);
                    } else if (eventMap.contains(unwarpTypeName)) {
                        outFile << std::format("    {} {};\n", typeName.replace(replaceInfoList[0].group[2].start, 0, "Event::"), fieldName);
                    } else {
                        outFile << std::format("    {} {};\n", typeName, fieldName);
                    }
                }
                outFile << "};\n";
            });
            wirteNamespace(outFile, "nlohmann", [&]() {
                outFile << "void to_json(json& j, const " << structureNamespaceString << "::" << structName << "& d);\n";
                outFile << "void from_json(const json& j, " << structureNamespaceString << "::" << structName << "& d);\n";
            });
            // 关闭文件
            outFile.close();
        }
        // 创建或打开文件用于写入
        outFile = std::ofstream(std::format("github/structure/{}.cpp", structName));
        // 检查文件是否成功打开
        if (outFile.is_open()) {
            outFile << "#include \"" << structName << ".h\"\n";
            wirteNamespace(outFile, "nlohmann", [&]() {
                outFile << "void to_json(json& j, const " << structureNamespaceString << "::" << structName << "& d)\n{\n    j = json {\n";
                for (auto& [fieldName, typeName] : fieldMap) {
                    outFile << "        {" << std::format(" \"{}\", d.{} ", fieldName, fieldName) << "},\n";
                }
                outFile << "    };\n}\n";
                outFile << "void from_json(const json& j, " << structureNamespaceString << "::" << structName << "& d)\n{\n    json t = j;\n";
                for (auto& [fieldName, typeName] : fieldMap) {
                    outFile << std::format("    SkyDreamBeta::JsonUtils::getAndRemove(t, \"{}\", d.{});\n", fieldName, fieldName);
                }
                outFile << "    _ASSERT_EXPR(t.size() == 0, \"Key size must be 0\");\n}\n";
            });
            // 关闭文件
            outFile.close();
        }
    }
    for (auto& [structName, fieldMap] : eventMap) {
        // 创建或打开文件用于写入
        std::ofstream outFile(std::format("github/event/{}.h", structName));
        // 检查文件是否成功打开
        if (outFile.is_open()) {
            outFile << "#pragma once\n#include \"Global.h\"\n#include \"utils/JsonUtils.h\"\n";
            typeNameMap.clear();
            bool existOptional = false;
            bool existVector = false;
            for (auto& [fieldName, typeName] : fieldMap) {
                auto unwarpTypeName = unwarpTypeRegex.replace(typeName, "$2").value_or(typeName);
                if (structMap.contains(unwarpTypeName)) {
                    typeNameMap.try_emplace(std::format("../structure/{}", unwarpTypeName), 0);
                } else if (eventMap.contains(unwarpTypeName)) {
                    typeNameMap.try_emplace(std::format("{}", unwarpTypeName), 0);
                }
                if (!existOptional) {
                    if (optionalTypeRegex.exist(typeName).value_or(false)) {
                        typeNameMap.emplace("optional", 1);
                        existOptional = true;
                    }
                }
                if (!existVector) {
                    if (vectorTypeRegex.exist(typeName).value_or(false)) {
                        typeNameMap.emplace("vector", 1);
                        existVector = true;
                    }
                }
            }
            for (auto& [typeName, type] : typeNameMap) {
                if (type == 0) {
                    outFile << std::format("#include \"{}.h\"\n", typeName);
                } else {
                    outFile << std::format("#include <{}>\n", typeName);
                }
            }
            outFile << "\n";
            wirteNamespace(outFile, eventNamespaceString, [&]() {
                outFile << "struct " << structName << "\n{\n";
                for (auto& [fieldName, typeName] : fieldMap) {
                    auto unwarpTypeName = unwarpTypeRegex.replace(typeName, "$2", replaceInfoList).value_or(typeName);
                    if (structMap.contains(unwarpTypeName)) {
                        outFile << std::format("    {} {};\n", typeName.replace(replaceInfoList[0].group[2].start, 0, "Structure::"), fieldName);
                    } else if (eventMap.contains(unwarpTypeName)) {
                        outFile << std::format("    {} {};\n", typeName.replace(replaceInfoList[0].group[2].start, 0, "Event::"), fieldName);
                    } else {
                        outFile << std::format("    {} {};\n", typeName, fieldName);
                    }
                }
                outFile << "};\n";
            });
            wirteNamespace(outFile, "nlohmann", [&]() {
                outFile << "void to_json(json& j, const " << eventNamespaceString << "::" << structName << "& d);\n";
                outFile << "void from_json(const json& j, " << eventNamespaceString << "::" << structName << "& d);\n";
            });
            // 关闭文件
            outFile.close();
        }
        // 创建或打开文件用于写入
        outFile = std::ofstream(std::format("github/event/{}.cpp", structName));
        // 检查文件是否成功打开
        if (outFile.is_open()) {
            outFile << "#include \"" << structName << ".h\"\n";
            wirteNamespace(outFile, "nlohmann", [&]() {
                outFile << "void to_json(json& j, const " << eventNamespaceString << "::" << structName << "& d)\n{\n    j = json {\n";
                for (auto& [fieldName, typeName] : fieldMap) {
                    outFile << "        {" << std::format(" \"{}\", d.{} ", fieldName, fieldName) << "},\n";
                }
                outFile << "    };\n}\n";
                outFile << "void from_json(const json& j, " << eventNamespaceString << "::" << structName << "& d)\n{\n    json t = j;\n";
                for (auto& [fieldName, typeName] : fieldMap) {
                    outFile << std::format("    SkyDreamBeta::JsonUtils::getAndRemove(t, \"{}\", d.{});\n", fieldName, fieldName);
                }
                outFile << "    _ASSERT_EXPR(t.size() == 0, \"Key size must be 0\");\n}\n";
            });
            // 关闭文件
            outFile.close();
        }
    }
    return content;
}
