// 本文件实现 搜索相关接口

// 本项目中的搜索, 是根据输入的关键词:
//  1. 先对关键词进行分词
//  2. 然后通过分词, 在倒排索引中进行检索, 检索到相关的倒排拉链
//  3. 然后再通过倒排拉链中 倒排元素的对应文档id, 在正排索引中获取文件内容

// 不过在正式开始搜索之前, 要先构建索引
// 而索引的构建, 在整个程序中只需要构建一次, 所以可以将索引设计为单例模式
#pragma once

#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <jsoncpp/json/json.h>
#include "util.hpp"
#include "index.hpp"

namespace ns_searcher {
	typedef struct invertedElemOut {
		std::size_t _docId;
		std::uint64_t _weight;
		std::vector<std::string> _keywords;
	} invertedElemOut_t;

	class searcher {
	private:
		ns_index::index* _index; // 建立索引的类

	public:
		void initSearcher(const std::string& input) {
			// 搜索前的初始化操作
			// 获取单例
			_index = ns_index::index::getInstance();
			std::cout << "获取单例成功 ..." << std::endl;
			// 建立索引
			_index->buildIndex(input);
			std::cout << "构建正排索引、倒排索引成功 ..." << std::endl;
		}

		// 搜索接口
		// 搜索需要实现什么功能?
		// 首先参数部分需要怎么实现?
		// 参数部分, 需要接收需要搜索的句子或关键字, 还需要一个输出型参数 用于输出查找结果
		//  查找结果我们使用jsoncpp进行序列化和反序列化
		// search() 具体需要实现的功能:
		//  1. 对接收的句子或关键词进行分词
		//  2. 根据分词, 在倒排索引中查找到所有分词的倒排拉链 汇总 的 invertedElem, 并根据相关性进行排序
		//  4. 然后再遍历所有的 invertedElem, 根据 invertedElem中存储的 文档id, 在正排索引中获取到文档内容
		//  5. 然后将获取到的文档内容使用jsoncpp 进行序列化, 存储到输出型参数中
		// 直到遍历完invertedElem
		void search(const std::string& query, std::string* jsonString) {
			// 1. 对需要搜索的句子或关键词进行分词
			std::vector<std::string> keywords;
			ns_util::jiebaUtil::cutString(query, &keywords);

			std::vector<invertedElemOut_t> allInvertedElemOut;
			// std::vector<ns_index::invertedElem_t> allInvertedElem;

			// 统计文档用, 因为可能存在不同的分词 在倒排索引中指向同一个文档的情况
			// 如果不去重, 会重复展示
			// std::unordered_map<std::size_t, ns_index::invertedElem_t> invertedElemMap;
			std::unordered_map<std::size_t, invertedElemOut_t> invertedElemOutMap;
			// 2. 根据分词获取倒排索引中的倒排拉链, 并汇总去重 invertedElem
			for (std::string word : keywords) {
				boost::to_lower(word);

				ns_index::invertedList_t* tmpInvertedList = _index->getInvertedList(word);
				if (nullptr == tmpInvertedList) {
					// 没有这个关键词
					continue;
				}

				for (auto& elem : *tmpInvertedList) {
					// 遍历倒排拉链, 根据文档id 对invertedElem 去重
					auto& item = invertedElemOutMap[elem._docId]; // 在map中获取 或 创建对应文档id的 invertedElem
					item._docId = elem._docId;
					item._weight += elem._weight; // 权重需要+= 是因为一个文档通过不同的关键词被搜索到, 此文档就应该将与其相关的关键词的权重相加
												  // 最好还将 此文档相关的关键词 也存储起来, 因为在客户端搜索结果中, 需要对网页中有的关键字进行高亮
												  // 但是 invertedElem 的第三个成员是 单独的一个string对象, 不太合适
												  // 所以, 可以定义一个与invertedElem 相似的, 但是第三个成员是一个 vector 的类, 比如 invertedElemOut
					item._keywords.push_back(elem._keyword);
					// 此时就将当前invertedElem 去重到了 invertedElemMap 中
				}
			}
			// 出循环之后, 就将搜索到的 文档的 id、权重和相关关键词 存储到了 invertedElemMap
			// 然后将文档的相关信息 invertedElemOut 都存储到 vector 中
			for (const auto& elemOut : invertedElemOutMap) {
				// map中的second: elemOut, 在执行此操作之后, 就没用了
				// 所以使用移动语义, 防止发生拷贝
				allInvertedElemOut.push_back(std::move(elemOut.second));
			}

			// 执行到这里, 可以搜索到的文档id 权重 和 相关关键词的信息, 已经都在allInvertedElemOut 中了.
			// 但是, 还不能直接 根据文档id 在正排索引中检索
			// 因为, 此时如果直接进行文档内容的索引, 在找到文档内容之后, 就要直接进行序列化并输出了. 而客户端显示的时候, 反序列化出来的文档顺序, 就是显示的文档顺序
			// 但是现在找到的文档还是乱序的. 还需要将allInvertedElemOut中的相关文档, 通过_weight 进行倒序排列
			// 这样, 序列化就是按照倒序排列的, 反序列化也会如此, 显示同样如此
			std::sort(allInvertedElemOut.begin(), allInvertedElemOut.end(),
					  [](const invertedElemOut_t& elem1, const invertedElemOut_t& elem2) {
						  return elem1._weight > elem2._weight;
					  });

			// 排序之后, allInvertedElemOut 中文档的排序就是倒序了
			// 然后 通过遍历此数组, 获取文档id, 根据id获取文档在正排索引中的内容
			// 然后再将 所有内容序列化
			Json::Value root;
			for (auto& elemOut : allInvertedElemOut) {
				// 通过Json::Value 对象, 存储文档内容
				Json::Value elem;
				// 通过elemOut._docId 获取正排索引中 文档的内容信息
				ns_index::docInfo_t* doc = _index->getForwardIndex(elemOut._docId);
				// elem赋值
				elem["url"] = doc->_url;
				elem["title"] = doc->_title;
				// 关于文档的内容, 搜索结果中是不展示文档的全部内容的, 应该只显示包含关键词的摘要, 点进文档才显示相关内容
				// 而docInfo中存储的是文档去除标签之后的所有内容, 所以不能直接将 doc._content 存储到elem对应key:value中
				elem["desc"] = getDesc(doc->_content, elemOut._keywords[0]); // 只根据第一个关键词来获取摘要
				// for Debug
				// 这里有一个bug, jsoncpp 0.10.5.2 是不支持long或long long 相关类型的, 所以需要转换成 double
				// 这里转换成 double不会有什么影响, 因为这两个参数只是本地调试显示用的.
				elem["docId"] = (double)doc->_docId;
				elem["weight"] = (double)elemOut._weight;

				root.append(elem);
			}

			// 序列化完成之后将相关内容写入字符串
			// for Debug 用 styledWriter
			Json::StyledWriter writer;
			*jsonString = writer.write(root);
		}

