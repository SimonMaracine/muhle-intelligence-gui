#include "subprocess.hpp"

#include <cassert>

namespace subprocess {
    Subprocess::Subprocess()
        : m_out(m_context), m_in(m_context), m_process(m_context) {}

    Subprocess::~Subprocess() {
        if (m_context_thread.joinable()) {
            m_context_thread.join();
        }
    }

    void Subprocess::start(const std::string& file_path) {
        m_process = boost_process::process(m_context, file_path, {}, boost_process::process_stdio{m_in, m_out, nullptr});  // FIXME errors?

        task_read_line();

        m_context_thread = std::thread([this]() {
            try {
                m_context.run();
            } catch (const boost_process::system_error& e) {
                // TODO
            }
        });
    }

    std::string Subprocess::read_line() {
        {
            std::lock_guard lock {m_read_mutex};
            if (!m_reading_queue.empty()) {
                const auto result {m_reading_queue.front()};
                m_reading_queue.pop_front();
                return result;
            }
        }

        return {};
    }

    void Subprocess::write_line(const std::string& data) {
        const auto line {data + '\n'};

        boost_process::error_code ec;
        boost::asio::write(m_in, boost::asio::const_buffer(line.data(), line.size()), ec);

        if (ec) {
            throw Error(ec.message());
        }
    }

    void Subprocess::wait() {
        boost_process::error_code ec;
        m_process.wait(ec);

        if (ec) {
            throw Error(ec.message());
        }
    }

    void Subprocess::terminate() {
        boost_process::error_code ec;
        m_process.request_exit(ec);

        if (ec) {
            throw Error(ec.message());
        }
    }

    bool Subprocess::alive() {
        boost_process::error_code ec;
        const bool result {m_process.running(ec)};

        if (ec) {
            throw Error(ec.message());
        }

        return result;
    }

    std::string Subprocess::extract_line(std::string& read_buffer) {
        const std::size_t position {read_buffer.find('\n')};

        assert(position != std::string::npos);

        const auto result {read_buffer.substr(0, position)};
        read_buffer = read_buffer.substr(position + 1);
        return result;
    }

    void Subprocess::task_read_line() {
        boost::asio::async_read_until(m_out, boost::asio::dynamic_buffer(m_read_buffer), '\n', [this](boost_process::error_code ec, std::size_t) {
            if (ec) {
                // TODO
                return;
            }

            {
                std::lock_guard lock {m_read_mutex};
                m_reading_queue.push_back(extract_line(m_read_buffer));
            }

            task_read_line();
        });
    }
}
