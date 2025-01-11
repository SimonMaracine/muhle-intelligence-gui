#include "subprocess.hpp"

#include <utility>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <cerrno>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>

using namespace std::string_literals;

namespace subprocess {
    Subprocess::Subprocess(const std::string& file_path) {
        int fd_r[2] {};
        if (pipe(fd_r) < 0) {
            throw Error("Could not create reading pipe"s + strerror(errno));
        }

        int fd_w[2] {};
        if (pipe(fd_w) < 0) {
            throw Error("Could not create writing pipe"s + strerror(errno));
        }

        const pid_t pid {fork()};

        if (pid < 0) {
            throw Error("Could not create subprocess"s + strerror(errno));
        } else if (pid == 0) {
            close(fd_r[0]);
            close(fd_w[1]);

            if (dup2(fd_r[1], STDOUT_FILENO) < 0) {
                std::abort();
            }

            if (dup2(fd_w[0], STDIN_FILENO) < 0) {
                std::abort();
            }

            char* const argv[] { const_cast<char*>(file_path.c_str()), nullptr };

            if (execv(file_path.c_str(), argv) < 0) {
                std::abort();
            }

            // Child execution stops in execv
        } else {
            close(fd_r[1]);
            close(fd_w[0]);

            // Parent reads from fd_r[0]
            // Parent writes to fd_w[1]

            m_input = fd_r[0];
            m_output = fd_w[1];
            m_child_pid = pid;
        }
    }

    Subprocess::~Subprocess() noexcept {
        if (!(m_child_pid < 0)) {
            std::abort();
        }
    }

    Subprocess::Subprocess(Subprocess&& other) noexcept {
        m_input = other.m_input;
        m_output = other.m_output;
        m_child_pid = other.m_child_pid;
        m_read_buffer = std::move(other.m_read_buffer);
        m_reading_queue = std::move(other.m_reading_queue);

        other.m_child_pid = -1;
    }

    Subprocess& Subprocess::operator=(Subprocess&& other) noexcept {
        if (!(m_child_pid < 0)) {
            std::abort();
        }

        m_input = other.m_input;
        m_output = other.m_output;
        m_child_pid = other.m_child_pid;
        m_read_buffer = std::move(other.m_read_buffer);
        m_reading_queue = std::move(other.m_reading_queue);

        other.m_child_pid = -1;

        return *this;
    }

    std::optional<std::string> Subprocess::readl() const {
        if (!m_reading_queue.empty()) {
            const auto result {m_reading_queue.front()};
            m_reading_queue.pop_front();
            return std::make_optional(result);
        }

        fd_set set;
        FD_ZERO(&set);
        FD_SET(m_input, &set);

        timeval time;
        time.tv_sec = 0;
        time.tv_usec = 0;

        const int result {select(m_input + 1, &set, nullptr, nullptr, &time)};

        if (result < 0) {
            throw Error("Could not poll read file: "s + strerror(errno));
        } else if (result != 1) {
            return std::nullopt;
        }

        char buffer[256] {};
        const ssize_t bytes {::read(m_input, buffer, sizeof(buffer))};

        if (bytes < 0) {
            throw Error("Could not read from file: "s + strerror(errno));
        }

        if (bytes == 0) {
            return std::nullopt;
        }

        m_read_buffer += std::string(buffer, bytes);

        if (m_read_buffer.find('\n') == m_read_buffer.npos) {
            return std::nullopt;
        }

        std::size_t last_index {0};

        for (std::size_t i {0}; i < m_read_buffer.size(); i++) {
            if (m_read_buffer[i] == '\n') {
                m_reading_queue.push_back(m_read_buffer.substr(last_index, i - last_index));
                last_index = i + 1;
            }
        }

        const auto remainder {m_read_buffer.substr(last_index)};
        m_read_buffer.clear();
        m_read_buffer += remainder;

        {
            const auto result {m_reading_queue.front()};
            m_reading_queue.pop_front();
            return std::make_optional(result);
        }
    }

    void Subprocess::write(const std::string& data) const {
        const char* buffer {data.data()};
        std::size_t size {data.size()};

        while (true) {
            const ssize_t bytes {::write(m_output, buffer, size)};

            if (bytes < 0) {
                throw Error("Could not write to file: "s + strerror(errno));
            }

            if (static_cast<std::size_t>(bytes) < size) {
                buffer = data.data() + bytes;
                size -= bytes;

                continue;
            }

            break;
        }
    }

    void Subprocess::wait() {
        if (waitpid(std::exchange(m_child_pid, -1), nullptr, 0) < 0) {
            throw Error("Failed waiting for subprocess: "s + strerror(errno));
        }
    }

    void Subprocess::terminate() {
        if (kill(std::exchange(m_child_pid, -1), SIGTERM) < 0) {
            throw Error("Could not send terminate signal to subprocess: "s + strerror(errno));
        }
    }

    bool Subprocess::active() const noexcept {
        return m_child_pid != -1;
    }
}
