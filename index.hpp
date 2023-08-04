// 本代码是 建立索引相关的接口
// 索引 是用来快速搜索的
// parser模块, 已经将所有文档内容处理好, 并存储到了 data/output/raw 中
// 索引的建立, 就是通过获取 已经处理好的文档内容 来建立的
// 项目中, 需要分别建立正排索引和倒排索引
// 正排索引, 是从文档id 找到文件内容的索引
// 倒排索引, 是从关键词 找到关键词所在文档id 的索引

// 首先第一个问题:
// 正排索引中 文件内容该如何表示?
// 其实在parser模块中, 已经有过相关的处理了, 即用结构体(docInfo) 成员为: title、content、url
// 不过, 在建立索引时, 文档在索引中 应该存在一个文档id.

// 正排索引结构
// 正排索引 可以通过文档id找到文件内容. 那么 正排索引可以用 vector 建立, vector 存储docInfo结构体 那么数组下标就天然是 文档id

// 倒排索引结构
// 倒排索引 需要通过关键字 找到包含关键字的文档id, 文档id 对应正排索引中的下标, 所以需要先建立正排索引, 再建立倒排索引
// 由于可能多个文档包含相同的关键字, 倒排索引更适合 keyword:value 结构存储. 所以 可以使用 unordered_map
// 并且, 同样因为关键字可能找到多个文档, value的类型就 可以是存储着文档id的vector, 称为倒排拉链

// 倒排索引中, 通过关键字找到的 倒排拉链中 不应该仅仅是文档id的数据.
// 因为倒排索引的查找结果是关乎到查找结果的显示顺序的. 所以 还需要知道对应文档id 在本次搜索的权重.
// 所以, 最好将文档id和权重结合起来, 构成一个结构体(invertedElem)存储.
// 不过, 不需要 先将所有文档的正排索引建立完成之后 再建立倒排索引. 可以先给 某文档建立正排索引之后, 直接对此文档建立倒排索引

#pragma once

#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include "util.hpp"

namespace ns_index {

	// 用于正排索引中 存储文档内容
	typedef struct docInfo {
		std::string _title;	  // 文档标题
		std::string _content; // 文档去标签之后的内容
		std::string _url;	  // 文档对应官网url
		std::size_t _docId;	  // 文档id
	} docInfo_t;

	// 用于倒排索引中 记录关键字对应的文档id和权重
	typedef struct invertedElem {
		std::size_t _docId;	   // 文档id
		std::string _keyword;  // 关键字
		std::uint64_t _weight; // 搜索此关键字, 此文档id 所占权重

		invertedElem() // 权重初始化为0
			: _weight(0) {}
	} invertedElem_t;

	// 关键字的词频
	typedef struct keywordCnt {
		std::size_t _titleCnt;	 // 关键字在标题中出现的次数
		std::size_t _contentCnt; // 关键字在内容中出现的次数

		keywordCnt()
			: _titleCnt(0)
			, _contentCnt(0) {}
	} keywordCnt_t;

	// 倒排拉链
	typedef std::vector<invertedElem_t> invertedList_t;

	class index {
	private:
		// 正排索引使用vector, 下标天然是 文档id
		std::vector<docInfo_t> forwardIndex;
		// 倒排索引 使用 哈希表, 因为倒排索引 一定是 一个keyword 对应一组 invertedElem拉链
		std::unordered_map<std::string, invertedList_t> invertedIndex;

		// 单例模式设计
		index() {}

		index(const index&) = delete;
		index& operator=(const index&) = delete;

		static index* _instance; // 单例
		static std::mutex _mtx;

	public:
		// 获取单例
		static index* getInstance() {
			if (nullptr == _instance) {
				_mtx.lock();
				if (nullptr == _instance) {
					_instance = new index;
				}
				_mtx.unlock();
			}

			return _instance;
		}

		// 通过关键字 检索倒排索引, 获取对应的 倒排拉链
		invertedList_t* getInvertedList(const std::string& keyword) {
			// 先找 关键字 所在迭代器
			auto iter = invertedIndex.find(keyword);
			if (iter == invertedIndex.end()) {
				std::cerr << keyword << " have no invertedList!" << std::endl;
				return nullptr;
			}

			// 找到之后
			return &(iter->second);
		}

		// 通过倒排拉链中 每个倒排元素中存储的 文档id, 检索正排索引, 获取对应文档内容
		docInfo_t* getForwardIndex(std::size_t docId) {
			if (docId >= forwardIndex.size()) {
				std::cerr << "docId out range, error!" << std::endl;
				return nullptr;
			}

			return &forwardIndex[docId];
		}

		// 根据parser模块处理过的 所有文档的信息
		// 提取文档信息, 建立 正排索引和倒排索引
		// input 为 ./data/output/raw
		bool buildIndex(const std::string& input) {
			// 先以读取方式打开文件
			std::ifstream in(input, std::ios::in);
			if (!in.is_open()) {
				std::cerr << "Failed to open " << input << std::endl;
				return false;
			}

			std::size_t count = 0;

			std::string line;
			while (std::getline(in, line)) {
				// 按照parser模块的处理, getline 一次读取到的数据, 就是一个文档的: title\3content\3url\n
				docInfo_t* doc = buildForwardIndex(line); // 将一个文档的数据 建立到索引中
				if (nullptr == doc) {
					std::cerr << "Failed to buildForwardIndex for " << line << std::endl;
					continue;
				}

				// 文档建立正排索引成功, 接着就通过 doc 建立倒排索引
				if (!buildInvertedIndex(*doc)) {
					std::cerr << "Failed to buildInvertedIndex for " << line << std::endl;
					continue;
				}

				count++;
				if (count % 50 == 0)
					std::cout << "当前已经建立的索引文档: " << count << std::endl;
			}

			return true;
		}

