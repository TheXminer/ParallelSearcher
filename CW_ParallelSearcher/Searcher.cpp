#include "Searcher.h"

Searcher::Searcher(std::shared_ptr<ThreadPool> threadPool) : threadPool(threadPool), fileCount(0)
{
	FileManager::Initialize();
    filesToAdd = FileManager::GetAllFileIds();
	threadPool->enqueue([this]() { this->batchUpdate(); });
}

Searcher::~Searcher()
{
}

void Searcher::AddFile(const uint64_t fileID)
{
	std::lock_guard<std::mutex> lock(fileAddMutex);
	filesToAdd.push_back(fileID);
}

using DocIndices = CustomHashTable::WordPositions;
DocIndices intersectDocIndices(const DocIndices& currentResults, const DocIndices& nextWordDocs, int offset)
{
    DocIndices result;

    for (const auto& curResult : currentResults)
    {
        auto fileID = curResult.first;
        const auto& currentLocations = curResult.second;

        auto itNextDoc = nextWordDocs.find(fileID);
        if (itNextDoc == nextWordDocs.end()) continue;

        const auto& nextLocations = itNextDoc->second;

        for (const auto& curLoc : currentLocations)
        {
            uint64_t targetIndex = curLoc.wordPosition + offset;

            auto it = std::lower_bound(nextLocations.begin(), nextLocations.end(), targetIndex,
                [](const CustomHashTable::WordLocation& loc, uint64_t val) {
                    return loc.wordPosition < val;
                });

            if (it != nextLocations.end() && it->wordPosition == targetIndex)
            {
                result[fileID].push_back(curLoc);
            }
        }
    }
    return result;
}

std::vector<std::pair<std::string, std::string>> Searcher::SearchPhrase(const std::string& phrase)
{
    std::vector<std::pair<std::string, std::string>> results;

    auto words = splitString(phrase);
    if (words.empty())
        return results;

    auto currentMatches = hashTable.Find(words[0].first.c_str());

    for (size_t i = 1; i < words.size(); ++i)
    {
        if (currentMatches.empty()) break;

        auto nextWordResults = hashTable.Find(words[i].first.c_str());

        currentMatches = intersectDocIndices(currentMatches, nextWordResults, i);
    }

    for (const auto& docIndex : currentMatches)
    {
        uint64_t fileID = docIndex.first;
        const std::string& fileName = FileManager::getFileName(fileID);
        
  //      for(const auto& loc : docIndex.second)
  //      {
		//	std::cout << "Found in file ID " << fileID << " (" << fileName << ") at word position " << loc.wordPosition << " (byte offset " << loc.byteOffset << ")" << std::endl;
		//}
        for (const auto& loc : docIndex.second)
        {
            std::string textPart = FileManager::GetFilePart(fileID, loc.byteOffset, PART_SIZE);
            results.emplace_back(fileName, textPart);
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

std::vector<std::pair<std::string, uint64_t>> Searcher::splitString(const std::string& str)
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
			std::string word = toLower(str.substr(start, end - start));
			result.push_back(std::pair<std::string, uint64_t>(word, start));
		}
		start = end + 1;
	}
	return result;
}

std::string Searcher::toLower(const std::string& str)
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
        [](unsigned char c) { return std::tolower(c); });
	return lowerStr;
}

void Searcher::loadFileContent(const uint64_t fileID)
{
	std::string content = FileManager::getFileText(fileID);
	auto words = splitString(content);
	int counter = 0;
	for (size_t pos = 0; pos < words.size(); ++pos)
	{
		const std::string& word = words[pos].first;
		if (std::find(ignoreWords.begin(), ignoreWords.end(), word) != ignoreWords.end())
		{
			continue;
		}

		hashTable.Insert(CustomHashTable::WordInfo(word, pos, counter, fileID));
		counter++;
	}
	++fileCount;
}
