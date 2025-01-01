#pragma once

#include <string>
#include <stdexcept>
#include <vector>

#include "subprocess.hpp"

namespace engine {
    class Engine {
    public:
        void initialize(const std::string& file_path);
        void uninitialize();

        void debug(bool active);  // TODO come up with better names
        void isready();
        void setoption(const std::string& name, const std::optional<std::string>& value);
        void newgame();
        void position(const std::optional<std::string>& position, const std::vector<std::string>& moves);
        void go(unsigned int wtime, unsigned int btime, unsigned int max_depth, unsigned int max_time);
        void stop();

        const std::string& get_name() const { return m_name; }
    private:
        static std::vector<std::string> parse_message(const std::string& message);
        static bool token_available(const std::vector<std::string>& tokens, std::size_t index);

        subprocess::Subprocess m_subprocess;
        std::string m_name;
    };

    struct EngineError : std::runtime_error {
        explicit EngineError(const char* message)
            : std::runtime_error(message) {}
        explicit EngineError(const std::string& message)
            : std::runtime_error(message) {}
    };
}
