#pragma once

#include <array>
#include <cstddef>

// Returns true when the first character in the null-terminated buffer is the
// string terminator, indicating the array does not contain any text.
template <std::size_t N>
[[nodiscard]] constexpr bool CharArrayIsBlank(const std::array<char, N> &buffer) noexcept
{
        static_assert(N > 0, "character arrays used for text must have capacity");
        return buffer[0] == '\0';
}

// Convenience wrapper for readability when a caller needs to test for text
// content without manually negating CharArrayIsBlank.
template <std::size_t N>
[[nodiscard]] constexpr bool CharArrayHasText(const std::array<char, N> &buffer) noexcept
{
        return !CharArrayIsBlank(buffer);
}
