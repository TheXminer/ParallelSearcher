#pragma once
#include "ThreadPool.h"
#include "Searcher.h"
#include "FileManager.h"

class Controller
{
private:
	std::shared_ptr<ThreadPool> threadPool;
	Searcher searcher;

public:
	Controller(std::shared_ptr<ThreadPool> threadPool);
	~Controller();

	//POST /addfile
	void handleAddFile(const std::string& filePath);

	//GET /search?word=example
	std::vector<std::pair<std::string, size_t>> handleSearchWord(const std::string& word);

	//GET /file?id=123
	FILE handleGetFile(uint64_t fileId);



	void handleClient(int clientSocket);
	std::string getMethod(const std::string& req);
	std::string getPath(const std::string& req);
};

