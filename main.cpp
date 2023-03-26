#include <iostream>

#include <boost/program_options.hpp>

#include <copy_file.hpp>

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

        po::positional_options_description pod;
        pod.add("in", 1);
        pod.add("out", 2);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv)
                      .options(desc)
                      .positional(pod)
                      .run(),
                  vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << '\n';
            return 1;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    if (in_filepath.empty())
    {
        std::cerr << "There is no file to read!\n";
        return EXIT_FAILURE;
    }
    else if (out_filepath.empty())
    {
        std::cerr << "There is no file to write!\n";
        return EXIT_FAILURE;
    }

    if (std::filesystem::exists(in_filepath) == false)
    {
        std::cerr << "This read file does not exist!\n";
        return EXIT_FAILURE;
    }

    my::copy_file copyInstance{in_filepath, out_filepath};
    copyInstance.run();

    return EXIT_SUCCESS;
}
