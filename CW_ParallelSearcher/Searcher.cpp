#include "Searcher.h"
#include <regex>

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

std::vector<Searcher::SearchResult> Searcher::SearchPhrase(const std::string& phrase)
{
    std::vector<SearchResult> results;

    auto words = splitString(phrase);
	words = tokenizeWord(words);
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
			auto fileName = FileManager::getFileName(fileID);
            results.emplace_back(SearchResult(fileID, fileName, textPart));
        }
    }

    return results;
}

void Searcher::batchUpdate()
{
	while (!stopFlag)
	{
        fileAddMutex.lock();
		for (const auto& fileID : filesToAdd)
		{
            threadPool->enqueue(
                [this, fileID]() {
                    this->loadFileContent(fileID);
                }
            );
		}
		filesToAdd.clear();
        fileAddMutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(BATCH_UPDATE_INTERVAL_MS));
	}
}

Searcher::WordTokens Searcher::splitString(const std::string& str)
{
    const std::string_view str_view(str);
    WordTokens result;

    result.reserve(str.length() / 5);

    size_t start = 0;

    while (start < str.length())
    {
        size_t end = str_view.find_first_of(delimiters.data(), start);

        if (end == std::string_view::npos)
        {
            end = str.length();
        }
        if (end > start)
        {
            std::string_view word_view = str_view.substr(start, end - start);
            std::string word(word_view);
            result.push_back({ std::move(word), start });
        }

        start = end + 1;
    }
    return result;
}
std::string Searcher::CleanWordForIndexing(const std::string& word) {
    static const std::regex allowed(R"([^A-Za-z0-9\-'])");
    auto result = std::regex_replace(word, allowed, "");;

    std::string clean_word = result;
    std::transform(clean_word.begin(), clean_word.end(), clean_word.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return clean_word;
}
Searcher::WordTokens Searcher::tokenizeWord(const WordTokens& tokens)
{
    WordTokens filtered_tokens = tokens;

    auto new_end = std::remove_if(
        filtered_tokens.begin(),
        filtered_tokens.end(),
        [this](const WordToken& token) {
            return ignore_set.count(token.first);
        }
    );
    filtered_tokens.erase(new_end, filtered_tokens.end());

    return filtered_tokens;
}

void Searcher::loadFileContent(const uint64_t fileID)
{
	std::string content = FileManager::getFileText(fileID);
	auto words = splitString(content);
	for (size_t pos = 0; pos < words.size(); ++pos)
	{
        auto cleanWord = CleanWordForIndexing(words[pos].first);
		hashTable.Insert(CustomHashTable::WordInfo(cleanWord, pos, words[pos].second, fileID));
	}
    std::cout << fileCount.load() << ": " << fileID << std::endl;
    fileCount.fetch_add(1);
}
