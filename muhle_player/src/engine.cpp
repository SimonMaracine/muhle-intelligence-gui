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
                    }
                }
            } else if (tokens[0] == "option") {
                // TODO
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
        std::optional<unsigned int> max_depth,
        std::optional<unsigned int> max_time
    ) {
        try {
            const auto moves_str {
                !moves.empty()
                ?
                " moves " + std::accumulate(++moves.cbegin(), moves.cend(), *moves.cbegin(), [](std::string r, const std::string& move) {
                    return std::move(r) + " " + move;
                })
                :
                ""
            };

            m_subprocess.write_line("position" + (position ? " pos " + *position : " startpos") + moves_str);
        } catch (const subprocess::SubprocessError& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        try {
            m_subprocess.write_line(
                "go"s +
                (wtime ? " wtime " + std::to_string(*wtime) : "") +
                (btime ? " btime " + std::to_string(*btime) : "") +
                (max_depth ? " maxdepth " + std::to_string(*max_depth) : "") +
                (max_time ? " maxtime " + std::to_string(*max_time) : "")
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
                return std::make_optional(tokens[1]);
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
                return std::make_optional(std::stoul(*iter));
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
                    return std::make_optional(Info::ScoreEval {std::stoi(*iter)});
                } catch (...) {
                    return std::nullopt;
                }
            }
        } else if (*iter == "win") {
            if (++iter != tokens.cend()) {
                try {
                    return std::make_optional(Info::ScoreWin {std::stoi(*iter)});
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

        return std::make_optional(pv);
    }

    bool Engine::token_available(const std::vector<std::string>& tokens, std::size_t index) {
        return index < tokens.size();
    }
}
