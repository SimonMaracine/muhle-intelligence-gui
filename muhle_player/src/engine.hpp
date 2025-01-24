#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include <optional>
#include <functional>
#include <variant>
#include <fstream>

#include "subprocess.hpp"

namespace engine {
    class Engine {
    public:
        struct Info {
            struct ScoreEval { int value; };
            struct ScoreWin { int value; };

            using Score = std::variant<ScoreEval, ScoreWin>;

            std::optional<unsigned int> depth;
            std::optional<unsigned int> time;
            std::optional<unsigned int> nodes;
            std::optional<Score> score;
            std::optional<std::vector<std::string>> pv;
        };

        struct Option {
            struct Check {
                bool default_;
            };

            struct Spin {
                int default_;
                int min;
                int max;
            };

            struct Combo {
                std::string default_;
                std::vector<std::string> vars;
            };

            struct Button {};

            struct String {
                std::string default_;
            };

            using Value = std::variant<Check, Spin, Combo, Button, String>;

            std::string name;
            Value value;
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
            std::optional<unsigned int> depth,
            std::optional<unsigned int> movetime
        );
        void stop_thinking();
        std::optional<std::string> done_thinking();
        void uninitialize();

        void set_info_callback(std::function<void(const Info&)>&& info_callback);
        void set_log_output(bool enable);
        const std::string& get_name() const { return m_name; }
        const std::string& get_author() const { return m_author; }
        const std::vector<Option>& get_options() const { return m_options; }
    private:
        static std::vector<std::string> parse_message(const std::string& message);
        static std::optional<Option> parse_option(const std::vector<std::string>& tokens);
        static std::optional<std::string> parse_option_name(const std::vector<std::string>& tokens);
        static std::optional<std::string> parse_option_type(const std::vector<std::string>& tokens);
        static std::optional<std::string> parse_option_default(const std::vector<std::string>& tokens);
        static std::optional<int> parse_option_min(const std::vector<std::string>& tokens);
        static std::optional<int> parse_option_max(const std::vector<std::string>& tokens);
        static std::optional<std::vector<std::string>> parse_option_vars(const std::vector<std::string>& tokens);
        static Info parse_info(const std::vector<std::string>& tokens);
        static std::optional<unsigned int> parse_info_ui(const std::vector<std::string>& tokens, const std::string& name);
        static std::optional<Info::Score> parse_info_score(const std::vector<std::string>& tokens);
        static std::optional<std::vector<std::string>> parse_info_pv(const std::vector<std::string>& tokens);
        static bool token_available(const std::vector<std::string>& tokens, std::size_t index);

        subprocess::Subprocess m_subprocess;
        std::function<void(const Info&)> m_info_callback;
        std::ofstream m_log_output_stream;
        std::string m_name;
        std::string m_author;
        std::vector<Option> m_options;
    };

    struct EngineError : std::runtime_error {
        explicit EngineError(const char* message)
            : std::runtime_error(message) {}
        explicit EngineError(const std::string& message)
            : std::runtime_error(message) {}
    };
}
