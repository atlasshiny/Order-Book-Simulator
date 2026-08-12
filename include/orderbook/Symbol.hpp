#pragma once
#include <array>
#include <string_view>
#include <functional>
#include <string>

namespace orderbook {

using Symbol = std::array<char, 8>;

inline Symbol make_symbol(const std::string& s) {
    Symbol out{};
    size_t n = std::min(s.size(), out.size() - 1);
    std::memcpy(out.data(), s.data(), n);
    return out;
}

struct SymbolHash {
    size_t operator()(const Symbol& s) const noexcept {
        // Ensure the string is null-terminated
        return std::hash<std::string_view>{}(std::string_view(s.data(), s.size()));
    }
};

} // namespace orderbook