#pragma once
#include <string>
#include <unordered_map>
#include <sstream>

class Response {
public:
    enum class Type {
        Ok = 200,
        BadRequest = 400,
        NotFound = 404,
        InternalError = 500
    };

private:
    Type type;
    std::string body;
    std::string contentType = "text/plain";

    static const std::unordered_map<Type, std::string>& statusText() {
        static const std::unordered_map<Type, std::string> map = {
            { Type::Ok,            "OK" },
            { Type::BadRequest,    "Bad Request" },
            { Type::NotFound,      "Not Found" },
            { Type::InternalError, "Internal Server Error" }
        };
        return map;
    }

public:
    Response(Type type, std::string body, std::string contentType = "text/plain");

    // ---- FACTORY METHODS ----

    static Response Ok(const std::string& body = "");

    static Response BadRequest(const std::string& msg = "Bad Request");

    static Response NotFound(const std::string& msg = "Not Found");

    static Response InternalServerError(const std::string& msg = "Internal Server Error");

    // ---- GETTERS ----

    Type getType() const noexcept { return type; }
    const std::string& getBody() const noexcept { return body; }

    // ---- BUILD FULL HTTP RESPONSE ----
    std::string toHttpString() const;
};