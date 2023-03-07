#include <iostream>

#include <boost/program_options.hpp>

#include <copy.hpp>

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

    my::copy copyInstance{"C:\\My\\Projects\\cpp\\file-copy\\deploy\\Debug\\lab 1.docx",
                          "C:\\My\\Projects\\cpp\\file-copy\\deploy\\Debug\\output.docx"};
    // my::copy copyInstance{"C:\\My\\Projects\\cpp\\file-copy\\.gitignore",
    //                       "C:\\My\\Projects\\cpp\\file-copy\\deploy\\Debug\\gitignore.txt"};
    // my::copy copyInstance{in_filepath, out_filepath};
    copyInstance.run();

    return 0;
}
