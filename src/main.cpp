#include "dotenv.h"

#include <iostream>
#include <istream>
#include <fstream>
#include <streambuf>
#include <iterator>

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

extern char **environ;

void usage(int argc, char *argv[]) {
    const char *program = argc > 0 ? argv[0] : "dotenv";
    std::cout << "usage: " << program << " [--file=PATH] [--replace] [--overwrite] [--] [command args...]\n";
    std::cout << "\n";
    std::cout << "Positional arguments:\n";
    std::cout << "    command    The command to run.\n";
    std::cout << "               If no command is provided the constructed environment will be printed.\n";
    std::cout << "    args...    Arguments to the command.\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "    -f, --file=PATH    Use this file instead of \".env\".\n";
    std::cout << "    -r, --replace      Construct an entirely new environment.\n";
    std::cout << "    -o, --overwrite    Overwrite already defined environment variables.\n";
    std::cout << "    -0, --print0       If the environment is printed use NUL bytes instead of new lines.\n";
}

int main(int argc, char *argv[]) {
    static struct option long_options[] = {
        { "replace",   no_argument,       0, 'r' },
        { "overwrite", no_argument,       0, 'o' },
        { "file",      required_argument, 0, 'f' },
        { "help",      no_argument,       0, 'h' },
        { "print0",    no_argument,       0, '0' },
        { 0,           0,                 0,  0  },
    };

    std::string file { ".env" };
    bool overwrite = false;
    bool replace = false;
    char endl = '\n';

    for (;;) {
        int c = getopt_long(argc, argv, "rof:h", long_options, nullptr);

        if (c == -1)
            break;

        switch (c) {
            case 'r':
                replace = true;
                break;
            case 'o':
                overwrite = true;
                break;
            case 'f':
                file = optarg;
                break;
            case 'h':
                usage(argc, argv);
                return 0;
            case '0':
                endl = 0;
                break;
            case '?':
                std::cerr << "Use --help for more details.\n";
                return 1;
        }
    }

    std::unordered_map<std::string, std::string> env;

    if (!replace) {
        for (char **envptr = environ; *envptr; ++ envptr) {
            char *equals = strchr(*envptr, '=');
            if (!equals) {
                std::cerr << "Error: malformed environ\n";
                return 1;
            }
            std::string key(*envptr, equals - *envptr);
            std::string value(equals + 1);
            env[key] = value;
        }
    }

    std::string src;
    
    if (file == "-") {
        src = std::string(
            std::istream_iterator<char>(std::cin),
            std::istream_iterator<char>()
        );
    } else {
        std::ifstream input { file };
        if (!input) {
            std::cerr << "Failed to open file: " << file << std::endl;
            return 1;
        }
        src = std::string(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        );
    }

    dotenv::parse(src, env, overwrite);

    if (optind >= argc) {
        for (auto& entry : env) {
            std::cout << entry.first << "=" << entry.second << endl;
        }
    } else {
        char **childenv = new char*[env.size() + 1];
        char **envptr = childenv;
        std::string buf;
        for (auto& entry : env) {
            buf.clear();
            buf.append(entry.first);
            buf.append("=");
            buf.append(entry.second);
            *envptr = strdup(buf.c_str());
            if (!*envptr) {
                perror("strdup");
                return 1;
            }
            ++ envptr;
        }
        *envptr = nullptr;
        execvpe(argv[optind], argv + optind, childenv);
        std::cerr << "error: (errno: " << errno << ") " << strerror(errno) << std::endl;
        std::cerr << "command:";
        for (int index = optind; index < argc; ++ index) {
            std::cerr << " " << argv[index];
        }
        std::cerr << std::endl;
        return 1;
    }

    return 0;
}
