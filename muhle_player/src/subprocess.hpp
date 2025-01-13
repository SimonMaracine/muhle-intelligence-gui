#pragma once

#include <string>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <deque>

#ifdef __GNUG__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wconversion"
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

        void start(const std::string& file_path);
        std::string read_line();
        void write_line(const std::string& data);
        void wait();
        void terminate();
        bool alive();
    private:
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
    };

    struct Error : public std::runtime_error {
        explicit Error(const char* message)
            : std::runtime_error(message) {}
        explicit Error(const std::string& message)
            : std::runtime_error(message) {}
    };
}
