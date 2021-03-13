#include "CLI.h"

void format(MyExt2& fs) {
    std::cout << "WARNING!!! ALL data will be lost!!! continue?(y/N) ";
    std::string res;
    std::getline(std::cin, res);
    if (res == "y" || res == "Y") {
        std::cout << "enter new volume name:(default:MyExt2) ";
        std::string vn;
        std::getline(std::cin, vn);
        if (vn == "")
            vn = "MyExt2";
        if (vn.size() > 15) {
            std::cout << "volume name too long! < 16 chars.\n";
        }
        fs.format(vn);
    }
}

void CLI(MyExt2& test, int argc, char* argv[]) {
    while (!test.is_formatted()) {
        std::cout << "New disk, please format it.\n";
        format(test);
    }
    std::string command, user("root"), token;
    if (argc > 1)
        user = argv[1];
    std::regex whites("\\s+");
    std::sregex_token_iterator end;
    while (true)
    {
        std::cout << user + "@" + test.volume_name() + ":" + test.curr_path() + "$ ";
        std::getline(std::cin, command);
        std::sregex_token_iterator it(command.begin(), command.end(), whites, -1);
        if (it != end)
            token = it->str();

        if (token == "") {
            if (++it != end)
                token = it->str();
            if (token == "")
                continue;
        }

        if (token == "ls") {
            if (++it == end) {
                test.ls("./");
            }
            else {
                do {
                    token = it->str();
                    test.ls(token);
                } while (++it != end);
            }
        }
        else if (token == "cd") {
            if (++it == end) {
                continue;
            }
            token = it->str();
            test.cd(token);
        }
        else if (token == "mkdir") {
            if (++it == end) {
                l("mkdir: missing operand");
                continue;
            }
            do {
                token = it->str();
                test.mkdir(token);
            } while (++it != end);
        }
        else if (token == "create") {
            if (++it == end) {
                l("create: missing operand");
                continue;
            }
            do {
                token = it->str();
                test.create(token);
            } while (++it != end);
        }
        else if (token == "rm") {
            if (++it == end) {
                l("rm: missing operand");
                continue;
            }
            do {
                token = it->str();
                test.rm(token);
            } while (++it != end);
        }
        else if (token == "read") {
            if (++it == end) {
                l("read: missing operand");
                continue;
            }
            do {
                token = it->str();
                test.read(token);
            } while (++it != end);
        }
        else if (token == "write") {
            if (++it == end) {
                l("write: missing operand");
                continue;
            }
            token = it->str();
            test.write(token);
        }
        else if (token == "format") {
            format(test);
        }
        else if (token == "chmod") {
            if (++it == end) {
                l("chmod: missing mode");
                continue;
            }
            token = it->str();
            if (++it == end) {
                l("chmod: missing file");
                continue;
            }
            char anum = token[0] - '0';
            if (anum > 7) {
                l("chmod: not a correct mode!");
                continue;
            }
            std::string file;
            do {
                file = it->str();
                test.chmod(anum, file);
            } while (++it != end);
        }
        else if (token == "exit" || token == "quit") {
            std::cout << "OK, exit.\n";
            break;
        }
        else {
            std::cout << token + ": command not found\n";
        }
    }
}