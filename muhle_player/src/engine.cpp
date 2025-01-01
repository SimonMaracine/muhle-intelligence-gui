#include "engine.hpp"

#include <chrono>
#include <algorithm>
#include <numeric>
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
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        const auto begin {std::chrono::steady_clock::now()};

        while (true) {
            const auto now {std::chrono::steady_clock::now()};

            if (now - begin > 5s) {
                try {
                    m_subprocess.terminate();
                } catch (const subprocess::Error&) {}

                throw EngineError("Engine did not respond in a timely manner");
            }

            std::optional<std::string> message;

            try {
                message = m_subprocess.read();
            } catch (const subprocess::Error& e) {
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
                        if (token_available(tokens, 2)) {
                            m_name = tokens[2];  // FIXME parse until new line
                        }
                    }
                }
            } else if (tokens[0] == "option") {
                // TODO
            }
        }
    }

    void Engine::uninitialize() {
        try {
            m_subprocess.write("quit\n");
        } catch (const subprocess::Error& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());  // FIXME terminate
        }
    }

    void Engine::debug(bool active) {
        try {
            m_subprocess.write("debug"s + (active ? "on" : "off") + '\n');
        } catch (const subprocess::Error& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::isready() {
        try {
            m_subprocess.write("isready\n");
        } catch (const subprocess::Error& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }

        const auto begin {std::chrono::steady_clock::now()};

        while (true) {
            const auto now {std::chrono::steady_clock::now()};

            if (now - begin > 5s) {
                try {
                    m_subprocess.terminate();
                } catch (const subprocess::Error&) {}

                throw EngineError("Engine did not respond in a timely manner");
            }

            std::optional<std::string> message;

            try {
                message = m_subprocess.read();
            } catch (const subprocess::Error& e) {
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

    void Engine::setoption(const std::string& name, const std::optional<std::string>& value) {
        try {
            m_subprocess.write("setoption name " + name + (value ? " value " + *value : "") + '\n');
        } catch (const subprocess::Error& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::newgame() {
        try {
            m_subprocess.write("newgame\n");
        } catch (const subprocess::Error& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::position(const std::optional<std::string>& position, const std::vector<std::string>& moves) {
        try {
            const auto moves_str {
                !moves.empty()
                ?
                "moves" + std::accumulate(std::next(moves.cbegin()), moves.cend(), *moves.cbegin(), [](std::string r, const std::string& move) {
                        return std::move(r) + " " + move;
                })
                :
                ""
            };

            m_subprocess.write("position" + (position ? "pos" + *position : "startpos") + moves_str + '\n');
        } catch (const subprocess::Error& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
        }
    }

    void Engine::go(unsigned int wtime, unsigned int btime, unsigned int max_depth, unsigned int max_time) {

    }

    void Engine::stop() {
        try {
            m_subprocess.write("stop\n");
        } catch (const subprocess::Error& e) {
            throw EngineError("Could not write to subprocess: "s + e.what());
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

    bool Engine::token_available(const std::vector<std::string>& tokens, std::size_t index) {
        return index < tokens.size();
    }
}
