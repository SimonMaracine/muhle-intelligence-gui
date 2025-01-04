#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include <optional>
#include <functional>

#include "subprocess.hpp"

namespace engine {
    class Engine {
    public:
        struct Info {

        };

        void initialize(const std::string& file_path);
        void set_debug(bool active);
        void synchronize();
        void set_option(const std::string& name, const std::optional<std::string>& value);
        void new_game();
        void start_thinking(
            const std::optional<std::string>& position,
            const std::vector<std::string>& moves,
            std::optional<unsigned int> wtime,
            std::optional<unsigned int> btime,
            std::optional<unsigned int> max_depth,
            std::optional<unsigned int> max_time
        );
        void stop_thinking();
        std::optional<std::string> done_thinking();
        void set_info_callback(std::function<void(const Info&, void*)>&& info_callback, void* info_callback_pointer);
        void uninitialize();

        bool active() const { return m_subprocess.active(); }
        const std::string& get_name() const { return m_name; }
    private:
        void try_terminate();
        static std::vector<std::string> parse_message(const std::string& message);
        static Info parse_info(const std::vector<std::string>& tokens);
        static bool token_available(const std::vector<std::string>& tokens, std::size_t index);

        subprocess::Subprocess m_subprocess;
        std::string m_name;
        std::function<void(const Info&, void*)> m_info_callback;
        void* m_info_callback_pointer {};
    };

    struct EngineError : std::runtime_error {
        explicit EngineError(const char* message)
            : std::runtime_error(message) {}
        explicit EngineError(const std::string& message)
            : std::runtime_error(message) {}
    };
}
