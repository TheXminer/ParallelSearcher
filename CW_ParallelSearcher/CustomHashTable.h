#pragma once
#include <vector>
#include <shared_mutex>
#include <map>
#include <set>
#define RESIZE_FACTOR 2
#define LOAD_FACTOR 0.7
#define START_SIZE 100

class CustomHashTable
{
public:
	struct WordInfo {
		std::string word;
		size_t wordNumber;
		size_t absolutePosition;
		uint32_t fileID;

		WordInfo(std::string word, size_t wordNumber, size_t absolutePosition)
			: word(word), wordNumber(wordNumber), absolutePosition(absolutePosition) {
			fileID = 0;
		}
	};

	CustomHashTable();
	~CustomHashTable();

	void Insert(const WordInfo&);
	std::map<uint32_t, std::set<std::pair<uint64_t, uint64_t>>> Find(const char* word);

private:
	struct HashEntry {
		std::string word;
		std::map<uint32_t, std::set<std::pair<uint64_t, uint64_t>>> positions; // fileID -> list of absolute and word positions
		HashEntry* next;
		HashEntry(const WordInfo& wordInfo)
			: word(wordInfo.word), next(nullptr)
		{
			positions[wordInfo.fileID].emplace(wordInfo.absolutePosition, wordInfo.wordNumber);
		}
		HashEntry(const std::map<uint32_t, std::set<std::pair<uint64_t, uint64_t>>>& pos, const std::string& w)
			: word(w), positions(pos), next(nullptr) {
		}
		void addPosition(const WordInfo& wordInfo) {
			positions[wordInfo.fileID].emplace(wordInfo.absolutePosition, wordInfo.wordNumber);
		};
	};

	struct HashBucket {
		HashEntry* head;
		HashEntry* tail;
		mutable std::shared_mutex mtx;
		HashBucket() : head(nullptr), tail(nullptr) {}
		void addEntry(HashEntry* entry) {
			if (!head) {
				head = entry;
				tail = entry;
				return;
			}
			tail->next = entry;
			tail = entry;
		}
	};
private:
	using SnapshotTable = std::vector<HashBucket>;
	using Snapshot = std::shared_ptr<SnapshotTable>;

	const bool contains(const Snapshot& snapshot, const char* word);
	const size_t HashFunction(const char* word, size_t size);
	HashEntry* findEntry(const SnapshotTable& snapshot, const std::string& word);
	Snapshot resize(const Snapshot& snapshot);
	std::mutex resizeMutex;
	std::shared_mutex snapshotMutex;
	Snapshot curSnapshot;
	size_t currentSize = 0;
};

