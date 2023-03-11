#ifndef __INCLUDE_COPY_HPP__
#define __INCLUDE_COPY_HPP__

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <queue>

namespace my
{
    namespace detail
    {

        struct buffer
        {
            static constexpr std::size_t bufferSize =
                std::hardware_destructive_interference_size * 100 - sizeof(std::size_t);

            char data[bufferSize]{};
            std::size_t readSize{};
        };

    } // namespace detail

    class copy
    {
    public:
        copy(const std::string &in_filepath, const std::string &out_filepath)
            : _inFile{in_filepath, std::ios_base::binary},
              _outFile{out_filepath, std::ios_base::binary}
        {
            if (_inFile)
            {
                _remainedSymbols = std::filesystem::file_size(in_filepath);
            }
        }

        /// @brief Runs two threads to async read-write data from one to another file
        inline void run()
        {
            if (!_inFile || !_outFile)
            {
                std::cout << "Cannot run copping because one of the two paths is invalid!\n";
                return;
            }

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
            uint8_t bufferToWrite = 1;

            while (_inFile)
            {
                if (_sizeOfBufferQueue > _bufferQueue.size())
                {
                    detail::buffer buffer;
                    _inFile.read(buffer.data, buffer.bufferSize);
                    buffer.readSize = _inFile.gcount();

                    std::lock_guard lk(_mtx);
                    _bufferQueue.push(std::move(buffer));
                }
            }
        }

        /// @brief Async write data from the buffer to the output file
        inline void asyncWriteToFile()
        {
            while (_remainedSymbols > 0)
            {
                if (_bufferQueue.empty() == false)
                {
                    std::unique_lock lk(_mtx);

                    auto nextBuffer = _bufferQueue.front();
                    _bufferQueue.pop();

                    lk.unlock();

                    _outFile.write(nextBuffer.data, nextBuffer.readSize);
                    _remainedSymbols -= nextBuffer.readSize;
                }
            }
        }

    private:
        std::ifstream _inFile;
        std::ofstream _outFile;

        std::mutex _mtx;

        std::size_t _remainedSymbols{};

        std::queue<detail::buffer> _bufferQueue;
        static constexpr uint32_t _sizeOfBufferQueue{100};
    };
} // namespace my

#endif // __INCLUDE_COPY_HPP__