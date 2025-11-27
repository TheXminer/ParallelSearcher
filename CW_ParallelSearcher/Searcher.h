#pragma once
#include "CustomHashTable.h"
#include "ThreadPool.h"
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>

class Searcher
{
public:
	Searcher(std::shared_ptr<ThreadPool>);
	~Searcher();
	void AddFile(const uint32_t fileID);
	std::vector<std::pair<std::string, size_t>> SearchPhrase(const std::string& word);

private:
	CustomHashTable hashTable;
	std::shared_ptr<ThreadPool> threadPool;

	int fileCount;
	std::mutex fileAddMutex;
	std::vector<std::string> filesToAdd;
	std::atomic<std::map<uint16_t, std::string>*> fileByIndex;

	std::vector<char> delimiters;
	std::vector<std::string> ignoreWords;

private:
	void batchUpdate();
	std::vector<std::string> splitString(const std::string& str);
	std::string loadFileContent(const std::string& filePath);
};

