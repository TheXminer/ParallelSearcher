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
	struct WordLocation {
		size_t wordPosition;
		size_t byteOffset;
		bool operator<(const WordLocation& other) const {
			return this->wordPosition < other.wordPosition;
		}
		WordLocation() : wordPosition(0), byteOffset(0) {}
		WordLocation(size_t wordPos, size_t byteOff)
			: wordPosition(wordPos), byteOffset(byteOff) {
		}
	};
	struct WordInfo {
		std::string word;
		WordLocation wordLocation;
		uint32_t fileID;

		WordInfo(std::string word, size_t wordNumber, size_t absolutePosition, uint64_t fileID)
			: word(word), wordLocation(wordNumber, absolutePosition), fileID(fileID){
		}
	};

	CustomHashTable();
	~CustomHashTable();

	void Insert(const WordInfo&);
	using WordPositions = std::map<uint64_t, std::vector<WordLocation>>;
	WordPositions Find(const char* word);

private:
	struct HashEntry {
		std::string word;
		WordPositions positions; // fileID -> list of absolute and word positions
		HashEntry* next;
		HashEntry(const WordInfo& wordInfo)
			: word(wordInfo.word), next(nullptr)
		{
			positions[wordInfo.fileID].push_back(WordLocation(wordInfo.wordLocation.wordPosition, wordInfo.wordLocation.byteOffset));
		}
		HashEntry(const WordPositions& pos, const std::string& w)
			: word(w), positions(pos), next(nullptr) {
		}
		void addPosition(const WordInfo& wordInfo) {
			positions[wordInfo.fileID].push_back(WordLocation(wordInfo.wordLocation.wordPosition, wordInfo.wordLocation.byteOffset));
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
	std::atomic<size_t> currentSize{ 0 };
};

