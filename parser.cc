#include <boost/filesystem/operations.hpp>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <boost/filesystem.hpp>
#include "util.hpp"

// 此程序是一个文档解析器
// boost文档的html文件中, 有许多的各种<>标签. 这些都是对搜索无关的内容, 所以需要清除掉
// 本程序实现以下功能:
//  1. 使用boost库提供的容器, 递归遍历 ./data/input 目录下(包括子目录)的所有文档html, 并保存其文件名到 vector中
//  2. 通过 vector 中保存的 文档名, 找到文档 并对 所有文档的内容去标签
//  3. 还是通过 vector中保存的文档名
//     读取所有文档的内容,  以每个文档 标题 内容 url 结构构成一个docInfo结构体. 并以 vector 存储起来
//  4. 将用vector 存储起来的所有文档的docInfo 存储到 ./data/output/raw 文件中, 每个文档的info用 \3 分割
// 至此 完成对所有文档的 解析

//  为提高解析效率, 可以将 2 3 步骤合并为一个函数:
//  每对一个文档html文件去标签之后, 就获取文档内容构成docInfo结构体, 并存储到 vector 中

// 代码规范
//  const & 表示输入型参数: const std::string&
//  * 表示输出型参数: std::string*
//  & 表示输入输出型参数: std::string&

#define SEP '\3'

const std::string srcPath = "data/input";	  // 存放所有文档的目录
const std::string output = "data/output/raw"; // 保存文档所有信息的文件

typedef struct docInfo {
	std::string _title;	  // 文档的标题
	std::string _content; // 文档内容
	std::string _url;	  // 该文档在官网中的url
} docInfo_t;

bool enumFile(const std::string& srcPath, std::vector<std::string>* filesList);
bool parseDocInfo(const std::vector<std::string>& filesList, std::vector<docInfo_t>* docResults);
bool saveDocInfo(const std::vector<docInfo_t>& docResults, const std::string& output);

int main() {
	std::vector<std::string> filesList;
	// 1. 递归式的把每个html文件名带路径，保存到filesList中，方便后期进行一个一个的文件进行读取
	if (!enumFile(srcPath, &filesList)) {
		// 获取文档html文件名失败
		std::cerr << "Failed to enum file name!" << std::endl;
		return ENUM_ERROR;
	}

	// 走到这里 获取所有文档html文件名成功
	// 2. 按照filesList读取每个文档的内容，并进行去标签解析
	// 3. 并获取文档的内容 以 标题 内容 url 构成docInfo结构体, 存储到vector中
	std::vector<docInfo_t> docResults;
	if (!parseDocInfo(filesList, &docResults)) {
		// 解析文档内容失败
		std::cerr << "Failed to parse document information!" << std::endl;
		return PARSEINFO_ERROR;
	}

	// 走到这里 获取所有文档内容 并以 docInfo 结构体形式存储到vector中成功
	// 4: 把解析完毕的各个文件内容，写入到output , 按照\3作为每个文档的分割符
	if (!saveDocInfo(docResults, output)) {
		std::cerr << "Failed to save document information!" << std::endl;
		return SAVEINFO_ERROR;
	}

	return 0;
}

bool enumFile(const std::string& srcPath, std::vector<std::string>* filesList) {
	// 使用 boost库 来对路径下的文档html进行 递归遍历
	namespace bs_fs = boost::filesystem;

	// 根据 srcPath 构建一个path对象
	bs_fs::path rootPath(srcPath);
	if (!bs_fs::exists(rootPath)) {
		// 指定的路径不存在
		std::cerr << srcPath << " is not exists" << std::endl;
		return false;
	}

	// boost库中 可以递归遍历目录以及子目录中 文件的迭代器, 不初始化可看作空
	bs_fs::recursive_directory_iterator end;
	// 再从 rootPath 构建一个迭代器, 递归遍历目录下的所有文件
	for (bs_fs::recursive_directory_iterator iter(rootPath); iter != end; iter++) {
		// 目录下 有目录文件 也有普通文件, 普通文件不仅仅只有 .html文件, 所以还需要过滤掉目录文件和非.html文件

		if (!bs_fs::is_regular_file(*iter)) {
			// 不是普通文件
			continue;
		}
		if (iter->path().extension() != ".html") { // boost::path 对象的 extension()接口, 可以获取到所指文件的后缀
			// 不是 html 文件
			continue;
		}

		std::cout << "Debug:  " << iter->path().string() << std::endl;

		// 走到这里的都是 .html 文件
		// 将 文件名存储到 filesList 中
		filesList->push_back(iter->path().string());
	}

	return true;
}

