#include "Searcher.h"

Searcher::Searcher(std::shared_ptr<ThreadPool> threadPool) : threadPool(threadPool), fileCount(0)
{
	threadPool->enqueue([this]() { this->batchUpdate(); });
}

void Searcher::AddFile(const uint32_t fileID)
{
	std::lock_guard<std::mutex> lock(fileAddMutex);
	filesToAdd.push_back(fileID);
}

std::vector<std::pair<std::string, size_t>>& Searcher::SearchPhrase(const std::string& phrase)
{
	std::vector<std::pair<std::string, size_t>> results;

	auto words = splitString(phrase);
	if (words.empty())
		return results;

	auto firstWordResults = hashTable.Find(words[0].c_str());

	for (const auto& elem : firstWordResults)
	{
		const char* fileWord = elem.first;
		size_t position = elem.second;
		bool match = true;

		for (size_t i = 1; i < words.size(); i++)
		{
			auto nextWordResults = hashTable.Find(words[i].c_str());

			auto it = std::find_if(
				nextWordResults.begin(),
				nextWordResults.end(),
				[&](const std::pair<char*, size_t>& p)
				{
					return strcmp(p.first, fileWord) == 0 &&
						p.second == position + i;
				}
			);

			if (it == nextWordResults.end())
			{
				match = false;
				break;
			}
		}

		if (match)
		{
			results.emplace_back(std::string(fileWord), position);
		}
	}

	return results;
}
void Searcher::batchUpdate()
{
	while (!stopFlag)
	{
		std::lock_guard<std::mutex> lock(fileAddMutex);
		for (const auto& fileID : filesToAdd)
		{
			loadFileContent(fileID);
		}
		filesToAdd.clear();
		std::this_thread::sleep_for(std::chrono::milliseconds(BATCH_UPDATE_INTERVAL_MS));
	}
}

std::vector<std::pair<std::string, uint64_t>>& Searcher::splitString(const std::string& str)
{
	std::vector<std::pair<std::string, uint64_t>> result;
	size_t start = 0;
	while (start < str.length())
	{
		size_t end = str.find_first_of(delimiters.data(), start);
		if (end == std::string::npos)
		{
			end = str.length();
		}
		if (end > start)
		{
			result.push_back(std::pair<std::string, uint64_t>(str.substr(start, end - start), start));
		}
		start = end + 1;
	}
	return result;
}

void Searcher::loadFileContent(const uint32_t fileID)
{
	std::string content = FileManager::getFileText(fileID);
	auto words = splitString(content);
	for (size_t pos = 0; pos < words.size(); ++pos)
	{
		const std::string& word = words[pos].first;
		if (std::find(ignoreWords.begin(), ignoreWords.end(), word) != ignoreWords.end())
		{
			continue;
		}

		hashTable.Insert(word.c_str(), pos, fileID);
	}
	++fileCount;
}
