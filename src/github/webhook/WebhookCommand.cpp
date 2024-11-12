#include "command/Command.h"
#include "Webhook.h"
#include "WebhookCommand.h"
#include <iostream>
#include <memory>

void github::webhook::WebhookCommand::init() {
    auto operateWebhookCommand = command::Command::instance()->addCommand("从go代码生成webhook代码", []() {
        std::string path;
        std::cout << "请输入go代码文件路径:";
        std::cin >> path;
        if (std::cin) {
            Webhook::transform(removeQuotes(path));
            return true;
        } else {
            std::cin.clear();
            return false;
        }
    });
}

std::string& github::webhook::WebhookCommand::removeQuotes(std::string& str) {
    if (!str.empty()) {
        // 检查并去除开头的双引号
        if (str.front() == '"') {
            str.erase(0, 1);
        }

        // 检查并去除结尾的双引号
        if (!str.empty() && str.back() == '"') {
            str.pop_back();
        }
    }
    return str;
}
