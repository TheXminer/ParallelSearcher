#pragma once
#include "CustomHashTable.h"
#include "ThreadPool.h"
#include "FileManager.h"
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#define BATCH_UPDATE_INTERVAL_MS 1000

class Searcher
{
public:
	Searcher(std::shared_ptr<ThreadPool> threadPool);
	~Searcher();
	void AddFile(const uint32_t fileID);
	std::vector<CustomHashTable::WordInfo>& SearchPhrase(const std::string& phrase);

private:
	CustomHashTable hashTable;
	std::shared_ptr<ThreadPool> threadPool;

	int fileCount;
	bool stopFlag = false;
	std::mutex fileAddMutex;
	std::vector<uint32_t> filesToAdd;
	
	const std::vector<char> delimiters = {
	' ', '\n', '\t',
	',', '.', '!', '?', ';', ':', '"', '\'', '(', ')', '[', ']', '{', '}',
	'-', '_', '/', '\\', '*'
	};

	const std::vector<std::string> ignoreWords = {
		"the", "a", "an", "and", "or", "but", "if", "then", "else",
		"i", "you", "he", "she", "it", "we", "they",
		"is", "are", "was", "were", "be", "been", "being",
		"of", "at", "in", "on", "to", "for", "with", "without",
		"as", "this", "that", "these", "those",
		"from", "by", "about", "into", "over", "under",
		"not", "so", "no", "yes" };

private:
	void batchUpdate();
	std::vector<std::pair<std::string, uint64_t>>& splitString(const std::string& str);
	void loadFileContent(const uint32_t fileID);
};

