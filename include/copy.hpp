#ifndef __INCLUDE_COPY_HPP__
#define __INCLUDE_COPY_HPP__

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

        void run()
        {
#if 0
        // Naive copy in one thread
        char c{};
        _inFile.get(c);

        while (_inFile)
        {
            _outFile << c;
            _inFile.get(c);
        }
#else
            std::jthread threadReader{[this]
                                      { asyncReadFile(); }};
            std::jthread threadWriter{[this]
                                      { asyncWriteToFile(); }};

            threadWriter.join();
#endif
            std::cout << "1 file was copied!\n";
        }

    private:
        void asyncReadFile()
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
                         { return (bufferToWrite == 1 && !_readyToWrite1) || (bufferToWrite == 2 && !_readyToWrite2); });

                if (bufferToWrite == 1)
                {
                    _inFile.read(_buffer1, _bufferSize);

                    bufferToWrite = 2;
                    _readyToWrite1 = true;
                }
                else if (bufferToWrite == 2)
                {
                    _inFile.read(_buffer2, _bufferSize);

                    bufferToWrite = 1;
                    _readyToWrite2 = true;
                }

                _cv.notify_one();
            }
        }

        void asyncWriteToFile()
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
                         { return (bufferToWrite == 1 && _readyToWrite1) || (bufferToWrite == 2 && _readyToWrite2); });

                if (bufferToWrite == 1)
                {
                    for (size_t i = 0; i < _bufferSize && _remainedSymbols > 0; ++i)
                    {
                        _outFile.put(_buffer1[i]);
                        --_remainedSymbols;
                    }

                    bufferToWrite = 2;
                    _readyToWrite1 = false;
                }
                else if (bufferToWrite == 2)
                {
                    for (size_t i = 0; i < _bufferSize && _remainedSymbols > 0; ++i)
                    {
                        _outFile.put(_buffer2[i]);
                        --_remainedSymbols;
                    }

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
        bool _readyToWrite1{false};
        bool _readyToWrite2{false};

        size_t _remainedSymbols{0};

        static constexpr size_t _bufferSize =
            std::hardware_destructive_interference_size;
        char _buffer1[_bufferSize]{};
        char _buffer2[_bufferSize]{};
    };
} // namespace my

#endif // __INCLUDE_COPY_HPP__