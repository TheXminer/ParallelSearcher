#include "CustomHashTable.h"


CustomHashTable::CustomHashTable() {
	curSnapshot = std::make_shared<std::vector<HashBucket>>();
}

CustomHashTable::~CustomHashTable() {
	std::unique_lock<std::shared_mutex> lock(snapshotMutex);
}

const bool CustomHashTable::contains(const Snapshot& snapshot, const char* word)
{
	size_t sz = snapshot->size();
	if (sz == 0) return false;
	size_t index = HashFunction(word, sz);
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
	if (size == 0) return 0;
	size_t hash = 5381;
	int c;
	const unsigned char* uword = reinterpret_cast<const unsigned char*>(word);
	while ((c = *uword++)) {
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
	size_t curSize = snapshot->size();
	curSize = (curSize == 0) ? START_SIZE : curSize;
	auto loadFactor = static_cast<double>(currentSize.load(std::memory_order_acquire)) / curSize;
	size_t newSize = (loadFactor > LOAD_FACTOR) ? curSize * RESIZE_FACTOR : curSize;

	if (newSize == curSize && snapshot->size() != 0) {
		return snapshot;
	}

	auto newBuckets = std::make_shared<SnapshotTable>(newSize);

	for (size_t i = 0; i < snapshot->size(); ++i) {
		std::unique_lock<std::shared_mutex> bucketLock(snapshot->at(i).mtx);
		HashEntry* current = snapshot->at(i).head;
		while (current) {
			HashEntry* newEntry = new HashEntry(*current->positions, current->word);
			size_t newIndex = HashFunction(current->word.c_str(), newSize);
			newBuckets->at(newIndex).addEntry(newEntry);
			current = current->next;
		}
	}

	return newBuckets;
}

void CustomHashTable::Insert(const WordInfo& wordInfo) {
    // Fast-path: check existence under shared snapshot lock
    {
        std::shared_lock<std::shared_mutex> lock(snapshotMutex);
        auto snapshot = curSnapshot;
        if (!snapshot->empty()) {
            size_t idx = HashFunction(wordInfo.word.c_str(), snapshot->size());
            // try to update existing entry
            std::unique_lock<std::shared_mutex> bucketLock(snapshot->at(idx).mtx);
            HashEntry* existing = findEntry(*snapshot, wordInfo.word);
            if (existing) {
                existing->addPosition(wordInfo);
                return;
            }
        }
    }

    // Need to insert new word (may require resize). Use resizeMutex to serialize new-word insertions/resizes.
    std::lock_guard<std::mutex> guard(resizeMutex);

    // Double-check after acquiring resize lock (the snapshot might have changed)
    {
        std::shared_lock<std::shared_mutex> lock(snapshotMutex);
        auto snapshot = curSnapshot;
        if (!snapshot->empty()) {
            size_t idx = HashFunction(wordInfo.word.c_str(), snapshot->size());
            std::unique_lock<std::shared_mutex> bucketLock(snapshot->at(idx).mtx);
            HashEntry* existing = findEntry(*snapshot, wordInfo.word);
            if (existing) {
                existing->addPosition(wordInfo);
                return;
            }
        }
    }

    // Perform resize (if needed) based on current curSnapshot
    Snapshot baseSnapshot;
    {
        std::shared_lock<std::shared_mutex> lock(snapshotMutex);
        baseSnapshot = curSnapshot; // snapshot to copy from
    }

    Snapshot reshapedTable = resize(baseSnapshot);

    // Insert new entry into reshapedTable
    HashEntry* extraEntry = new HashEntry(wordInfo);
    size_t newIndex = HashFunction(wordInfo.word.c_str(), reshapedTable->size());
    // lock the target bucket in the new table (not strictly necessary since no other thread sees reshapedTable yet)
    reshapedTable->at(newIndex).addEntry(extraEntry);

    // Swap in new snapshot atomically under exclusive snapshotMutex
    {
        std::unique_lock<std::shared_mutex> lock(snapshotMutex);
        curSnapshot = reshapedTable;
    }

    currentSize.fetch_add(1, std::memory_order_release);
}

CustomHashTable::WordPositions CustomHashTable::Find(const char* word)
{
    std::shared_lock<std::shared_mutex> lock(snapshotMutex);
    auto snapshot = curSnapshot;
    if (snapshot.get()->empty()) {
        return {};
    }

    size_t index = HashFunction(word, snapshot.get()->size());
    std::shared_lock<std::shared_mutex> bucketLock(snapshot->at(index).mtx);
    HashEntry* entry = findEntry(*snapshot, word);
    if (entry) {
        // return a copy of positions so caller can use it without holding locks
        return *entry->positions;
    }
    return {};
}