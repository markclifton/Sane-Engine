
#include <fstream>
#include <string>
#include <vector>

std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

typedef std::vector<std::string> Tokens;

Tokens split(const std::string& s, char seperator)
{
    Tokens tokens;
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = s.find(seperator, pos)) != std::string::npos)
    {
        std::string substring(s.substr(prev_pos, pos - prev_pos));
        tokens.push_back(substring);
        prev_pos = ++pos;
    }

    tokens.push_back(s.substr(prev_pos, pos - prev_pos));
    return tokens;
}

int main(int argc, char* argv[])
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
    {
        args.push_back(argv[i]);
    }

    if (args.empty()) return -1;

    std::string outfile = "";
    for (int i = 0; i < args.size(); ++i)
    {
        if (args[i] == "-o" && (i+1 < args.size()))
        {
            outfile = args[i + 1];
            args.erase(args.begin() + i + 1);
            args.erase(args.begin() + i);
            break;
        }
    }

    std::string type = "";
    for (int i = 0; i < args.size(); ++i)
    {
        if (args[i] == "-t" && (i + 1 < args.size()))
        {
            type = args[i + 1];
            args.erase(args.begin() + i + 1);
            args.erase(args.begin() + i);
            break;
        }
    }

#pragma warning( disable : 4996 )
    std::fstream outStream(outfile.c_str(), std::ios_base::app | std::ios_base::binary);
    if (!outStream.is_open())
        return -2;

    outStream.seekp(0, std::ios_base::end);

    for (auto& file : args)
    {
        uint32_t fileLength = (uint32_t)filesize(file.c_str());
        std::string fileName = type + "_" + split(file, '\\').back();

        for (auto& c : fileName) c = toupper(c);
        for (auto& c : fileName) if (c == '/' || c == '\\' || c == '.') c = '_';

        while (fileName.front() == '_')
            fileName.erase(fileName.begin());

        std::ifstream file_ifs(file, std::ios_base::binary);
        if (!file_ifs.is_open())
            return -3;

        uint32_t fileNameLength = static_cast<uint32_t>(fileName.length());
        outStream.write((char*)&fileNameLength, sizeof(fileNameLength));
        outStream << fileName;
        outStream.write((char*)&fileLength, sizeof(fileLength));
        outStream << file_ifs.rdbuf();
    }

    outStream.close();

    return 0;
}
