#include "jwt.h"

#include <openssl/hmac.h>
#include <openssl/evp.h>

#include <chrono>
#include <cctype>
#include <ctime>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <random>

namespace {

std::string base64Encode(const unsigned char* data, size_t len) {
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        uint32_t chunk = (data[i] << 16);
        if (i + 1 < len) chunk |= (data[i + 1] << 8);
        if (i + 2 < len) chunk |= data[i + 2];

        out.push_back(chars[(chunk >> 18) & 0x3F]);
        out.push_back(chars[(chunk >> 12) & 0x3F]);
        if (i + 1 < len) {
            out.push_back(chars[(chunk >> 6) & 0x3F]);
        } else {
            out.push_back('=');
        }
        if (i + 2 < len) {
            out.push_back(chars[chunk & 0x3F]);
        } else {
            out.push_back('=');
        }
    }
    return out;
}

std::string base64UrlEncode(const std::string& input) {
    std::string b64 = base64Encode(reinterpret_cast<const unsigned char*>(input.data()), input.size());
    for (char& c : b64) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!b64.empty() && b64.back() == '=') {
        b64.pop_back();
    }
    return b64;
}

std::string base64UrlEncode(const unsigned char* data, size_t len) {
    std::string b64 = base64Encode(data, len);
    for (char& c : b64) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!b64.empty() && b64.back() == '=') {
        b64.pop_back();
    }
    return b64;
}

std::string base64UrlDecode(const std::string& input) {
    std::string b64 = input;
    for (char& c : b64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    while (b64.size() % 4 != 0) {
        b64.push_back('=');
    }

    static const int table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,64,-1,-1,
        -1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };

    std::string out;
    out.reserve((b64.size() / 4) * 3);
    for (size_t i = 0; i + 3 < b64.size(); i += 4) {
        int vals[4];
        for (int j = 0; j < 4; ++j) {
            unsigned char c = static_cast<unsigned char>(b64[i + j]);
            vals[j] = c < 256 ? table[c] : -1;
        }
        if (vals[0] == -1 || vals[1] == -1) break;
        int v0 = vals[0];
        int v1 = vals[1];
        int v2 = vals[2];
        int v3 = vals[3];

        out.push_back(static_cast<char>((v0 << 2) | (v1 >> 4)));
        if (v2 != -1 && v2 != 64) {
            out.push_back(static_cast<char>(((v1 & 0xF) << 4) | (v2 >> 2)));
            if (v3 != -1 && v3 != 64) {
                out.push_back(static_cast<char>(((v2 & 0x3) << 6) | v3));
            }
        }
    }
    return out;
}

std::string hmacSha256(const std::string& data, const std::string& key) {
    unsigned int len = 0;
    unsigned char buffer[EVP_MAX_MD_SIZE];
    HMAC(EVP_sha256(),
         reinterpret_cast<const unsigned char*>(key.data()),
         static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()),
         data.size(),
         buffer,
         &len);
    return std::string(reinterpret_cast<char*>(buffer), len);
}

std::string randomId(const std::string& prefix) {
    static std::mt19937 rng{std::random_device{}()};
    static const char chars[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<> dist(0, static_cast<int>(sizeof(chars) - 2));
    std::string out = prefix;
    out.push_back('-');
    for (int i = 0; i < 12; ++i) {
        out.push_back(chars[dist(rng)]);
    }
    return out;
}

std::string extractField(const std::string& json, const std::string& key) {
    std::regex rgx("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    if (std::regex_search(json, match, rgx)) {
        return match[1];
    }
    return {};
}

long long extractNumber(const std::string& json, const std::string& key) {
    std::regex rgx("\"" + key + "\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    if (std::regex_search(json, match, rgx)) {
        return std::stoll(match[1]);
    }
    return 0;
}

} // namespace

JwtService::JwtService(std::string secret_, int expirySeconds_)
    : secret(std::move(secret_)), expirySeconds(expirySeconds_) {
    if (secret.empty()) {
        secret = randomId("secret");
    }
}

std::string JwtService::issueToken(const std::string& userId, const std::string& role) const {
    std::string header = R"({"alg":"HS256","typ":"JWT"})";
    auto now = std::chrono::system_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    long long exp = seconds + expirySeconds;

    std::ostringstream payload;
    payload << "{\"sub\":\"" << userId << "\","
            << "\"role\":\"" << role << "\","
            << "\"exp\":" << exp << "}";

    std::string headerB64 = base64UrlEncode(header);
    std::string payloadB64 = base64UrlEncode(payload.str());
    std::string signingInput = headerB64 + "." + payloadB64;
    std::string signature = hmacSha256(signingInput, secret);
    std::string signatureB64 = base64UrlEncode(reinterpret_cast<const unsigned char*>(signature.data()), signature.size());

    return signingInput + "." + signatureB64;
}

bool JwtService::verifyToken(const std::string& token, std::string& userId, std::string& role) const {
    auto firstDot = token.find('.');
    auto secondDot = token.find('.', firstDot == std::string::npos ? 0 : firstDot + 1);
    if (firstDot == std::string::npos || secondDot == std::string::npos) {
        return false;
    }

    std::string headerB64 = token.substr(0, firstDot);
    std::string payloadB64 = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string signatureB64 = token.substr(secondDot + 1);

    std::string signingInput = headerB64 + "." + payloadB64;
    std::string expectedSig = hmacSha256(signingInput, secret);
    std::string expectedSigB64 = base64UrlEncode(reinterpret_cast<const unsigned char*>(expectedSig.data()), expectedSig.size());

    if (expectedSigB64 != signatureB64) {
        return false;
    }

    std::string payloadJson = base64UrlDecode(payloadB64);
    if (payloadJson.empty()) {
        return false;
    }
    userId = extractField(payloadJson, "sub");
    role = extractField(payloadJson, "role");
    long long exp = extractNumber(payloadJson, "exp");

    auto nowSeconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (exp > 0 && nowSeconds > exp) {
        return false;
    }

    return !userId.empty();
}
