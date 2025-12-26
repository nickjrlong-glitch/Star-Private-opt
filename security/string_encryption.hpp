#pragma once
#include <string>
#include <array>
#include <random>
#include <type_traits>

namespace security {

// Compile-time string encryption
template<typename T, size_t S>
class encrypted_string {
    std::array<T, S> _data;
    const char* _key;
    
    constexpr T encrypt_char(T c, size_t i) const {
        return c ^ (_key[i % 16] + i);
    }
    
    constexpr T decrypt_char(T c, size_t i) const {
        return c ^ (_key[i % 16] + i);
    }

public:
    template<size_t... Is>
    constexpr encrypted_string(const T* str, const char* key, std::index_sequence<Is...>) 
        : _data{encrypt_char(str[Is], Is)...}, _key(key) {}
    
    constexpr const T* decrypt() const {
        static T decrypted[S] = {};
        static bool is_decrypted = false;
        
        if (!is_decrypted) {
            for (size_t i = 0; i < S - 1; ++i) {
                decrypted[i] = decrypt_char(_data[i], i);
            }
            decrypted[S-1] = '\0';
            is_decrypted = true;
        }
        return decrypted;
    }
};

// Helper to create encrypted strings
template<typename T, size_t S>
constexpr auto make_encrypted_string(const T (&str)[S], const char* key) {
    return encrypted_string<T, S>(str, key, std::make_index_sequence<S>{});
}

// Anti-debug string obfuscation
class string_obfuscator {
public:
    static constexpr size_t KEY_LENGTH = 32;
    static inline std::array<uint8_t, KEY_LENGTH> runtime_key = {};
    
    static void initialize_key() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (auto& k : runtime_key) {
            k = static_cast<uint8_t>(dis(gen));
        }
    }

    static std::string obfuscate(const std::string& input) {
        if (runtime_key[0] == 0) initialize_key();
        
        std::string result = input;
        for (size_t i = 0; i < result.length(); ++i) {
            result[i] ^= runtime_key[i % KEY_LENGTH] + static_cast<uint8_t>(i);
        }
        return result;
    }
    
    static std::string deobfuscate(const std::string& input) {
        return obfuscate(input); // XOR is reversible
    }
};

// Macro for easy string encryption
#define ENCRYPT_STR(str) (security::make_encrypted_string(str, __TIME__ __DATE__).decrypt())
#define OBFUSCATE(str) (security::string_obfuscator::obfuscate(str))
#define DEOBFUSCATE(str) (security::string_obfuscator::deobfuscate(str))

} // namespace security 