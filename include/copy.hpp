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

            while (_inFile)
            {
                {
                    std::unique_lock lk(_mtx1);
                    _cv1.wait(lk, [&]
                              { return !_readyToWrite1.load(std::memory_order_acquire); });

                    _inFile.read(_buffer1, _bufferSize);

                    _readyToWrite1.store(true, std::memory_order_release);
                    _cv1.notify_one();
                }

                {
                    std::unique_lock lk(_mtx2);
                    _cv2.wait(lk, [&]
                              { return !_readyToWrite2.load(std::memory_order_acquire); });

                    _inFile.read(_buffer2, _bufferSize);

                    _readyToWrite2.store(true, std::memory_order_release);
                    _cv2.notify_one();
                }
            }
        }

        /// @brief Async write data from the buffer to the output file
        inline void asyncWriteToFile()
        {
            if (!_outFile)
            {
                return;
            }

            while (_remainedSymbols > 0)
            {
                {
                    std::unique_lock lk(_mtx1);
                    _cv1.wait(lk, [&]
                              { return _readyToWrite1.load(std::memory_order_acquire); });

                    for (size_t i = 0; i < _bufferSize && _remainedSymbols > 0; ++i)
                    {
                        _outFile.put(_buffer1[i]);
                        --_remainedSymbols;
                    }

                    _readyToWrite1.store(false, std::memory_order_release);
                    _cv1.notify_one();
                }

                {
                    std::unique_lock lk(_mtx2);
                    _cv2.wait(lk, [&]
                              { return _readyToWrite2.load(std::memory_order_acquire); });

                    for (size_t i = 0; i < _bufferSize && _remainedSymbols > 0; ++i)
                    {
                        _outFile.put(_buffer2[i]);
                        --_remainedSymbols;
                    }

                    _readyToWrite2.store(false, std::memory_order_release);
                    _cv2.notify_one();
                }
            }
        }

    private:
        std::ifstream _inFile;
        std::ofstream _outFile;

        std::mutex _mtx1, _mtx2;
        std::condition_variable _cv1, _cv2;
        std::atomic_bool _readyToWrite1{false};
        std::atomic_bool _readyToWrite2{false};

        size_t _remainedSymbols{0};

        static constexpr size_t _bufferSize =
            std::hardware_destructive_interference_size;
        char _buffer1[_bufferSize]{};
        char _buffer2[_bufferSize]{};
    };
} // namespace my

#endif // __INCLUDE_COPY_HPP__