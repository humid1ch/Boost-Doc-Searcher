// util.hpp 一般定义一些通用的宏定义、工具函数等

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>

namespace ns_util {
	class fileUtil {
	public:
		// readFile 用于读取指定文本文件的内容, 到string输出型参数中
		static bool readFile(const std::string& filePath, std::string* out) {
			// 要读取文件内容, 就要先打开文件
			// 1. 以读取模式打开文件
			std::ifstream in(filePath, std::ios::in);
			if (!in.is_open()) {
				// 打卡文件失败
				std::cerr << "Failed to open " << filePath << "!" << std::endl;
				return false;
			}

			// 走到这里打开文件成功
			// 2. 读取文件内, 并存储到out中
			std::string line;
			while (std::getline(in, line)) {
				*out += line;
			}

			in.close();

			return true;
		}
	};
} // namespace ns_util
