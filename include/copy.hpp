#ifndef __INCLUDE_COPY_HPP__
#define __INCLUDE_COPY_HPP__

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <new>
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
            threadReader.join();
            threadWriter.join();

            std::cout << "Read bytes: " << readerCounter << '\n';
            std::cout << "Wrote bytes: " << writeCounter << '\n';
#endif
        }

    private:
        void asyncReadFile()
        {
            
            while (_inFile)
            {
                { // Read for first line of cache
                    std::unique_lock lk(_mtx1);
                    _inFile.read(_buffer1, _bufferSize);
                }

                { // Read for second line of cache
                    std::unique_lock lk(_mtx2);
                    _inFile.read(_buffer2, _bufferSize);
                }

                readerCounter += _bufferSize * 2;
            }
        }

        void asyncWriteToFile()
        {
            while (_remainedSymbols > 0)
            {
                { // Write first line of cache to file
                    std::unique_lock lk(_mtx1);
                    for (size_t i = 0; i < _bufferSize && _remainedSymbols > 0; ++i)
                    {
                        _outFile << _buffer1[i];

                        --_remainedSymbols;
                        ++writeCounter;
                    }
                }

                { // Write second line of cache to file
                    std::unique_lock lk(_mtx2);
                    for (size_t i = 0; i < _bufferSize && _remainedSymbols > 0; ++i)
                    {
                        _outFile << _buffer2[i];

                        --_remainedSymbols;
                        ++writeCounter;
                    }
                }
            }
        }

    private:
        std::ifstream _inFile;
        std::ofstream _outFile;

        std::mutex _mtx1, _mtx2, _mtx3;
        std::condition_variable _cv1, _cv2;

        size_t readerCounter{0};
        size_t writeCounter{0};

        size_t _remainedSymbols{0};

        static constexpr size_t _bufferSize =
            std::hardware_destructive_interference_size;
        char _buffer1[_bufferSize]{};
        char _buffer2[_bufferSize]{};
    };
} // namespace my

#endif // __INCLUDE_COPY_HPP__