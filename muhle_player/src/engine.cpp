#include "engine.hpp"

#include <chrono>
#include <algorithm>
#include <numeric>
#include <utility>
#include <cstring>

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace engine {
    void Engine::initialize(const std::string& file_path) {
        try {
            m_subprocess.open(file_path);
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not start subprocess: "s + e.what());
        }

        try {
            m_subprocess.write_line("gbgp");
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        const auto begin {std::chrono::steady_clock::now()};

        while (true) {
            const auto now {std::chrono::steady_clock::now()};

            if (now - begin > 5s) {
                throw EngineError("Engine did not respond in a timely manner");
            }

            std::string message;

            try {
                message = m_subprocess.read_line();
            } catch (const subprocess::SubprocessError& e) {
                throw EngineError("Could not read from subprocess: "s + e.what());
            }

            if (message.empty()) {
                continue;
            }

            if (m_log_output_stream.is_open()) {
                m_log_output_stream << message << '\n';
                m_log_output_stream.flush();
            }

            const auto tokens {parse_message(message)};

            if (tokens.empty()) {
                continue;
            }

            if (tokens[0] == "gbgpok") {
                break;
            } else if (tokens[0] == "id") {
                if (token_available(tokens, 1)) {
                    if (tokens[1] == "name") {
                        std::size_t index {2};
                        while (token_available(tokens, index)) {
                            m_name += ' ' + tokens[index++];
                        }
                        m_name = m_name.substr(1);
                    } else if (tokens[1] == "author") {
                        std::size_t index {2};
                        while (token_available(tokens, index)) {
                            m_author += ' ' + tokens[index++];
                        }
                        m_author = m_author.substr(1);
                    }
                }
            } else if (tokens[0] == "option") {
                const auto option {parse_option(tokens)};

                if (option) {
                    m_options.push_back(*option);
                }
            }
        }
    }

    void Engine::set_debug(bool active) {
        try {
            m_subprocess.write_line("debug"s + (active ? " on" : " off"));
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::synchronize() {
        try {
            m_subprocess.write_line("isready");
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        const auto begin {std::chrono::steady_clock::now()};

        while (true) {
            const auto now {std::chrono::steady_clock::now()};

            if (now - begin > 5s) {
                throw EngineError("Engine did not respond in a timely manner");
            }

            std::string message;

            try {
                message = m_subprocess.read_line();
            } catch (const subprocess::SubprocessError& e) {
                throw EngineError("Could not read from subprocess: "s + e.what());
            }

            if (message.empty()) {
                continue;
            }

            if (m_log_output_stream.is_open()) {
                m_log_output_stream << message << '\n';
                m_log_output_stream.flush();
            }

            const auto tokens {parse_message(message)};

            if (tokens.empty()) {
                continue;
            }

            if (tokens[0] == "readyok") {
                break;
            }
        }
    }

    void Engine::set_option(const std::string& name, const std::optional<std::string>& value) {
        try {
            m_subprocess.write_line("setoption name " + name + (value ? " value " + *value : ""));
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::new_game() {
        try {
            m_subprocess.write_line("newgame");
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::start_thinking(
        const std::optional<std::string>& position,
        const std::vector<std::string>& moves,
        std::optional<unsigned int> wtime,
        std::optional<unsigned int> btime,
        std::optional<unsigned int> depth,
        std::optional<unsigned int> movetime
    ) {
        const auto moves_str {
            !moves.empty()
            ?
            " moves " + std::accumulate(++moves.cbegin(), moves.cend(), *moves.cbegin(), [](std::string r, const std::string& move) {
                return std::move(r) + " " + move;
            })
            :
            ""
        };

        try {
            m_subprocess.write_line("position" + (position ? " pos " + *position : " startpos") + moves_str);
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        try {
            m_subprocess.write_line(
                "go"s +
                (wtime ? " wtime " + std::to_string(*wtime) : "") +
                (btime ? " btime " + std::to_string(*btime) : "") +
                (depth ? " depth " + std::to_string(*depth) : "") +
                (movetime ? " movetime " + std::to_string(*movetime) : "")
            );
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::stop_thinking() {
        try {
            m_subprocess.write_line("stop");
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    std::optional<std::string> Engine::done_thinking() {
        std::string message;

        try {
            message = m_subprocess.read_line();
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not read from subprocess: "s + e.what());
        }

        if (message.empty()) {
            return std::nullopt;
        }

        if (m_log_output_stream.is_open()) {
            m_log_output_stream << message << '\n';
            m_log_output_stream.flush();
        }

        const auto tokens {parse_message(message)};

        if (tokens.empty()) {
            return std::nullopt;
        }

        if (tokens[0] == "bestmove") {
            if (token_available(tokens, 1)) {
                return tokens[1];
            }
        } else if (tokens[0] == "info") {
            if (m_info_callback) {
                m_info_callback(parse_info(tokens));
            }
        }

        return std::nullopt;
    }

    void Engine::uninitialize() {
        m_name.clear();

        try {
            m_subprocess.write_line("quit");
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        try {
            m_subprocess.wait();
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not wait for subprocess: "s + e.what());
        }
    }

    void Engine::set_info_callback(std::function<void(const Info&)>&& info_callback) {
        m_info_callback = std::move(info_callback);
    }

    void Engine::set_log_output(bool enable) {
        if (enable) {
            m_log_output_stream.open("muhle_player.log", std::ios::app);
        } else {
            m_log_output_stream.close();
        }
    }

    std::vector<std::string> Engine::parse_message(const std::string& message) {
        std::vector<std::string> tokens;
        std::string buffer {message};

        char* token {std::strtok(buffer.data(), " \t")};

        while (token != nullptr) {
            tokens.emplace_back(token);
            token = std::strtok(nullptr, " \t");
        }

        tokens.erase(std::remove_if(tokens.begin(), tokens.end(), [](const std::string& token) {
            return token.empty();
        }), tokens.end());

        return tokens;
    }

    std::optional<Engine::Option> Engine::parse_option(const std::vector<std::string>& tokens) {
        Option option;

        const auto name {parse_option_name(tokens)};

        if (!name) {
            return std::nullopt;
        }

        option.name = *name;

        const auto default_ {parse_option_default(tokens)};

        if (!default_) {
            return std::nullopt;
        }

        const auto type {parse_option_type(tokens)};

        if (!type) {
            return std::nullopt;
        }

        if (*type == "check") {
            Option::Check value;

            if (*default_ == "true") {
                value.default_ = true;
            } else if (*default_ == "false") {
                value.default_ = false;
            } else {
                return std::nullopt;
            }

            option.value = value;
        } else if (*type == "spin") {
            Option::Spin value;

            try {
                value.default_ = static_cast<int>(std::stol(*default_));
            } catch (...) {
                return std::nullopt;
            }

            const auto min {parse_option_min(tokens)};

            if (min) {
                value.min = *min;
            }

            const auto max {parse_option_min(tokens)};

            if (max) {
                value.max = *max;
            }

            option.value = value;
        } else if (*type == "combo") {
            Option::Combo value;

            value.default_ = *default_;

            const auto vars {parse_option_vars(tokens)};

            if (vars) {
                value.vars = *vars;
            }

            option.value = value;
        } else if (*type == "string") {
            Option::String value;

            value.default_ = *default_;

            option.value = value;
        } else if (*type == "button") {
            Option::Button value;

            option.value = value;
        }

        return option;
    }

    std::optional<std::string> Engine::parse_option_name(const std::vector<std::string>& tokens) {
        auto iter {std::find(tokens.cbegin(), tokens.cend(), "name")};

        if (iter == tokens.cend()) {
            return std::nullopt;
        }

        std::string name;

        while (++iter != tokens.cend()) {
            if (*iter == "type") {
                break;
            }

            name += ' ' + *iter;
        }

        if (name.empty()) {
            return std::nullopt;
        }

        return name.substr(1);
    }

    std::optional<std::string> Engine::parse_option_type(const std::vector<std::string>& tokens) {
        auto iter {std::find(tokens.cbegin(), tokens.cend(), "type")};

        if (iter == tokens.cend()) {
            return std::nullopt;
        }

        if (++iter != tokens.cend()) {
            return *iter;
        }

        return std::nullopt;
    }

    std::optional<std::string> Engine::parse_option_default(const std::vector<std::string>& tokens) {
        auto iter {std::find(tokens.cbegin(), tokens.cend(), "default")};

        if (iter == tokens.cend()) {
            return std::nullopt;
        }

        std::string default_;

        while (++iter != tokens.cend()) {
            if (*iter == "min" || *iter == "max" || *iter == "var") {
                break;
            }

            default_ += ' ' + *iter;
        }

        if (default_.empty()) {
            return std::nullopt;
        }

        return default_.substr(1);
    }

    std::optional<int> Engine::parse_option_min(const std::vector<std::string>& tokens) {
        auto iter {std::find(tokens.cbegin(), tokens.cend(), "min")};

        if (iter == tokens.cend()) {
            return std::nullopt;
        }

        if (++iter != tokens.cend()) {
            try {
                return std::stol(*iter);
            } catch (...) {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::optional<int> Engine::parse_option_max(const std::vector<std::string>& tokens) {
        auto iter {std::find(tokens.cbegin(), tokens.cend(), "max")};

        if (iter == tokens.cend()) {
            return std::nullopt;
        }

        if (++iter != tokens.cend()) {
            try {
                return std::stol(*iter);
            } catch (...) {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::optional<std::vector<std::string>> Engine::parse_option_vars(const std::vector<std::string>& tokens) {
        std::vector<std::string>::const_iterator iter {tokens.cbegin()};
        std::vector<std::string> vars;

        while (true) {
            iter = std::find(iter, tokens.cend(), "var");

            if (iter == tokens.cend()) {
                break;
            }

            std::string var;

            while (++iter != tokens.cend()) {
                var += ' ' + *iter;
            }

            if (var.empty()) {
                break;
            }

            vars.push_back(var.substr(1));
        }

        if (vars.empty()) {
            return std::nullopt;
        }

        return vars;
    }

    Engine::Info Engine::parse_info(const std::vector<std::string>& tokens) {
        Info info;
        info.depth = parse_info_ui(tokens, "depth");
        info.time = parse_info_ui(tokens, "time");
        info.nodes = parse_info_ui(tokens, "nodes");
        info.score = parse_info_score(tokens);
        info.pv = parse_info_pv(tokens);

        return info;
    }

    std::optional<unsigned int> Engine::parse_info_ui(const std::vector<std::string>& tokens, const std::string& name) {
        auto iter {std::find(tokens.cbegin(), tokens.cend(), name)};

        if (iter == tokens.cend()) {
            return std::nullopt;
        }

        if (++iter != tokens.cend()) {
            try {
                return std::stoul(*iter);
            } catch (...) {
                return std::nullopt;
            }
        }

        return std::nullopt;
    }

    std::optional<Engine::Info::Score> Engine::parse_info_score(const std::vector<std::string>& tokens) {
        auto iter {std::find(tokens.cbegin(), tokens.cend(), "score")};

        if (iter == tokens.cend()) {
            return std::nullopt;
        }

        if (++iter == tokens.cend()) {
            return std::nullopt;
        }

        if (*iter == "eval") {
            if (++iter != tokens.cend()) {
                try {
                    return Info::ScoreEval {std::stoi(*iter)};
                } catch (...) {
                    return std::nullopt;
                }
            }
        } else if (*iter == "win") {
            if (++iter != tokens.cend()) {
                try {
                    return Info::ScoreWin {std::stoi(*iter)};
                } catch (...) {
                    return std::nullopt;
                }
            }
        }

        return std::nullopt;
    }

    std::optional<std::vector<std::string>> Engine::parse_info_pv(const std::vector<std::string>& tokens) {
        auto iter {std::find(tokens.cbegin(), tokens.cend(), "pv")};

        if (iter == tokens.cend()) {
            return std::nullopt;
        }

        std::vector<std::string> pv;

        while (++iter != tokens.cend()) {
            pv.push_back(*iter);
        }

        return pv;
    }

    bool Engine::token_available(const std::vector<std::string>& tokens, std::size_t index) {
        return index < tokens.size();
    }
}
