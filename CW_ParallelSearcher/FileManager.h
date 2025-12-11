#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <mutex>
#include <map>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

#define STORAGE_DIR "storage"

class FileManager
{
private:
    static uint64_t currentFileId;
    static std::mutex fileSaveMutex;

    static std::map<uint64_t, std::string> fileIndexMap; // maps file ID to file path

    static std::string getTodayFolder();

public:
    FileManager() = delete;
    static uint64_t SaveFile(const std::string& fileName, const std::string& fileData);
    static std::string getFileText(uint64_t fileId);
    static std::string getFileName(uint64_t fileId);
    static void Initialize(const std::string& storageDir = STORAGE_DIR);
    static std::string GetFilePart(uint64_t fileId, size_t partIndex, size_t partSize);
	static std::vector<uint64_t> GetAllFileIds();


};