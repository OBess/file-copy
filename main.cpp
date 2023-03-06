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
          _outFile{out_filepath, std::ios_base::binary},
          _sizeOfFile{std::filesystem::file_size(in_filepath)}
    {
    }

    void run()
    {
        // char c{};
        // _inFile.get(c);

        // while (_inFile)
        // {
        //     _outFile << c;
        //     _inFile.get(c);
        // }

        std::jthread threadReader{[this]
                                  { asyncReadFile(); }};
        std::jthread threadWriter{[this]
                                  { asyncWriteToFile(); }};
        threadReader.join();
        threadWriter.join();

        std::cout << "Read bytes: " << readerCounter << '\n';
        std::cout << "Wrote bytes: " << writeCounter << '\n';
    }

private:
    void asyncReadFile()
    {
        while (_inFile)
        {
            { // Read for first line of cache
                std::unique_lock lk(_mtx1);
                for (size_t i = 0; i < _bufferLineSize; ++i)
                {
                    _inFile.get(_buffer1[i]);
                    ++readerCounter;
                }
            }

            { // Read for second line of cache
                std::unique_lock lk(_mtx2);
                for (size_t i = 0; i < _bufferLineSize; ++i)
                {
                    _inFile.get(_buffer2[i]);
                    ++readerCounter;
                }
            }
        }
    }

    void asyncWriteToFile()
    {
        while (_sizeOfFile > 0)
        {
            { // Write first line of cache to file
                std::unique_lock lk(_mtx1);
                for (size_t i = 0; i < _bufferLineSize && _sizeOfFile > 0; ++i)
                {
                    _outFile << _buffer1[i];

                    --_sizeOfFile;
                    ++writeCounter;
                }
            }

            { // Write second line of cache to file
                std::unique_lock lk(_mtx2);
                for (size_t i = 0; i < _bufferLineSize && _sizeOfFile > 0; ++i)
                {
                    _outFile << _buffer2[i];

                    --_sizeOfFile;
                    ++writeCounter;
                }
            }
        }
    }

private:
    std::ifstream _inFile;
    std::ofstream _outFile;
    std::mutex _mtx1, _mtx2;

    size_t readerCounter{0};
    size_t writeCounter{0};

    size_t _sizeOfFile{0};

    static constexpr size_t _bufferLineSize =
        std::hardware_destructive_interference_size;
    char _buffer1[_bufferLineSize]{};
    char _buffer2[_bufferLineSize]{};
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
    // in_filepath = "C:\\My\\Projects\\cpp\\file-copy\\.gitignore";
    // out_filepath = "C:\\My\\Projects\\cpp\\file-copy\\deploy\\Debug\\gitignore.txt";

    copy copyInstance{in_filepath, out_filepath};
    copyInstance.run();

    return 0;
}