		std::string getDesc(const std::string& content, const std::string& keyword) {
			// 如何获取摘要呢?
			// 我们尝试获取正文中 第一个keyword 的前50个字节和后100个字节的内容 作为摘要
			const std::size_t prevStep = 50;
			const std::size_t nextStep = 100;
			// 获取正文中 第一个 keyword 的位置

			// 直接这样处理, 会出现一个问题:
			// keyword是有大小写的.
			// 而 string::find() 是区分大小写的查找
			// 那要怎么查找呢? string容器也没有提供不区分大小写的查找方法
			// 那就要用到std::search()
			// std::search(it1, it2, it3, it4, pred);
			// 可以在[it1, it2)中 查找第一个[it3, it4)(词语)的出现位置. 并且, 如果使用第5个参数, 就可以将
			std::size_t pos = content.find(keyword);
			if (pos == std::string::npos)
				return "keyword does not exist!";

			std::size_t begin = 0;
			std::size_t end = content.size() - 1;

			// 获取前50字节 和 后100字节的迭代器位置
			if (pos > begin + prevStep)
				begin += (pos - prevStep);
			if (pos + nextStep < end)
				end = pos + nextStep;

			if (begin >= end)
				return "nothing!";

			// 获取摘要
			std::string desc;
			if (pos <= begin + prevStep)
				desc = "...";
			desc += content.substr(begin, end - begin);
			if (pos + nextStep < end)
				desc += "...";

			return desc;
		}
	};
} // namespace ns_searcher
