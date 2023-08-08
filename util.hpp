// util.hpp 一般定义一些通用的宏定义、工具函数等

#pragma once

#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <mutex>
#include <boost/algorithm/string.hpp>
#include "logMessage.hpp"
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
				LOG(WARNING, "Failed to open %s !", filePath.c_str());
				// std::cerr << "Failed to open " << filePath << "!" << std::endl;
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
		static bool readFaviconFile(const std::string& filePath, std::string* out) {
			std::ifstream in(filePath, std::ios::in | std::ios::binary);
			if (!in.is_open()) {
				// 打卡文件失败
				LOG(WARNING, "Failed to open %s !", filePath.c_str());
				// std::cerr << "Failed to open " << filePath << "!" << std::endl;
				return false;
			}

			// 走到这里打开文件成功
			// 2. 读取文件内, 并存储到out中
			char buffer[1024];
			while (in.read(buffer, sizeof buffer - 1)) {
				*out += buffer;
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
		cppjieba::Jieba _jieba;
		std::unordered_map<std::string, bool> _stopKeywordMap;

		jiebaUtil()
			: _jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH) {}

		jiebaUtil(const jiebaUtil&) = delete;
		jiebaUtil& operator=(const jiebaUtil&) = delete;

		static jiebaUtil* _instance;

	private:
		void noStopHelper(const std::string& src, std::vector<std::string>* out) {
			_jieba.CutForSearch(src, *out);
			// 遍历out 查询是否为停止词 是则删除
			// 需要注意迭代器失效的问题
			for (auto iter = out->begin(); iter != out->end();) {
				std::string word = *iter;
				boost::to_lower(word);
				auto stopIt = _stopKeywordMap.find(word);
				// auto stopIt = _stopKeywordMap.find(*iter);
				if (stopIt != _stopKeywordMap.end()) {
					// 注意接收erase的返回值 防止出现迭代器失效问题
					iter = out->erase(iter);
				}
				else {
					iter++;
				}
			}
		}

		// 主要是为了支持 消除停止词的分词
		// 也就是需要将停止词, 写入到 map中
		bool initJiebaUtil() {
			// 首先按行读取文件 const char* const STOP_WORD_PATH = "./cppjiebaDict/stop_words.utf8"
			std::ifstream stopFile(STOP_WORD_PATH, std::ios::in);
			if (!stopFile.is_open()) {
				return false;
			}

			std::string line;
			while (std::getline(stopFile, line)) {
				_stopKeywordMap.insert({line, true});
			}

			stopFile.close();

			return true;
		}

	public:
		static jiebaUtil* getInstance() {
			static std::mutex mtx;
			if (nullptr == _instance) {
				mtx.lock();
				if (nullptr == _instance) {
					_instance = new jiebaUtil;
					_instance->initJiebaUtil();
				}
				mtx.unlock();
			}

			return _instance;
		}

		// 分词: 不消除停止词的版本
		void cutString(const std::string& src, std::vector<std::string>* out) {
			_jieba.CutForSearch(src, *out);
		}
		// 分词: 消除停止词的版本
		void cutStringNoStop(const std::string& src, std::vector<std::string>* out) {
			noStopHelper(src, out);
		}
	};
	jiebaUtil* jiebaUtil::_instance;
	// cppjieba::Jieba jiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);
} // namespace ns_util
