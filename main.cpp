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
        : _inFile{in_filepath}, _outFile{out_filepath}
    {
    }

    void run()
    {
        std::jthread th1{[this]
                         { asyncReadFile(); }};
        asyncWriteToFile();
    }

private:
    void asyncReadFile()
    {
        while (_inFile)
        {
            { // Read for first line of cache
                std::lock_guard lk(_mtx1);
                for (size_t i = 0; i < _bufferLineSize; ++i)
                {
                    char symbol{};
                    _inFile >> std::noskipws >> symbol;

                    _buffer[i] = symbol;
                }
            }

            { // Read for second line of cache
                std::lock_guard lk(_mtx2);

                const size_t doubleSize = _bufferLineSize + _bufferLineSize;
                for (size_t i = _bufferLineSize; i < doubleSize; ++i)
                {
                    char symbol{};
                    _inFile >> std::noskipws >> symbol;

                    _buffer[i] = symbol;
                }
            }
        }

        _write = false;
    }

    void asyncWriteToFile()
    {
        while (_write)
        {
            { // Write first line of cache to file
                std::lock_guard lk(_mtx1);
                for (size_t i = 0; i < _bufferLineSize; ++i)
                {
                    if (_buffer[i] == '\0')
                    {
                        break;
                    }

                    _outFile << _buffer[i];
                }
            }

            { // Write second line of cache to file
                std::lock_guard lk(_mtx2);

                const size_t doubleSize = _bufferLineSize + _bufferLineSize;
                for (size_t i = _bufferLineSize; i < doubleSize; ++i)
                {
                    if (_buffer[i] == '\0')
                    {
                        break;
                    }

                    _outFile << _buffer[i];
                }
            }
        }
    }

private:
    std::ifstream _inFile;
    std::ofstream _outFile;
    std::mutex _mtx1, _mtx2;

    bool _write{true};

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

    if (in_filepath.empty())
    {
        std::cerr << "There is no file to read!\n";
        return -1;
    }
    else if (out_filepath.empty())
    {
        std::cerr << "There is no file to write!\n";
        return -1;
    }

    if (std::filesystem::exists(in_filepath) == false)
    {
        std::cerr << "This read file does not exist!\n";
        return -1;
    }

    copy copyInstance{in_filepath, out_filepath};
    copyInstance.run();

    return 0;
}
