#pragma once
#include <vector>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <functional>
#include <optional>
#include <atomic>
#include <thread>
#include <random>
#include <iostream>
#include <chrono>
#include <cassert>


class ConcurrentHashMap
{
public:
    struct WordLocation {
        uint32_t wordPosition;
        uint32_t byteOffset;
        uint32_t fileID;
        bool operator<(const WordLocation& other) const {
            if (this->fileID == other.fileID)
                return this->wordPosition < other.wordPosition;
            return this->fileID < other.fileID;
        }
        WordLocation() : wordPosition(0), byteOffset(0), fileID(0) {}
        WordLocation(uint64_t fileID, size_t byteOff, size_t wordPos)
            : wordPosition(wordPos), byteOffset(byteOff), fileID(fileID) {
        }
    };

	using keyType = std::string;
	using mappedType = std::vector<WordLocation>;

    struct Shard {
        mutable std::shared_mutex mtx;
        std::unordered_map<std::string, std::vector<WordLocation>> map;
    };

    ConcurrentHashMap(const ConcurrentHashMap&) = delete;
    ConcurrentHashMap& operator=(const ConcurrentHashMap&) = delete;

    ~ConcurrentHashMap() = default;

    ConcurrentHashMap(size_t numShards = 32, size_t startMapSize = 10000) {
        _num_shards = numShards;
        _shards.reserve(_num_shards);
        for (size_t i = 0; i < _num_shards; ++i) {
            _shards.push_back(std::make_unique<Shard>());
            _shards.back()->map.reserve(startMapSize);
        }
	}

    void insert(const keyType& key, const WordLocation& value) {
        size_t shardIndex = hashFunction(key);
        Shard& shard = *_shards[shardIndex];
        {
            std::unique_lock<std::shared_mutex> lock(shard.mtx);
            shard.map[key].push_back(value);
        }
        _size.fetch_add(1, std::memory_order_relaxed);
    }
    mappedType find(const keyType& key) const {
        size_t shardIndex = hashFunction(key);
        const Shard& shard = *_shards[shardIndex];
        {
            std::shared_lock<std::shared_mutex> lock(shard.mtx);
            auto it = shard.map.find(key);
            if (it != shard.map.end()) {
                return it->second;
            }
        }
        return mappedType();
    }
    size_t size() const {
        return _size.load(std::memory_order_relaxed);
	}


private:

    size_t _num_shards;
    std::vector<std::unique_ptr<Shard>> _shards;
    std::atomic<size_t> _size;

    size_t hashFunction(const std::string& key) const {
        return std::hash<std::string>{}(key) % _num_shards;
	}

};
