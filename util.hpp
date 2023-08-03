// util.hpp 一般定义一些通用的宏定义、工具函数等

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include "cppjieba/Jieba.hpp"

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

	class stringUtil {
	public:
		static bool split(const std::string& file, std::vector<std::string>* fileResult, const std::string& sep) {
			// 使用 boost库中的split接口, 可以将 string 以指定的分割符分割, 并存储到vector<string>输出型参数中
			boost::split(*fileResult, file, boost::is_any_of(sep), boost::algorithm::token_compress_on);
			// boost::algorithm::token_compress_on 表示压缩连续的分割符

			if (fileResult->empty()) {
				return false;
			}

			return true;
		}
	};

	const char* const DICT_PATH = "./cppjiebaDict/jieba.dict.utf8";
	const char* const HMM_PATH = "./cppjiebaDict/hmm_model.utf8";
	const char* const USER_DICT_PATH = "./cppjiebaDict/user.dict.utf8";
	const char* const IDF_PATH = "./cppjiebaDict/idf.utf8";
	const char* const STOP_WORD_PATH = "./cppjiebaDict/stop_words.utf8";

	class jiebaUtil {
	private:
		static cppjieba::Jieba jieba;

	public:
		static void cutString(const std::string& src, std::vector<std::string>* out) {
			// 以用于搜索的方式 分词
			jieba.CutForSearch(src, *out);
		}
	};
	cppjieba::Jieba jiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);
} // namespace ns_util
