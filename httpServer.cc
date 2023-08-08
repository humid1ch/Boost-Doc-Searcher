#include <iostream>
#include <pthread.h>
#include "util.hpp"
#include "daemonize.hpp"
#include "searcher.hpp"
#include "logMessage.hpp"
#include "httplib.h"

const std::string& input = "./data/output/raw";
const std::string& rootPath = "./wwwRoot";

int main() {
	// 守护进程设置
	daemonize();
	// 日志系统
	class log logSvr;
	logSvr.enable();

	ns_searcher::searcher searcher;
	searcher.initSearcher(input);

	httplib::Server svr;

	svr.set_base_dir(rootPath.c_str());
	svr.Get("/s", [&searcher](const httplib::Request& request, httplib::Response& response) {
		// 首先, 网页发起请求 如果需要带参数, 则是需要以 key=value的格式在url中 或者 正文有效中传参的
		// 就像我们使用一般搜索引擎搜索一样:
		// 如果在 google搜索http, 那么 url就会变为 https://www.google.com/search?q=http&sxsrf=AB5stBgDxDV91zrABB
		// 其中 q=http 就是一对 key=value 值, 而 httplib::Request::has_param() 就是识别请求url中是否携带了 某个key=value
		// 本项目中, 我们把搜索内容 的key=value对, 设置为word=搜索内容
		if (!request.has_param("word")) {
			// url中没有 word 键值
			// set_content() 第一个参数是设置正文内容, 第二个参数是 正文内容类型等属性
			response.set_content("请输入内容后搜索", "text/plain; charset=utf-8");
		}
		std::string searchContent = request.get_param_value("word");
		LOG(NOTICE, "User search:: %s", searchContent.c_str());
		// std::cout << "User search:: " << searchContent << std::endl;

		std::string searchJsonResult;
		searcher.search(searchContent, &searchJsonResult);
		// 搜获取到搜索结果之后 设置相应内容
		response.set_content(searchJsonResult, "application/json");
	});
	// svr.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
	// 	res.set_content("Hello World!", "text/plain");
	// });

	LOG(NOTICE, "服务器启动成功...");
	// std::cout << "服务器启动成功..." << std::endl;
	svr.listen("0.0.0.0", 8080);

	return 0;
}
