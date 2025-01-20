#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <deque>
#include <stdexcept>
#include <exception>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
    #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define BOOST_PROCESS_VERSION 2
#include <boost/process.hpp>
#include <boost/asio.hpp>

#ifdef __GNUG__
    #pragma GCC diagnostic pop
#endif

namespace boost_process = boost::process::v2;

namespace subprocess {
    class Subprocess {
    public:
        Subprocess();
        ~Subprocess();

        Subprocess(const Subprocess&) = delete;
        Subprocess& operator=(const Subprocess&) = delete;
        Subprocess(Subprocess&&) = delete;
        Subprocess& operator=(Subprocess&&) = delete;

        void open(const std::string& file_path);
        void wait();
        bool alive();
        std::string read_line();
        void write_line(const std::string& data);
    private:
        void throw_if_error();
        void kill();
        static std::string extract_line(std::string& read_buffer);
        void task_read_line();

        boost::asio::io_context m_context;
        boost::asio::readable_pipe m_out;
        boost::asio::writable_pipe m_in;
        boost_process::process m_process;
        std::thread m_context_thread;

        std::mutex m_read_mutex;
        std::string m_read_buffer;
        std::deque<std::string> m_reading_queue;

        std::exception_ptr m_exception;
    };

    struct SubprocessError : public std::runtime_error {
        explicit SubprocessError(const char* message)
            : std::runtime_error(message) {}
        explicit SubprocessError(const std::string& message)
            : std::runtime_error(message) {}
    };
}
