#pragma once
#include "ThreadPool.h"
#include "Searcher.h"
#include "FileManager.h"
#include "Response.h"
#include <map>

class Controller
{
private:
	std::shared_ptr<ThreadPool> threadPool;
	Searcher searcher;

	using Handler = std::function<Response(const std::string&)>;
	std::map<std::string, Handler> routeHandlers;

public:
	Controller(std::shared_ptr<ThreadPool> threadPool);
	//~Controller();

	//POST /addfile
	Response handleAddFile(const std::string& request);

	//GET /search?word=example
	Response handleSearchPhrase(const std::string& request);

	//GET /file?id=123
	Response handleGetFile(const std::string& request);

	std::string JSONifySearchResults(const std::vector<std::pair<std::string, std::string>>& results);
	std::string urlDecode(const std::string& str);
	std::string getParam(const std::string& req, const std::string& key);

	void handleClient(int clientSocket);
	std::string getRequest(int clientSocket);
	void sendResponse(int clientSocket, Response response);
	std::string getRequestInfo(const std::string& req);

	void stopSearcher() {
		searcher.stopUpdate();
	}
};