	private:
		// 对一个文档建立正排索引
		docInfo_t* buildForwardIndex(const std::string& file) {
			// 一个文档的 正排索引的建立, 是将 title\3content\3url (file) 中title content url 提取出来
			// 构成一个 docInfo_t doc
			// 然后将 doc 存储到正排索引vector中
			std::vector<std::string> fileResult;
			const std::string sep("\3");
			// stringUtil::split() 字符串通用工具接口, 分割字符串
			ns_util::stringUtil::split(file, &fileResult, sep);

			docInfo_t doc;
			doc._title = fileResult[0];
			doc._content = fileResult[1];
			doc._url = fileResult[2];

			// 因为doc是需要存储到 forwardIndex中的, 存储之前 forwardIndex的size 就是存储之后 doc所在的位置
			doc._docId = forwardIndex.size();

			forwardIndex.push_back(std::move(doc));

			return &forwardIndex.back();
		}

		// 对一个文档建立倒排索引
		// 倒排索引是用来通过关键词定位文档的.
		// 倒排索引的结构是 std::unordered_map<std::string, invertedList_t> invertedIndex;
		// keyword值就是关键字, value值则是关键字所映射到的文档的倒排拉链
		// 对一个文档建立倒排索引的原理是:
		//  1. 首先对文档的标题 和 内容进行分词, 并记录分词
		//  2. 分别统计整理标题分析的词频 和 内容分词的词频
		//     统计词频是为了可以大概表示关键字在文档中的 相关性.
		//     在本项目中, 可以简单的认为关键词在文档中出现的频率, 代表了此文档内容与关键词的相关性. 当然这是非常肤浅的联系, 一般来说相关性的判断都是非常复杂的. 因为涉及到词义 语义等相关分析.
		//     每个关键字 在标题中出现的频率 和 在内容中出现的频率, 可以记录在一个结构体中. 此结构体就表示关键字的词频
		//  3. 使用 unordered_map<std::string, wordCnt_t> 记录关键字与其词频
		//  4. 通过遍历记录关键字与词频的 unordered_map, 构建 invertedElem: _docId, _keyword, _weight
		//  5. 构建了关键字的invertedElem 之后, 再将关键词的invertedElem 添加到在 invertedIndex中 关键词的倒排拉链 invertedList中
		// 注意, 搜索引擎一般不区分大小写, 所以可以将分词出来的所有的关键字, 在倒排索引中均以小写的形式映射. 在搜索时 同样将搜索请求分词出的关键字小写化, 在进行检索. 就可以实现搜索不区分大小写.

		// 关于分词 使用 cppjieba 中文分词库
		bool buildInvertedIndex(const docInfo_t& doc) {
			// 用来映射关键字 和 关键字的词频
			std::unordered_map<std::string, keywordCnt_t> keywordsMap;
			ns_util::jiebaUtil* jiebaIns = ns_util::jiebaUtil::getInstance();

			// 标题分词
			std::vector<std::string> titleKeywords;
			jiebaIns->cutString(doc._title, &titleKeywords);
			// 标题词频统计 与 转换 记录
			for (auto keyword : titleKeywords) {
				boost::to_lower(keyword);		  // 关键字转小写
				keywordsMap[keyword]._titleCnt++; // 记录关键字 并统计标题中词频
												  // unordered_map 的 [], 是用来通过keyword值 访问value的. 如果keyword值已经存在, 则返回对应的value, 如果keyword值不存在, 则会插入keyword并创建对应的value
			}

			// 内容分词
			std::vector<std::string> contentKeywords;
			jiebaIns->cutString(doc._content, &contentKeywords);
			// 内容词频统计 与 转换 记录
			for (auto keyword : contentKeywords) {
				boost::to_lower(keyword);			// 关键字转小写
				keywordsMap[keyword]._contentCnt++; // 记录关键字 并统计内容中词频
			}

			// 这两个const 变量是用来计算 关键字在文档中的权重的.
			// 并且, 关键字出现在标题中  文档与关键字的相关性大概率是要高的, 所以 可以把titleWeight 设置的大一些
			const int titleWeight = 20;
			const int contentWeight = 1;
			// 分词并统计词频之后, keywordsMap 中已经存储的当前文档的所有关键字, 以及对应的在标题 和 内容中 出现的频率
			// 就可以遍历 keywordsMap 获取关键字信息, 构建 invertedElem 并添加到 invertedIndex中 关键词的倒排拉链 invertedList中了
			for (auto& keywordInfo : keywordsMap) {
				invertedElem_t item;
				item._docId = doc._docId;		   // 本文档id
				item._keyword = keywordInfo.first; // 关键字
				item._weight = keywordInfo.second._titleCnt * titleWeight + keywordInfo.second._contentCnt * contentWeight;

				// 上面构建好了 invertedElem, 下面就要将 invertedElem 添加到对应关键字的 倒排拉链中, 构建倒排索引
				invertedList_t& list = invertedIndex[keywordInfo.first]; // 获取关键字对应的倒排拉链
				list.push_back(std::move(item));
			}

			return true;
		}
	};
	// 单例相关
	index* index::_instance = nullptr;
	std::mutex index::_mtx;
} // namespace ns_index
