#pragma once
#include "ConcurrentHashMap.h"
#include "ThreadPool.h"
#include "FileManager.h"
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <unordered_set>
#define BATCH_UPDATE_INTERVAL_MS 10000
#define PART_SIZE 100

class Searcher
{
public:
	struct SearchResult {
		uint64_t fileID;
		std::string fileName;
		std::string textPart;

		SearchResult(uint64_t id, const std::string& name, const std::string& part)
			: fileID(id), fileName(name), textPart(clearTextPart(part)) {
		}

		std::string clearTextPart(const std::string& textPart) const {
			std::string cleaned;
			cleaned.reserve(textPart.size());
			for (char c : textPart) {
				if (c == '\n' || c == '\r') {
					cleaned += ' ';
				}
				else {
					cleaned += c;
				}
			}
			return cleaned;
		}

		std::string escapeJsonString(const std::string& input) const {
			std::string output;
			for (char c : input) {
				switch (c) {
				case '"': output += "\\\""; break;
				case '\\': output += "\\\\"; break;
				case '\n': output += "\\n"; break;
				case '\r': output += "\\r"; break;
				default: output += c; break;
				}
			}
			return output;
		}

		std::string toJSON() const {
			// Впевніться, що textPart і fileName екрановані
			std::string safeTextPart = escapeJsonString(textPart);
			std::string safeFileName = escapeJsonString(fileName);

			return "{\"fileid\": " + std::to_string(fileID) +
				", \"filename\": \"" + safeFileName +
				"\", \"textpart\": \"" + safeTextPart + "\"}";
		}
	};
	Searcher(std::shared_ptr<ThreadPool> threadPool);
	~Searcher();
	void AddFile(const uint64_t fileID);
	void stopUpdate() { stopFlag = true; }
	std::vector<SearchResult> SearchPhrase(const std::string& phrase);

private:
	ConcurrentHashMap hashTable;
	std::shared_ptr<ThreadPool> threadPool;

	std::atomic<int> fileCount{ 0 };
	bool stopFlag = false;
	std::mutex fileAddMutex;
	std::vector<uint64_t> filesToAdd;
	
	const std::vector<char> delimiters = {
		' ', '\n', '\t', '\r', '\f', '\v', '\0',
		',', '.', '!', '?', ';', ':',
		'"', '\'', '(', ')', '[', ']', '{', '}', '<', '>',
		'-', '_',
		'/', '\\', '*', '+', '=', '%', '^', '~',
		'#', '@', '&', '|', '$'
	};

	const std::unordered_set<std::string> ignore_set = {
		"the", "a", "an", "and", "or", "but", "if", "then", "else",
		"i", "you", "he", "she", "it", "we", "they",
		"is", "are", "was", "were", "be", "been", "being",
		"of", "at", "in", "on", "to", "for", "with", "without",
		"as", "this", "that", "these", "those",
		"from", "by", "about", "into", "over", "under",
		"not", "so", "no", "yes" };

private:
	void batchUpdate();
	void loadFileContent(const uint64_t fileID);

	using WordToken = std::pair<std::string, uint64_t>;
	using WordTokens = std::vector<WordToken>;
	WordTokens splitString(const std::string& str);
	WordTokens tokenizeWord(const WordTokens& tokens);
	std::string CleanWordForIndexing(const std::string& word);
};

