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
            m_subprocess = subprocess::Subprocess(file_path);
        } catch (const subprocess::Error& e) {
            throw EngineError("Could not start subprocess: "s + e.what());
        }

        try {
            m_subprocess.write("gbgp\n");
        } catch (const subprocess::Error& e) {
            try_terminate();
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        const auto begin {std::chrono::steady_clock::now()};

        while (true) {
            const auto now {std::chrono::steady_clock::now()};

            if (now - begin > 5s) {
                try_terminate();
                throw EngineError("Engine did not respond in a timely manner");
            }

            std::optional<std::string> message;

            try {
                message = m_subprocess.read();
            } catch (const subprocess::Error& e) {
                try_terminate();
                throw EngineError("Could not read from subprocess: "s + e.what());
            }

            if (!message) {
                continue;
            }

            const auto tokens {parse_message(message->substr(0, message->size() - 1))};

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
            m_subprocess.write("debug"s + (active ? " on" : " off") + '\n');
        } catch (const subprocess::Error& e) {
            try_terminate();
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::synchronize() {
        try {
            m_subprocess.write("isready\n");
        } catch (const subprocess::Error& e) {
            try_terminate();
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        const auto begin {std::chrono::steady_clock::now()};

        while (true) {
            const auto now {std::chrono::steady_clock::now()};

            if (now - begin > 5s) {
                try_terminate();
                throw EngineError("Engine did not respond in a timely manner");
            }

            std::optional<std::string> message;

            try {
                message = m_subprocess.read();
            } catch (const subprocess::Error& e) {
                try_terminate();
                throw EngineError("Could not read from subprocess: "s + e.what());
            }

            if (!message) {
                continue;
            }

            const auto tokens {parse_message(message->substr(0, message->size() - 1))};

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
            m_subprocess.write("setoption name " + name + (value ? " value " + *value : "") + '\n');
        } catch (const subprocess::Error& e) {
            try_terminate();
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::new_game() {
        try {
            m_subprocess.write("newgame\n");
        } catch (const subprocess::Error& e) {
            try_terminate();
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
                " moves " + std::accumulate(std::next(moves.cbegin()), moves.cend(), *moves.cbegin(), [](std::string r, const std::string& move) {
                    return std::move(r) + " " + move;
                })
                :
                ""
            };

            m_subprocess.write("position" + (position ? " pos " + *position : " startpos") + moves_str + '\n');
        } catch (const subprocess::Error& e) {
            try_terminate();
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        try {
            m_subprocess.write(
                "go"s +
                (wtime ? " wtime " + *wtime : "") +
                (btime ? " btime " + *btime : "") +
                (max_depth ? " maxdepth " + *max_depth : "") +
                (max_time ? " maxtime " + *max_time : "") +
                '\n'
            );
        } catch (const subprocess::Error& e) {
            try_terminate();
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::stop_thinking() {
        try {
            m_subprocess.write("stop\n");
        } catch (const subprocess::Error& e) {
            try_terminate();
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    std::optional<std::string> Engine::done_thinking() {
        std::optional<std::string> message;

        try {
            message = m_subprocess.read();
        } catch (const subprocess::Error& e) {
            try_terminate();
            throw EngineError("Could not read from subprocess: "s + e.what());
        }

        if (!message) {
            return std::nullopt;
        }

        const auto tokens {parse_message(message->substr(0, message->size() - 1))};

        if (tokens.empty()) {
            return std::nullopt;
        }

        if (tokens[0] == "bestmove") {
            if (token_available(tokens, 1)) {
                return std::make_optional(tokens[1]);
            }
        } else if (tokens[0] == "info") {
            if (m_info_callback) {
                m_info_callback(parse_info(tokens), m_info_callback_pointer);
            }
        }

        return std::nullopt;
    }

    void Engine::set_info_callback(std::function<void(const Info&, void*)>&& info_callback, void* info_callback_pointer) {
        m_info_callback = std::move(info_callback);
        m_info_callback_pointer = info_callback_pointer;
    }

    void Engine::uninitialize() {
        m_name.clear();

        try {
            m_subprocess.write("quit\n");
        } catch (const subprocess::Error& e) {
            try_terminate();
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        try {
            m_subprocess.wait();
        } catch (const subprocess::Error& e) {
            try_terminate();
            throw EngineError("Could not wait for subprocess: "s + e.what());
        }
    }

    void Engine::try_terminate() {
        try {
            m_subprocess.terminate();
        } catch (const subprocess::Error&) {}
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
        return {};  // TODO
    }

    bool Engine::token_available(const std::vector<std::string>& tokens, std::size_t index) {
        return index < tokens.size();
    }
}
