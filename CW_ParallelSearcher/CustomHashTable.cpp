#include "CustomHashTable.h"


CustomHashTable::CustomHashTable() {
	curSnapshot = std::make_shared<std::vector<HashBucket>>();
}

CustomHashTable::~CustomHashTable() {
	std::unique_lock<std::shared_mutex> lock(snapshotMutex);
	auto snapshot = curSnapshot;
	for (HashBucket& bucket : *snapshot) {
		HashEntry* current = bucket.head;
		while (current) {
			HashEntry* toDelete = current;
			current = current->next;
			delete toDelete;
		}
	}
}

const bool CustomHashTable::contains(const Snapshot& snapshot, const char* word)
{
	size_t index = HashFunction(word, snapshot.get()->size());
	std::shared_lock<std::shared_mutex> lock(snapshot->at(index).mtx);
	HashEntry* current = snapshot->at(index).head;
	while (current) {
		if (current->word == word) {
			return true;
		}
		current = current->next;
	}
	return false;
}

const size_t CustomHashTable::HashFunction(const char* word, size_t size) {
	size_t hash = 5381;
	int c;
	while ((c = *word++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash % size;
}

CustomHashTable::HashEntry* CustomHashTable::findEntry(const SnapshotTable& snapshot, const std::string& word)
{
	size_t index = HashFunction(word.c_str(), snapshot.size());
	HashEntry* current = snapshot.at(index).head;
	while (current) {
		if (current->word == word) {
			return current;
		}
		current = current->next;
	}
	return nullptr;
}

CustomHashTable::Snapshot CustomHashTable::resize
	(const Snapshot& snapshot) 
{
	auto curSize = snapshot.get()->size();
	curSize = (curSize == 0) ? 10 : curSize;
	auto loadFactor = static_cast<double>(currentSize) / curSize;
	auto newSize = (loadFactor > LOAD_FACTOR) ? curSize * RESIZE_FACTOR: curSize;
	auto newBuckets = new SnapshotTable(newSize);

	for (const HashBucket& bucket : *snapshot) {
		HashEntry* current = bucket.head;
		while (current) {
			HashEntry* newEntry = new HashEntry(current->positions, current->word);
			size_t newIndex = HashFunction(current->word.c_str(), newSize);
			newBuckets->at(newIndex).addEntry(newEntry);

			current = current->next;
		}
	}	

	return std::shared_ptr<std::vector<HashBucket>>(newBuckets);
}

void CustomHashTable::Insert(const WordInfo& wordInfo) {
	snapshotMutex.lock_shared();
	auto snapshot = curSnapshot;
	snapshotMutex.unlock_shared();


	if(!snapshot.get()->empty() && contains(snapshot, wordInfo.word.c_str())) {
		size_t index = HashFunction(wordInfo.word.c_str(), snapshot.get()->size());
		std::unique_lock<std::shared_mutex> lock(snapshot->at(index).mtx);
		HashEntry* oldEntry = findEntry(*snapshot, wordInfo.word);
		oldEntry->addPosition(wordInfo);
		currentSize.fetch_add(1);
		return;
	}

	std::lock_guard<std::mutex> resizeLock(resizeMutex);
	auto reshapedTable = resize(snapshot);

	HashEntry* extraEntry = new HashEntry(wordInfo);
	size_t newIndex = HashFunction(wordInfo.word.c_str(), reshapedTable.get()->size());
	reshapedTable.get()->at(newIndex).addEntry(extraEntry);

	std::unique_lock<std::shared_mutex> lock(snapshotMutex);
	curSnapshot = reshapedTable;
	currentSize.fetch_add(1);
}

CustomHashTable::WordPositions CustomHashTable::Find(const char* word)
{
	snapshotMutex.lock_shared();
	auto snapshot = curSnapshot;
	snapshotMutex.unlock_shared();

	if(snapshot.get()->empty()) {
		return {};
	}

	size_t index = HashFunction(word, snapshot.get()->size());
	std::shared_lock<std::shared_mutex> lock(snapshot->at(index).mtx);
	HashEntry* entry = findEntry(*snapshot, word);
	if (entry) {
		return entry->positions;
	}
	return {};
}

