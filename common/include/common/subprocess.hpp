#pragma once

#include <string>
#include <optional>
#include <stdexcept>

namespace subprocess {
    class Subprocess {
    public:
        Subprocess() noexcept = default;
        explicit Subprocess(const std::string& file_path);
        ~Subprocess() noexcept;

        Subprocess(const Subprocess&) = delete;
        Subprocess& operator=(const Subprocess&) = delete;
        Subprocess(Subprocess&& other) noexcept;
        Subprocess& operator=(Subprocess&& other) noexcept;

        std::optional<std::string> read() const;
        void write(const std::string& data) const;
        void wait();
        void terminate();
        bool active() const noexcept;
    private:
        int input {-1};  // Read from
        int output {-1};  // Write to
        int child_pid {-1};

        mutable std::string buffered;
    };

    struct Error : public std::runtime_error {
        explicit Error(const char* message)
            : std::runtime_error(message) {}
        explicit Error(const std::string& message)
            : std::runtime_error(message) {}
    };
}
