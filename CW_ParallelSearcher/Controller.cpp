#include "Controller.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

Controller::Controller(std::shared_ptr<ThreadPool> threadPool)
	: threadPool(threadPool), searcher(this->threadPool) {
	routeHandlers["POST /addfile"] =
		[this](const std::string& req) {
		return this->handleAddFile(req);
		};

	routeHandlers["GET /search"] =
		[this](const std::string& req) {
		return this->handleSearchPhrase(req);
		};

	routeHandlers["GET /file"] =
		[this](const std::string& req) {
		return this->handleGetFile(req);
		};

	auto optionRoutes = std::vector<std::string>();
	for (const auto& route : routeHandlers) {
		optionRoutes.push_back(route.first.substr(route.first.find(' ') + 1));
	}

	auto optionHandler =
		[this](const std::string& req) {
		return this->handleOptions(req);
		};
	for(const auto& route : optionRoutes) {
		routeHandlers["OPTIONS " + route] = optionHandler;
	}
}

Response Controller::handleAddFile(const std::string& request)
{
	std::string fileName = getParamFromBody(request, "fileName");
	if(fileName.empty()) {
		return Response::BadRequest("Missing 'fileName' parameter");
	}

	std::string fileData = getParamFromBody(request, "content");
	if(fileData.empty()) {
		return Response::BadRequest("Missing 'content' parameter");
	}

	uint32_t fileID = FileManager::SaveFile(fileName, fileData);
	if(fileID == 0) {
		return Response::InternalServerError("Failed to save file");
	}
	searcher.AddFile(fileID);
	return Response::Ok("File will be added soon!");
}

Response Controller::handleOptions(const std::string& request)
{
	return Response::Ok();
}

Response Controller::handleSearchPhrase(const std::string& request)
{
	auto phrase = getParam(request, "phrase");
	if (phrase.empty()) {
		return Response::BadRequest("Missing 'phrase' parameter");
	}

	auto results = searcher.SearchPhrase(phrase);
	return Response::Ok(JSONifySearchResults(results));
}

Response Controller::handleGetFile(const std::string& request)
{
	uint32_t fileId;
	try{
		fileId = std::stoi(getParam(request, "id"));
	}
	catch(...)
	{
		return Response::BadRequest("Invalid 'id' parameter");
	}

	auto fileContent = FileManager::getFileText(fileId);
	return Response::Ok(fileContent);
}

std::string Controller::urlDecode(const std::string& str) {
	std::string ret;
	char ch;
	int i, ii;
	for (i = 0; i < str.length(); i++) {
		if (str[i] == '%') {
			if (i + 2 < str.length()) {
				std::istringstream iss(str.substr(i + 1, 2));
				iss >> std::hex >> ii;
				ch = static_cast<char>(ii);
				ret += ch;
				i += 2;
			}
		}
		else if (str[i] == '+') {
			ret += ' ';
		}
		else {
			ret += str[i];
		}
	}
	return ret;
}

void Controller::handleClient(int clientSocket)
{
	std::string request = getRequest(clientSocket);
	if (request.empty()) {
		sendResponse(clientSocket, Response::BadRequest("Empty request"));
		return;
	}

	std::string path = getRequestInfo(request);
	if(path.empty()) {
		sendResponse(clientSocket, Response::BadRequest("Incorrect path"));
		return;
	}
	
	auto handlerIt = routeHandlers.find(path);
	if (handlerIt != routeHandlers.end()) {
		Response response = handlerIt->second(request);
		sendResponse(clientSocket, response);
	} else {
		sendResponse(clientSocket, Response::BadRequest("Path not found"));
	}
}

std::string Controller::getRequest(int clientSocket)
{
	std::string request;
	char buffer[4096];

	while (true) {
		size_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

		if (bytesReceived <= 0) {
			return std::string();
		}

		request.append(buffer, bytesReceived);

		if (request.find("\r\n\r\n") != std::string::npos) {
			break;
		}
	}

	return request;
}

void Controller::sendResponse(int clientSocket, Response response)
{
	std::string responseString = response.toHttpString();
	send(clientSocket, responseString.c_str(), responseString.size(), 0);
}

std::string Controller::getRequestInfo(const std::string& req)
{
	auto first_space_pos = req.find(' ');
	if (first_space_pos == std::string::npos)
		return std::string();
	auto second_space_pos = req.find(' ', first_space_pos + 1);
	auto second_delim_pos = req.find('?', first_space_pos + 1);
	if(second_delim_pos != std::string::npos && second_delim_pos < second_space_pos)
	{
		second_space_pos = second_delim_pos;
	}

	std::string path = req.substr(0, second_space_pos);
	return path;
}

std::string Controller::JSONifySearchResults(const std::vector<Searcher::SearchResult>& results)
{
	std::string json = "{ \"results\": [";
	for (size_t i = 0; i < results.size(); ++i) {
		json += results[i].toJSON();
		if (i < results.size() - 1) {
			json += ", ";
		}
	}
	json += "] }";
	return json;
}

std::string Controller::getParam(const std::string& req, const std::string& key)
{
	auto qpos = req.find('?');
	if (qpos == std::string::npos) return "";

	auto endPath = req.find(' ', qpos);
	std::string query = req.substr(qpos + 1, endPath - (qpos + 1));

	auto pos = query.find(key + "=");
	if (pos == std::string::npos) return "";

	pos += key.size() + 1;
	auto amp = query.find('&', pos);

	return urlDecode(query.substr(pos, amp - pos));
}

std::string Controller::getParamFromBody(const std::string& req, const std::string& key)
{
	std::string lowerKey = std::string(key);
	std::transform(key.begin(), key.end(), lowerKey.begin(),
		[](unsigned char c) { return std::tolower(c); });


	auto bodyPos = req.find("\r\n\r\n");
	if (bodyPos == std::string::npos) return "";
	std::string body = req.substr(bodyPos + 4);
	auto pos = body.find(lowerKey + "\"");
	if (pos == std::string::npos) return "";
	pos = body.find("\"", pos) + 1;
	auto start = body.find("\"", pos) + 1;
	auto amp = body.find("\"", start);
	return urlDecode(body.substr(start, amp - start));
}
