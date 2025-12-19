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

struct MatchedLocation {
    uint64_t fileID;
    size_t wordPositionW1;
    size_t wordPositionW2;
};

using WordLocation = ConcurrentHashMap::WordLocation;
using WordLocations = std::vector<WordLocation>;
WordLocations intersectDocIndices(const WordLocations& currentResults, const WordLocations& nextWordDocs)
{
    std::unordered_map<uint64_t, std::vector<const WordLocation*>> map1;
    std::unordered_map<uint64_t, std::vector<size_t>> map2;

    for (const auto& loc : currentResults)
        map1[loc.fileID].push_back(&loc);

    for (const auto& loc : nextWordDocs)
        map2[loc.fileID].push_back(loc.wordPosition);

    for (auto& [file, positions] : map1)
        std::sort(positions.begin(), positions.end(),
            [](const WordLocation* a, const WordLocation* b) { return a->wordPosition < b->wordPosition; });

    for (auto& [file, positions] : map2)
        std::sort(positions.begin(), positions.end());

    WordLocations results;

    // Для кожного файлу з обома словами
    for (auto& [file, positions1] : map1) {
        auto it2 = map2.find(file);
        if (it2 == map2.end()) continue;
        auto& positions2 = it2->second;

        size_t i = 0, j = 0;
        while (i < positions1.size() && j < positions2.size()) {
            if (positions1[i]->wordPosition + 1 == positions2[j]) {
                results.push_back(WordLocation(file, positions1[i]->byteOffset, positions2[j]));
                ++i; ++j;
            }
            else if (positions1[i]->wordPosition + 1 < positions2[j]) {
                ++i;
            }
            else {
                ++j;
            }
        }
    }

    return results;

}

std::vector<Searcher::SearchResult> Searcher::SearchPhrase(const std::string& phrase)
{
    std::vector<SearchResult> results;

    auto words = splitString(phrase);
	for(auto& word : words) {
        word.first = CleanWordForIndexing(word.first);
	}
    if (words.empty())
        return results;

    auto currentMatches = hashTable.find(words[0].first.c_str());

    for (size_t i = 1; i < words.size(); ++i)
    {
        if (currentMatches.empty()) break;

        auto nextWordResults = hashTable.find(words[i].first.c_str());

        currentMatches = intersectDocIndices(currentMatches, nextWordResults);
    }

    for (const auto& phrasePosition : currentMatches)
    {
        uint64_t fileID = phrasePosition.fileID;
        const std::string& fileName = FileManager::getFileName(fileID);
        
        std::string textPart = FileManager::GetFilePart(fileID, phrasePosition.byteOffset, PART_SIZE);
        results.emplace_back(SearchResult(fileID, fileName, textPart));
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
    std::string cleanWord;
    cleanWord.reserve(word.size());
    for (unsigned char c : word) {
        if (std::isalnum(c) || c == '-' || c == '\'') {
            cleanWord.push_back(std::tolower(c));
        }
    }
    std::transform(cleanWord.begin(), cleanWord.end(), cleanWord.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return cleanWord;
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
        hashTable.insert(cleanWord, WordLocation(fileID, words[pos].second, pos));
	}
    //std::cout << fileCount.load() << ": " << fileID << std::endl;
    fileCount.fetch_add(1);
}
