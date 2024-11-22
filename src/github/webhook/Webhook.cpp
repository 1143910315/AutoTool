#include "regex/Pcre2Implementation.h"
#include "utils/Defer.hpp"
#include "Webhook.h"
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <ranges>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

class FieldInfo {
public:
    FieldInfo(const std::string& fieldName, const std::string& typeName, const std::string& keyName) : fieldName(fieldName), typeName(typeName), keyName(keyName) {
    }

    std::string fieldName;
    std::string typeName;
    std::string keyName;
};
std::string github::webhook::Webhook::transform(std::string fileName) {
    // https://docs.github.com/zh/webhooks/webhook-events-and-payloads#push
    // go语言 https://github.com/go-playground/webhooks
    // https://github.com/go-playground/webhooks/blob/master/github%2Fpayload.go
    // C# https://github.com/octokit/webhooks.net
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
    auto replaceResult = regex::Pcre2Implementation("(\\n)\\r").replace(content, "$1");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    size_t id = 0;
    bool running = false;
    std::unordered_map<std::string, std::tuple<std::string, std::vector<FieldInfo>>> structInfoMap;
    std::unordered_map<std::string, std::vector<FieldInfo>> headStructInfoMap;
    std::unordered_set<std::string> needIncludeFile;
    std::vector<regex::Pcre2Implementation::MatchInfo> matchInfoList;
    auto extractStructRegex = regex::Pcre2Implementation("^(?<space>\\s*)(?<structName>\\S+)\\s+struct\\s*\\{(?<body>[^\\{\\}]+)\\}\\s*`json:\"(?<name>\\S+?)(,\\S*)?\"`");
    auto extractArrayStructRegex = regex::Pcre2Implementation("^(?<space>\\s*)(?<structName>\\S+)\\s+\\[\\]struct\\s*\\{(?<body>[^\\{\\}]+)\\}\\s*`json:\"(?<name>\\S+?)(,\\S*)?\"`");
    auto extractOptionalStructRegex = regex::Pcre2Implementation("^(?<space>\\s*)(?<structName>\\S+)\\s+\\*struct\\s*\\{(?<body>[^\\{\\}]+)\\}\\s*`json:\"(?<name>\\S+?)(,\\S*)?\"`");
    auto extractNamedStructRegex = regex::Pcre2Implementation("^(?<space>\\s*)(?<structName>\\S+?)\\s+struct\\s*\\{(?<body>[^\\{\\}]+)\\}\\s*$");
    auto structFieldRegex = regex::Pcre2Implementation("^(?<space>\\s*)(?<fieldName>\\S+)\\s+(?<typeName>\\S+)\\s+`json:\"(?<keyName>\\S+?)(,\\S*)?\"`");
    auto goStringTypeRegex = regex::Pcre2Implementation("string");
    auto goTimeTypeRegex = regex::Pcre2Implementation("time\\.Time");
    auto goFloat64TypeRegex = regex::Pcre2Implementation("float64");
    auto goUint32TypeRegex = regex::Pcre2Implementation("uint32");
    auto goOptionalTypeRegex = regex::Pcre2Implementation("(\\S*)\\*(\\S+)");
    auto goArrayTypeRegex = regex::Pcre2Implementation("\\[\\](\\S+)");
    auto extractTypeRegex = regex::Pcre2Implementation("((?<begin>.*<)(?<type1>[^\\s<>]+)(?<end>>.*))|(?<type2>[^<>]*)");
    auto decodeFieldFunction = [&](const std::string& body) {
            std::vector<FieldInfo> fieldInfoList;
            auto findResult = structFieldRegex.find(body);
            if (findResult) {
                for (auto& matchInfo : *findResult) {
                    auto& typeString = matchInfo["typeName"];
                    typeString = goStringTypeRegex.replace(typeString, "std::string").value_or(typeString);
                    typeString = goTimeTypeRegex.replace(typeString, "std::string").value_or(typeString);
                    typeString = goFloat64TypeRegex.replace(typeString, "double").value_or(typeString);
                    typeString = goUint32TypeRegex.replace(typeString, "uint").value_or(typeString);
                    typeString = goOptionalTypeRegex.replace(typeString, "std::optional<${1}${2}>").value_or(typeString);
                    typeString = goArrayTypeRegex.replace(typeString, "std::vector<${1}>").value_or(typeString);
                    fieldInfoList.emplace_back(FieldInfo(matchInfo["fieldName"], matchInfo["typeName"], matchInfo["keyName"]));
                }
            } else {
                std::cerr << findResult.error() << std::endl;
            }
            return fieldInfoList;
        };
    auto beginNamespace = [](std::ofstream& outFile, const std::string& name, std::function<void()> func) {
            outFile << std::format("namespace {}\n{{\n", name);
            func();
            outFile << std::format("}} // namespace {}\n", name);
        };
    auto beginStruct = [](std::ofstream& outFile, const std::string& name, int deep, std::function<void()> func) {
            std::string spaceString(deep * 4, ' ');
            outFile << std::format("{}struct {}\n{}{{\n", spaceString, name, spaceString);
            func();
            outFile << std::format("{}}};\n", spaceString);
        };
    auto generateField = [&](std::ofstream& outFile, const std::vector<FieldInfo>& fieldInfoList, int deep) {
            std::string spaceString(deep * 4, ' ');
            outFile << std::format("{}public:\n", spaceString);
            for (auto& fieldInfo : fieldInfoList) {
                std::string prefix = "";
                if (fieldInfo.keyName == "default" || fieldInfo.keyName == "private" || fieldInfo.keyName == "public") {
                    prefix = "_";
                }
                if (fieldInfo.typeName == "struct") {
                    outFile << std::format("{}    {} {}{};\n", spaceString, std::get<0>(structInfoMap[fieldInfo.fieldName]), prefix, fieldInfo.keyName);
                } else if (fieldInfo.typeName == "std::vector<struct>") {
                    outFile << std::format("{}    std::vector<{}> {}{};\n", spaceString, std::get<0>(structInfoMap[fieldInfo.fieldName]), prefix, fieldInfo.keyName);
                } else if (fieldInfo.typeName == "std::optional<struct>") {
                    outFile << std::format("{}    std::optional<{}> {}{};\n", spaceString, std::get<0>(structInfoMap[fieldInfo.fieldName]), prefix, fieldInfo.keyName);
                } else {
                    replaceResult = extractTypeRegex.replace(fieldInfo.typeName, "${type1}${type2}", 1);
                    if (replaceResult) {
                        auto& name = *replaceResult;
                        if (headStructInfoMap.contains(name)) {
                            replaceResult = extractTypeRegex.replace(fieldInfo.typeName, "${begin}${type1}${type2}Data::${type1}${type2}${end}", 1);
                            if (replaceResult) {
                                auto& nameWithNamespace = *replaceResult;
                                outFile << std::format("{}    {} {}{};\n", spaceString, nameWithNamespace, prefix, fieldInfo.keyName);
                            } else {
                                std::cerr << replaceResult.error() << std::endl;
                            }
                        } else {
                            outFile << std::format("{}    {} {}{};\n", spaceString, fieldInfo.typeName, prefix, fieldInfo.keyName);
                        }
                    } else {
                        std::cerr << replaceResult.error() << std::endl;
                    }
                }
            }
        };
    auto traverseField = [&](const std::string& namespaceName, const std::vector<FieldInfo>& fieldInfoList, std::function<void(const std::string& namespaceName, const std::string& name, const std::vector<FieldInfo>& list)> func) {
            for (auto& fieldInfo : fieldInfoList) {
                if (fieldInfo.typeName == "struct" || fieldInfo.typeName == "std::vector<struct>" || fieldInfo.typeName == "std::optional<struct>") {
                    auto& [name, list] = structInfoMap[fieldInfo.fieldName];
                    func(namespaceName, name, list);
                }
            }
        };
    std::function<void(std::ofstream&, const std::string&, const std::vector<FieldInfo>&, int )> generateStruce = [&](std::ofstream& outFile, const std::string& struceName, const std::vector<FieldInfo>& fieldInfoList, int deep) {
            beginStruct(outFile, struceName, deep, [&]() {
            for (auto& fieldInfo : fieldInfoList) {
                if (fieldInfo.typeName == "struct" || fieldInfo.typeName == "std::vector<struct>" || fieldInfo.typeName == "std::optional<struct>") {
                    auto& [name, list] = structInfoMap[fieldInfo.fieldName];
                    generateStruce(outFile, name, list, deep + 1);
                }
            }
            generateField(outFile, fieldInfoList, deep);
        });
        };
    auto transformJsonDefinition = [](std::ofstream& outFile, const std::string& className, const std::string& variableName, std::function<void()> toJsonFunc, std::function<void()> fromJsonFunc) {
            outFile << "void to_json(json& j, const " << className << "& " << variableName << ")";
            toJsonFunc();
            outFile << "void from_json(const json& j, " << className << "& " << variableName << ")";
            fromJsonFunc();
        };
    replaceResult = regex::Pcre2Implementation("(interface\\{\\})|(struct\\{\\})").replace(content, "nlohmann::json");
    if (replaceResult) {
        content = replaceResult.value();
    } else {
        std::cerr << replaceResult.error() << std::endl;
    }
    auto typeFindResult = regex::Pcre2Implementation("^type\\s+(?<eventName>\\S+)\\s+struct\\s*\\{(?<eventBody>[\\s\\S]*?)^\\}$").find(content);
    if (typeFindResult) {
        for (auto& typeStructMatchInfo : *typeFindResult) {
            do {
                running = false;
                replaceResult = extractStructRegex.replace(typeStructMatchInfo["eventBody"], std::format("${{space}}{} struct `json:\"${{name}}\"`", id), matchInfoList, 1);
                if (replaceResult) {
                    if (matchInfoList.size() == 1) {
                        typeStructMatchInfo["eventBody"] = replaceResult.value();
                        structInfoMap.emplace(std::format("{}", id), std::make_tuple<>(matchInfoList[0]["structName"], decodeFieldFunction(matchInfoList[0]["body"])));
                        id++;
                    }
                } else {
                    std::cerr << replaceResult.error() << std::endl;
                }
                running = matchInfoList.size() > 0;
                replaceResult = extractArrayStructRegex.replace(typeStructMatchInfo["eventBody"], std::format("${{space}}{} []struct `json:\"${{name}}\"`", id), matchInfoList, 1);
                if (replaceResult) {
                    if (matchInfoList.size() == 1) {
                        typeStructMatchInfo["eventBody"] = replaceResult.value();
                        structInfoMap.emplace(std::format("{}", id), std::make_tuple<>(matchInfoList[0]["structName"], decodeFieldFunction(matchInfoList[0]["body"])));
                        id++;
                    }
                } else {
                    std::cerr << replaceResult.error() << std::endl;
                }
                running = running || matchInfoList.size() > 0;
                replaceResult = extractOptionalStructRegex.replace(typeStructMatchInfo["eventBody"], std::format("${{space}}{} *struct `json:\"${{name}}\"`", id), matchInfoList, 1);
                if (replaceResult) {
                    if (matchInfoList.size() == 1) {
                        typeStructMatchInfo["eventBody"] = replaceResult.value();
                        structInfoMap.emplace(std::format("{}", id), std::make_tuple<>(matchInfoList[0]["structName"], decodeFieldFunction(matchInfoList[0]["body"])));
                        id++;
                    }
                } else {
                    std::cerr << replaceResult.error() << std::endl;
                }
                running = running || matchInfoList.size() > 0;
                auto findResult = extractNamedStructRegex.find(typeStructMatchInfo["eventBody"], 1);
                if (findResult) {
                    if (findResult->size() > 0) {
                        running = true;
                        auto& matchInfo = (*findResult)[0];
                        std::string lowerString = matchInfo["structName"];
                        for (char& c : lowerString) {  // 遍历字符串中的每个字符
                            c = (char)std::tolower(c); // 将字符转换为小写
                        }
                        typeStructMatchInfo["eventBody"].replace(matchInfo.group[0].start, matchInfo[0].length(), std::format("{}{} struct `json:\"{}\"`", matchInfo["space"], id, lowerString));
                        structInfoMap.emplace(std::format("{}", id), std::make_tuple<>(matchInfo["structName"], decodeFieldFunction(matchInfo["body"])));
                        id++;
                    }
                } else {
                    std::cerr << findResult.error() << std::endl;
                }
            } while (running);
        }
        for (auto& typeStructMatchInfo : *typeFindResult) {
            headStructInfoMap.emplace(typeStructMatchInfo["eventName"], decodeFieldFunction(typeStructMatchInfo["eventBody"]));
        }
        std::function<void(std::vector<FieldInfo>& fieldInfoList, const std::vector<std::string>& keyLink, const FieldInfo& fieldInfo, int index)> appendJsonKeyTypeLoop = [&](std::vector<FieldInfo>& fieldInfoList, const std::vector<std::string>& keyLink, const FieldInfo& fieldInfo, int index) {
                if (index < keyLink.size()) {
                    for (auto& fieldInfoData : fieldInfoList) {
                        if (fieldInfoData.keyName == keyLink[index]) {
                            auto& [name, list] = structInfoMap[fieldInfoData.fieldName];
                            appendJsonKeyTypeLoop(list, keyLink, fieldInfo, index + 1);
                        }
                    }
                } else {
                    fieldInfoList.emplace_back(fieldInfo);
                }
            };
        auto appendJsonKeyType = [&](const std::string& eventName, const std::vector<std::string>& keyLink, const FieldInfo& fieldInfo) {
                auto& eventInfo = headStructInfoMap[eventName];
                appendJsonKeyTypeLoop(eventInfo, keyLink, fieldInfo, 0);
            };
        auto appendStructInfo = [&](const std::string& structName) {
                auto idString = std::format("{}", id);
                structInfoMap.emplace(idString, std::make_tuple<>(structName, std::vector<FieldInfo>()));
                id++;
                return idString;
            };
        std::function<void(std::vector<FieldInfo>& fieldInfoList, const std::vector<std::string>& keyLink, const std::string& newTypeName, size_t index)> changeJsonKeyTypeLoop = [&](std::vector<FieldInfo>& fieldInfoList, const std::vector<std::string>& keyLink, const std::string& newTypeName, size_t index) {
                if (index < keyLink.size()) {
                    for (auto& fieldInfoData : fieldInfoList) {
                        if (fieldInfoData.keyName == keyLink[index]) {
                            if (index + 1 == keyLink.size()) {
                                fieldInfoData.typeName = newTypeName;
                            } else {
                                auto& [name, list] = structInfoMap[fieldInfoData.fieldName];
                                changeJsonKeyTypeLoop(list, keyLink, newTypeName, index + 1);
                            }
                        }
                    }
                }
            };
        auto changeJsonKeyType = [&](const std::string& eventName, const std::vector<std::string>& keyLink, const std::string& newTypeName) {
                auto& eventInfo = headStructInfoMap[eventName];
                changeJsonKeyTypeLoop(eventInfo, keyLink, newTypeName, 0);
            };
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "bool", "allow_forking" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "bool", "archived" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "std::string", "deployments_url" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "bool", "disabled" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "bool", "has_discussions" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "bool", "has_projects" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "bool", "is_template" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { appendStructInfo("License"), "struct", "license" });
        appendJsonKeyType("ReleasePayload", { "repository", "license" }, { "", "std::string", "key" });
        appendJsonKeyType("ReleasePayload", { "repository", "license" }, { "", "std::string", "name" });
        appendJsonKeyType("ReleasePayload", { "repository", "license" }, { "", "std::string", "node_id" });
        appendJsonKeyType("ReleasePayload", { "repository", "license" }, { "", "std::string", "spdx_id" });
        appendJsonKeyType("ReleasePayload", { "repository", "license" }, { "", "std::string", "url" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "std::string", "node_id" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "std::vector<std::string>", "topics" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "std::string", "visibility" });
        appendJsonKeyType("ReleasePayload", { "repository" }, { "", "bool", "web_commit_signoff_required" });
        changeJsonKeyType("ReleasePayload", { "installation" }, "std::optional<struct>");
        // 创建目录
        std::filesystem::create_directories("github");
        for (auto& [eventName, decodeFieldList] : headStructInfoMap) {
            // 创建或打开文件用于写入
            std::ofstream outFile(std::format("github/{}.h", eventName));
            if (outFile.is_open()) {
                outFile << "#pragma once\n#include \"Global.h\"\n#include \"utils/JsonUtils.h\"\n#include <optional>\n#include <string>\n#include <vector>\n";
                needIncludeFile.clear();
                std::function<void(const std::vector<FieldInfo>& fieldInfoList)> traverseInclude = [&](const std::vector<FieldInfo>& fieldInfoList) {
                        for (auto& fieldInfo : fieldInfoList) {
                            replaceResult = extractTypeRegex.replace(fieldInfo.typeName, "${type1}${type2}", 1);
                            if (replaceResult) {
                                auto& name = *replaceResult;
                                if (headStructInfoMap.contains(name)) {
                                    needIncludeFile.emplace(name);
                                }
                            } else {
                                std::cerr << replaceResult.error() << std::endl;
                            }
                            if (fieldInfo.typeName == "struct" || fieldInfo.typeName == "std::vector<struct>" || fieldInfo.typeName == "std::optional<struct>") {
                                auto& [name, list] = structInfoMap[fieldInfo.fieldName];
                                traverseInclude(list);
                            }
                        }
                    };
                traverseInclude(decodeFieldList);
                for (auto& includeFile : needIncludeFile) {
                    outFile << std::format("#include \"{}.h\"\n", includeFile);
                }
                outFile << "\n";
                beginNamespace(outFile, std::format("SkyDreamBeta::inline Network::Github::{}Data", eventName), [&]() {
                    generateStruce(outFile, eventName, decodeFieldList, 1);
                });
                beginNamespace(outFile, "nlohmann", [&]() {
                    std::function<void(const std::string& namespaceName, const std::string& name, const std::vector<FieldInfo>& list)> fieldDeal = [&](const std::string& namespaceName, const std::string& name, const std::vector<FieldInfo>& list) {
                            auto className = std::format("{}::{}", namespaceName, name);
                            traverseField(className, list, fieldDeal);
                            std::string variableName = name;
                            if (variableName.length() > 0) {
                                variableName[0] = (char)std::tolower(variableName[0]);
                            }
                            transformJsonDefinition(outFile, className, variableName, [&]() {
                            outFile << ";\n";
                        }, [&]() {
                            outFile << ";\n";
                        });
                        };
                    fieldDeal(std::format("SkyDreamBeta::Network::Github::{}Data", eventName), eventName, decodeFieldList);
                });
                outFile.close();
            }
            outFile = std::ofstream(std::format("github/{}.cpp", eventName));
            if (outFile.is_open()) {
                outFile << std::format("#include \"{}.h\"\n\n", eventName);
                beginNamespace(outFile, "nlohmann", [&]() {
                    std::function<void(const std::string& namespaceName, const std::string& name, const std::vector<FieldInfo>& list)> fieldDeal = [&](const std::string& namespaceName, const std::string& name, const std::vector<FieldInfo>& list) {
                            auto className = std::format("{}::{}", namespaceName, name);
                            traverseField(className, list, fieldDeal);
                            std::string variableName = name;
                            if (variableName.length() > 0) {
                                variableName[0] = (char)std::tolower(variableName[0]);
                            }
                            transformJsonDefinition(outFile, className, variableName, [&]() {
                            outFile << "\n{\n    j = json {\n";
                            for (auto& field : list) {
                                std::string prefix = "";
                                if (field.keyName == "default" || field.keyName == "private" || field.keyName == "public") {
                                    prefix = "_";
                                }
                                outFile << std::format("        {{ \"{}\", {}.{}{} }},\n", field.keyName, variableName, prefix, field.keyName);
                            }
                            outFile << "    };\n}\n";
                        }, [&]() {
                            outFile << "\n{\n    json t = j;\n";
                            for (auto& field : list) {
                                std::string prefix = "";
                                if (field.keyName == "default" || field.keyName == "private" || field.keyName == "public") {
                                    prefix = "_";
                                }
                                outFile << std::format("    SkyDreamBeta::JsonUtils::getAndRemove(t, \"{}\", {}.{}{});\n", field.keyName, variableName, prefix, field.keyName);
                            }
                            outFile << "    _ASSERT_EXPR(t.size() == 0, \"Key size must be 0\");\n}\n";
                        });
                        };
                    fieldDeal(std::format("SkyDreamBeta::Network::Github::{}Data", eventName), eventName, decodeFieldList);
                });
                outFile.close();
            }
        }
    } else {
        std::cerr << typeFindResult.error() << std::endl;
    }
    return content;
}
