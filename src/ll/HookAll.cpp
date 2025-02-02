#include "HookAll.h"
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

std::vector<std::string> get_relative_file_paths(const fs::path& root_dir) {
    // 验证输入路径有效性
    if (!fs::exists(root_dir)) {
        throw std::invalid_argument("Directory does not exist: " + root_dir.string());
    }
    if (!fs::is_directory(root_dir)) {
        throw std::invalid_argument("Path is not a directory: " + root_dir.string());
    }

    // 获取规范化绝对路径（处理符号链接和路径规范化）
    const fs::path canonical_root = fs::canonical(root_dir);
    std::vector<std::string> file_paths;

    try {
        // 配置递归迭代器选项（跳过无权限目录）
        auto options = fs::directory_options::skip_permission_denied
            | fs::directory_options::follow_directory_symlink;

        std::error_code error;
        // 递归遍历目录
        for (const auto& entry : fs::recursive_directory_iterator(
            canonical_root,
            options,
            error
        )) {
            // 跳过非常规文件（目录、符号链接等）
            if (!entry.is_regular_file()) {
                continue;
            }

            // 获取相对路径
            const fs::path absolute_path = entry.path();
            const fs::path relative_path = absolute_path.lexically_relative(canonical_root);

            // 转换为通用格式字符串（统一使用 / 分隔符）
            file_paths.emplace_back(relative_path.generic_string());
        }
    } catch (const fs::filesystem_error& e) {
        throw std::runtime_error("Filesystem error: " + std::string(e.what()));
    }

    return file_paths;
}

std::string ll::HookAll::generate(std::string srcPath) {
    try {
        auto files = get_relative_file_paths(srcPath + "/src/mc");
        for (const auto& path : files) {
            std::cout << path << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return std::string();
}