bool parseTitle(const std::string& fileContent, std::string* title) {
	// 简单分析一个html文件, 可以发现 <title>标签只有一对 格式是这样的: <title> </title>, 并且<title>内部不会有其他字段
	// 在 > < 之间就是这个页面的 title , 所以我们想要获取 title 就只需要获取<title>和</title> 之间的内容就可以了
	// 1. 先找 <title>
	std::size_t begin = fileContent.find("<title>");
	if (begin == std::string::npos) {
		// 没找到
		return false;
	}
	std::size_t end = fileContent.find("</title>");
	if (end == std::string::npos) {
		// 没找到
		return false;
	}

	// 走到这里就是都找到了, 然后就可以获取 > <之间的内容了
	begin += std::string("<title>").size(); // 让begin从>后一位开始
	if (begin > end) {
		return false;
	}

	*title = fileContent.substr(begin, end - begin);

	return true;
}
bool parseContent(const std::string& fileContent, std::string* content) {
	// parseContent 需要实现的功能是, 清除标签
	// html的语法都是有一定的格式的. 虽然标签可能会成对出现 <head></head>, 也可能会单独出现 <mate>
	// 但是 标签的的内容永远都是在相邻的 < 和 >之间的, 在 > 和 < 之间的则是是正文的内容
	// 并且, html文件中的第一个字符永远都是 <, 并且之后还会有> 成对出现
	// 可以根据这种语法特性来遍历整个文件内容 清除标签
	enum status {
		LABLE,	// 表示在标签内
		CONTENT // 表示在正文内
	};

	enum status s = LABLE; // 因为首先的状态一定是在标签内
	for (auto c : fileContent) {
		switch (s) {
		case LABLE: {
			// 如果此时的c表示标签内的内容, 不做处理
			// 除非 当c等于>时, 表示即将出标签, 此时需要切换状态
			if (c == '>') {
				s = CONTENT;
			}

		} break;
		case CONTENT: {
			// 此时 c 表示正文的内容, 所以需要存储在 content中, 但是为了后面存储以及分割不同文档, 所以也不要存储 \n, 将 \n 换成 ' '存储
			// 并且, 当c表示<时, 也就不要存储了, 表示已经出了正文内容, 需要切换状态
			if (c == '<') {
				s = LABLE;
			}
			else {
				if (c == '\n') {
					c = ' ';
				}
				*content += c;
			}
		} break;
		default:
			break;
		}
	}

	return true;
}
bool parseUrl(const std::string& filePath, std::string* url) {
	// 先去官网看一看 官网的url是怎么分配的: https://www.boost.org/doc/libs/1_82_0/doc/html/function/reference.html
	// 我们本地下载的boost库的html路径又是怎么分配的: boost_1_82_0/doc/html/function/reference.html
	// 我们在程序中获取的文件路径 即项目中文件的路径 又是什么: data/input/function/reference.html

	// 已经很明显了, url 的获取就是 https://www.boost.org/doc/libs/1_82_0/doc/html + /function/reference.html
	// 其中, 如果版本不变的话, https://www.boost.org/doc/libs/1_82_0/doc/html 是固定的
	// 而后半部分, 则是 filePath 除去 data/input, 也就是 const std::string srcPath = "data/input" 部分

	// 所以, url的获取也很简单
	std::string urlHead = "https://www.boost.org/doc/libs/1_82_0/doc/html";
	std::string urlTail = filePath.substr(srcPath.size()); // 从srcPath长度处向后截取

	*url = urlHead + urlTail;

	return true;
}

static void ShowDoc(const docInfo_t& doc) {
	std::cout << "title: " << doc._title << std::endl;
	std::cout << "content: " << doc._content << std::endl;
	std::cout << "url: " << doc._url << std::endl;
}

bool parseDocInfo(const std::vector<std::string>& filesList, std::vector<docInfo_t>* docResults) {
	// parseDocInfo 是对文档html文件的内容做去标签化 并 获取 title content url 构成结构体
	// 文档的路径都在 filesList 中存储着, 所以需要遍历 filesList 处理文件
	for (const std::string& filePath : filesList) {
		// 获取到文档html的路径之后, 就需要对 html文件进行去标签化等一系列解析操作了

		// 1. 读取文件内容到 string 中
		std::string fileContent;
		if (!ns_util::fileUtil::readFile(filePath, &fileContent)) {
			// 读取文件内容失败
			continue;
		}

		// 读取到文档html文件内容之后, 就可以去标签 并且 获取 title content 和 url了
		docInfo_t doc;
		// 2. 解析并获取title, html文件中只有一个 title标签, 所以再去标签之前 获取title比较方便
		if (!parseTitle(fileContent, &doc._title)) {
			// 解析title失败
			continue;
		}

		// 3. 解析并获取文档有效内容, 去标签的操作实际就是在这一步进行的
		if (!parseContent(fileContent, &doc._content)) {
			// 解析文档有效内容失败
			continue;
		}

		// 4. 获取 官网的对应文档的 url
		if (!parseUrl(filePath, &doc._url)) {
			continue;
		}

		ShowDoc(doc);
		// 做完上面的一系列操作 走到这里时 如果没有不过 doc 应该已经被填充完毕了
		// doc出此次循环时就要被销毁了, 所以将doc 设置为将亡值 可以防止拷贝构造的发生 而使用移动语义来向 vector中添加元素
		// 这里发生拷贝构造是非常的消耗资源的 因为 doc._content 非常的大
		docResults->push_back(std::move(doc));
	}

	return true;
}

bool saveDocInfo(const std::vector<docInfo_t>& docResults, const std::string& output) {
	// 最后就是将 已经结构化的所有的文档数据, 以一定的格式存储在指定的文件中.
	// 以什么格式存储呢? 每个文档都是结构化的数据: _title _content _url.
	// 我们可以将 三个字段以'\3'分割, 不过 _url后不用'\3' 而是用'\n'
	// 因为, 像文件中写入不能只关心写入, 还要考虑读取时的问题. 方便的 读取文本文件, 通常可以用 getline 来获取一行数据
	// 所以, 当以这种格式 (_title\3_content\3_url\n) 将 文档数据存储到文件中时, getline() 成功读取一次文件内容, 获取的就是一个文档的所有有效内容.

	// 按照二进制方式进行写入, 二进制写入, 写入什么就是什么 转义字符也不会出现被优化改变的现象
	std::ofstream out(output, std::ios::out | std::ios::binary);
	if (!out.is_open()) {
		// 文件打开失败
		std::cerr << "open " << output << " failed!" << std::endl;
		return false;
	}

	// 就可以进行文件内容的写入了
	for (auto& item : docResults) {
		std::string outStr;
		outStr = item._title;
		outStr += SEP;
		outStr += item._content;
		outStr += SEP;
		outStr += item._url;
		outStr += '\n';

		out.write(outStr.c_str(), outStr.size());
	}

	out.close();

	return true;
}
