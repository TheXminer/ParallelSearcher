#include "FileManager.h"
#include <iostream>

uint64_t FileManager::currentFileId = 0;
std::mutex FileManager::fileSaveMutex;
std::map<uint64_t, std::string> FileManager::fileIndexMap;

std::string FileManager::getTodayFolder()
{
	std::time_t t = std::time(nullptr);
    std::tm tm_struct;
    localtime_s(&tm_struct, &t);

    std::stringstream ss;
    ss << std::put_time(&tm_struct, "%Y-%m-%d");
    return ss.str();
}

uint64_t FileManager::SaveFile(const std::string& fileName, const std::string& fileData)
{
    std::lock_guard<std::mutex> lock(fileSaveMutex);

    std::string todayFolder = std::string(STORAGE_DIR) + "/" + getTodayFolder() + "/";
    std::filesystem::create_directories(todayFolder);

    uint64_t fileId = ++currentFileId;

    std::string filePath = todayFolder + fileName;

    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile) {
        return 0;
    }
    outFile.write(fileData.c_str(), fileData.size());
    outFile.close();

    fileIndexMap[fileId] = filePath;
    return fileId;
}

std::string FileManager::getFileText(uint64_t fileId)
{
    std::string filePath = fileIndexMap[fileId];
    std::ifstream inFile(filePath, std::ios::binary);

    if (!inFile) {
        return "";
    }

    std::string fileContent(
        (std::istreambuf_iterator<char>(inFile)),
        std::istreambuf_iterator<char>()
    );

    return fileContent;
}

std::string FileManager::getFileName(uint64_t fileId)
{
    return fileIndexMap[fileId];
}

void FileManager::Initialize(const std::string& storageDir)
{
    std::lock_guard<std::mutex> lock(fileSaveMutex);

    currentFileId = 0;
    fileIndexMap.clear();

    // iterate through storage/YYYY-MM-DD/
    for (const auto& dirEntry : std::filesystem::directory_iterator(storageDir))
    {
        if (!dirEntry.is_directory())
            continue;

        for (const auto& fileEntry : std::filesystem::directory_iterator(dirEntry.path()))
        {
            std::string filePath = fileEntry.path().string();
            std::string filename = fileEntry.path().filename().string();

			++currentFileId;
            fileIndexMap[currentFileId] = filePath;
        }
    }
	std::cout << "FileManager initialized. Current file ID: " << currentFileId << std::endl;
}

std::string FileManager::GetFilePart(uint64_t fileId, size_t partIndex, size_t partSize)
{
    std::string filePath = fileIndexMap[fileId];
    std::ifstream inFile(filePath, std::ios::binary);

    if (!inFile)
        return "";

    inFile.seekg(partIndex, std::ios::beg);

    std::vector<char> buffer(partSize);
    inFile.read(buffer.data(), partSize);
    size_t bytesRead = inFile.gcount();

    return std::string(buffer.data(), bytesRead);
}

std::vector<uint64_t> FileManager::GetAllFileIds()
{
    std::vector<uint64_t> fileIds;
    for (const auto& entry : fileIndexMap) {
        fileIds.push_back(entry.first);
    }
	return fileIds;
}
