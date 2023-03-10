#ifndef __INCLUDE_COPY_HPP__
#define __INCLUDE_COPY_HPP__

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

namespace my
{
    namespace detail
    {

        struct buffer
        {
            static constexpr std::size_t bufferSize =
                std::hardware_destructive_interference_size - sizeof(std::size_t);

            char data[bufferSize]{};
            std::size_t readSize{};
        };

    } // namespace detail

    class copy
    {
    public:
        copy(const std::string &in_filepath, const std::string &out_filepath)
            : _inFile{in_filepath, std::ios_base::binary},
              _outFile{out_filepath, std::ios_base::binary},
              _remainedSymbols{std::filesystem::file_size(in_filepath)}
        {
        }

        /// @brief Runs two threads to async read-write data from one to another file
        inline void run()
        {
            std::jthread threadReader{[this]
                                      { asyncReadFile(); }};
            std::jthread threadWriter{[this]
                                      { asyncWriteToFile(); }};

            threadWriter.join();
            
            std::cout << "1 file was copied successfully!\n";
        }

    private:
        /// @brief Async read data from the input file to the buffer
        inline void asyncReadFile()
        {
            if (!_inFile)
            {
                return;
            }

            uint8_t bufferToWrite = 1;

            while (_inFile)
            {
                std::unique_lock lk(_mtx);
                _cv.wait(lk, [&]
                         { return (bufferToWrite == 1 && !_readyToWrite1) ||
                                  (bufferToWrite == 2 && !_readyToWrite2); });

                if (bufferToWrite == 1)
                {
                    _inFile.read(_buf1.data, _buf1.bufferSize);
                    _buf1.readSize = _inFile.gcount();

                    bufferToWrite = 2;
                    _readyToWrite1 = true;
                }
                else if (bufferToWrite == 2)
                {
                    _inFile.read(_buf2.data, _buf2.bufferSize);
                    _buf2.readSize = _inFile.gcount();

                    bufferToWrite = 1;
                    _readyToWrite2 = true;
                }

                _cv.notify_one();
            }
        }

        /// @brief Async write data from the buffer to the output file
        inline void asyncWriteToFile()
        {
            if (!_outFile)
            {
                return;
            }

            uint8_t bufferToWrite = 1;

            while (_remainedSymbols > 0)
            {
                std::unique_lock lk(_mtx);
                _cv.wait(lk, [&]
                         { return (bufferToWrite == 1 && _readyToWrite1) ||
                                  (bufferToWrite == 2 && _readyToWrite2); });

                if (bufferToWrite == 1)
                {
                    _outFile.write(_buf1.data, _buf1.readSize);
                    _remainedSymbols -= _buf1.readSize;

                    bufferToWrite = 2;
                    _readyToWrite1 = false;
                }
                else if (bufferToWrite == 2)
                {
                    _outFile.write(_buf2.data, _buf2.readSize);
                    _remainedSymbols -= _buf2.readSize;

                    bufferToWrite = 1;
                    _readyToWrite2 = false;
                }

                _cv.notify_one();
            }
        }

    private:
        std::ifstream _inFile;
        std::ofstream _outFile;

        std::mutex _mtx;
        std::condition_variable _cv;
        std::atomic_bool _readyToWrite1{false};
        std::atomic_bool _readyToWrite2{false};

        std::size_t _remainedSymbols;

        detail::buffer _buf1;
        detail::buffer _buf2;
    };
} // namespace my

#endif // __INCLUDE_COPY_HPP__