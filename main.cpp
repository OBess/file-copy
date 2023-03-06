#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <new>
#include <thread>

#include <boost/program_options.hpp>

class copy
{
public:
    copy(const std::string &in_filepath, const std::string &out_filepath)
        : _inFile{in_filepath, std::ios_base::binary},
          _outFile{out_filepath, std::ios_base::binary}
    {
    }

    void run()
    {
        [[maybe_unused]] std::jthread threadReader{[this]
                                                   {
                                                       asyncReadFile();
                                                       std::cout << "Read bytes: " << readerCounter << '\n';
                                                   }};
        [[maybe_unused]] std::jthread threadWriter{[this]
                                                   {
                                                       asyncWriteToFile();
                                                       std::cout << "Wrote bytes: " << writeCounter << '\n';
                                                   }};
    }

private:
    void asyncReadFile()
    {
        while (_inFile)
        {
            char symbol{};

            { // Read for first line of cache
                std::lock_guard lk(_mtx1);

                _inFile.get(symbol);
                for (size_t i = 0; i < _bufferLineSize && _inFile; ++i)
                {
                    _buffer[i] = symbol;
                    _inFile.get(symbol);

                    _counter.fetch_add(1, std::memory_order_release);
                    ++readerCounter;
                }
            }

            _phase.fetch_add(1, std::memory_order_release);

            { // Read for second line of cache
                std::lock_guard lk(_mtx2);

                const size_t doubleSize = _bufferLineSize + _bufferLineSize;

                _inFile.get(symbol);
                for (size_t i = _bufferLineSize; i < doubleSize && _inFile; ++i)
                {
                    _buffer[i] = symbol;
                    _inFile.get(symbol);

                    _counter.fetch_add(1, std::memory_order_release);
                    ++readerCounter;
                }
            }

            _phase.fetch_add(1, std::memory_order_release);
        }
    }

    void asyncWriteToFile()
    {
        while (_counter.load(std::memory_order_acquire) == 0 && _phase.load(std::memory_order_acquire) == 0)
        {
        }

        while (_phase.load(std::memory_order_acquire) > 0)
        {
            { // Write first line of cache to file
                std::lock_guard lk(_mtx1);
                for (size_t i = 0; i < _bufferLineSize && _counter.load(std::memory_order_acquire) > 0; ++i)
                {
                    _outFile << _buffer[i];

                    _counter.fetch_sub(1, std::memory_order_release);
                    ++writeCounter;
                }
            }

            { // Write second line of cache to file
                std::lock_guard lk(_mtx2);

                const size_t doubleSize = _bufferLineSize + _bufferLineSize;
                for (size_t i = _bufferLineSize; i < doubleSize && _counter.load(std::memory_order_acquire) > 0; ++i)
                {
                    _outFile << _buffer[i];

                    _counter.fetch_sub(1, std::memory_order_release);
                    ++writeCounter;
                }
            }
        }
    }

private:
    std::ifstream _inFile;
    std::ofstream _outFile;
    std::mutex _mtx1, _mtx2;

    bool _endOfRead{false};

    std::atomic<int64_t> _counter{0};
    std::atomic<int64_t> _phase{0};
    size_t readerCounter{0};
    size_t writeCounter{0};

    static constexpr size_t _bufferLineSize =
        std::hardware_destructive_interference_size;
    char _buffer[_bufferLineSize + _bufferLineSize]{};
};

int main(int argc, const char *argv[])
{
    std::string in_filepath{};
    std::string out_filepath{};

    try
    {
        namespace po = boost::program_options;

        po::options_description desc("Allowed options");

        // clang-format off
        desc.add_options()
            ("help", "produce help message")
            ("in", po::value<std::string>(&in_filepath), "the file to read")
            ("out", po::value<std::string>(&out_filepath), "the file to write");
        // clang-format on
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << "\n";
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    // if (in_filepath.empty())
    // {
    //     std::cerr << "There is no file to read!\n";
    //     return -1;
    // }
    // else if (out_filepath.empty())
    // {
    //     std::cerr << "There is no file to write!\n";
    //     return -1;
    // }

    // if (std::filesystem::exists(in_filepath) == false)
    // {
    //     std::cerr << "This read file does not exist!\n";
    //     return -1;
    // }

    in_filepath = "C:\\My\\Projects\\cpp\\file-copy\\deploy\\Debug\\lab 1.docx";
    out_filepath = "C:\\My\\Projects\\cpp\\file-copy\\deploy\\Debug\\output.docx";

    copy copyInstance{in_filepath, out_filepath};
    copyInstance.run();

    return 0;
}
