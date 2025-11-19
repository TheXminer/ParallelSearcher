#pragma once
#include <vector>
#include <shared_mutex>
#define RESIZE_FACTOR 2
#define LOAD_FACTOR 0.7
#define START_SIZE 100

class CustomHashTable
{
private:
	struct HashBucket;
	struct HashEntry;
	struct WordPosition;

public:
	CustomHashTable();
	~CustomHashTable();

	bool Insert(const char* word, size_t position, const char* fileName);
	std::vector<std::pair<char*, size_t>> Find(const char* word);

private:
	size_t HashFunction(const char* word);
	void resize();
	std::vector<HashBucket> buckets;
};

