#ifdef _WIN32
#include <windows.h> //GetModuleFileNameW
#elif __APPLE__
#include <mach-o/dyld.h>
#elif __linux__
#include <limits.h>
#include <unistd.h> //readlink
#endif

#include <string>
#include <vector>
#include <regex>
#include <map>
#include <algorithm>

#include "logger.h"



class Template;
class Template
{
    std::string directory;
    std::string alias;
    std::vector<std::string> replaceables = {};
    std::map<std::string, std::vector<std::string>> pathToReplaceables = {};
    std::map<std::string, std::string> replacedText;
    std::vector<std::string> bannedPaths = {"node_modules"};
    std::string outputDirectory = "";
    std::string outputFolder = "";
    static Logger *logger;

public:
    Template(std::string directory)
    {
        this->directory = directory;
        size_t last_pos = directory.find_last_of('/');
        this->alias = directory.substr(last_pos + 1, directory.length() - last_pos - 1);

        GatherReplaceables();
    }

    std::vector<std::string> GetReplaceables()
    {
        return replaceables;
    }

    std::string *GetReplacedTextPtr(std::string replaceable)
    {
        return &(replacedText[replaceable]);
    }

    std::string *GetOutputDirectoryPtr()
    {
        return &outputDirectory;
    }
    std::string *GetOutputFolderPtr()
    {
        return &outputFolder;
    }

    void GatherReplaceables()
    {
        // std::regex rgx("\\%.*?\\%");
        // std::regex rgx("\\%[^\\s\\-\\n]*?\\%");
        // std::regex rgx("\\^[A-Za-z0-9]*?\\^");
        std::regex rgx("\\^[A-Za-z0-9]+?\\^");

        for (auto &p : std::filesystem::recursive_directory_iterator(directory))
        {
            if (p.is_directory())
            {
                continue;
            }

            std::string pathString = p.path().string();
            bool foundBannedString = false;
            for (auto bp : bannedPaths)
            {
                if (pathString.find(bp) != std::string::npos)
                {
                    foundBannedString = true;
                    break;
                }
            }

            if (pathString != "" && !foundBannedString)
            {
                std::fstream originalFile;
                originalFile.open(p.path());
                if (!originalFile.is_open())
                { // check for and open the .env file of devilbox
                    continue;
                }

                std::string input;
                while (std::getline(originalFile, input)) // copy the whole file to memory
                {
                    std::smatch match;
                    std::string::const_iterator searchStart(input.cbegin());
                    while (std::regex_search(searchStart, input.cend(), match, rgx))
                    {
                        if(std::find(pathToReplaceables[pathString].begin(), pathToReplaceables[pathString].end(), match[0]) == pathToReplaceables[pathString].end()){


                            pathToReplaceables[pathString].push_back(match[0]);
                        }




                        // std::cout << match[0] << std::endl;
                        if (std::find(replaceables.begin(), replaceables.end(), match[0]) == replaceables.end())
                        {
                            replaceables.push_back(match[0]);
                            replacedText[match[0]] = "";
                        }

                        searchStart = match.suffix().first;
                    }
                }
                originalFile.close();
            }
        }
    }

    std::string getDirectory()
    {
        return directory;
    }

    std::string getAlias()
    {
        return alias;
    }

    static std::string Command(std::string command)
    {
        // set up file redirection
        std::filesystem::path redirection = std::filesystem::absolute(".output.temp");
        command.append(" &> \"" + redirection.string() + "\"");

        // execute command
        auto status = std::system(command.c_str());

        // read redirection file and remove the file
        std::ifstream output_file(redirection);
        std::string output((std::istreambuf_iterator<char>(output_file)), std::istreambuf_iterator<char>());
        std::filesystem::remove(redirection);

        return output;
    }

    void GenerateFiles()
    { // make this break and log
        if (outputDirectory == "" || outputFolder == "")
        {
            return;
        }

        /*logger->log(Command());

        curl -X POST -H "Content-Type: application/json" -d '{
            "scm": "git",
            "project": {
                "key": "GGAT"
            }
        }' https://api.bitbucket.org/2.0/repositories/designamiteltd/test-repo*/

        for (auto it : replaceables)
        {
            if (replacedText[it] == "")
                return;
        }
        std::cout << outputDirectory + "/" + outputFolder << std::endl;
        // CopyRecursive(directory, outputDirectory + "/" + outputFolder);

        for (auto &p : std::filesystem::recursive_directory_iterator(directory))
        {


            std::string pathString = p.path().string();

            if (pathString == "")
            {
                continue;
            }



            bool foundBannedString = false;
            for (auto bp : bannedPaths)
            {
                if (pathString.find(bp) != std::string::npos)
                {
                    foundBannedString = true;
                    break;
                }
            }

            if (foundBannedString)
            {
                continue;
            }

            std::string doctoredPath = outputDirectory + "/" + outputFolder + pathString.substr(directory.length(), pathString.length() - directory.length());

            if (p.is_directory())
            {

                std::filesystem::create_directories(doctoredPath);
                continue;
            }


            std::fstream originalFile;
            originalFile.open(p.path());
            if (!originalFile.is_open())
            {
                continue;
            }

            bool replaceReplaceables = /*pathToReplaceables[pathString] ?*/ pathToReplaceables[pathString].size() > 0 /*: false*/;

            std::string input;
            std::vector<std::string> lines;
            while (std::getline(originalFile, input)) // copy the whole file to memory
            {
                if(replaceReplaceables){
                    for (std::string replaceable: pathToReplaceables[pathString]){
                        if(input.find(replaceable) != std::string::npos){
                            input.replace(input.find(replaceable), replaceable.length(), replacedText[replaceable]);
                        }

                    }
                }
                lines.push_back(input);
            }

            originalFile.close();

            std::ofstream outputFile;

            outputFile.open(doctoredPath, std::ios_base::trunc);
            if (!outputFile.is_open())
            {
                continue;
            }

            for (auto line : lines)
            {
                outputFile << line << std::endl;
            }
            outputFile.close();
        }
    }

    static std::vector<std::string> GetDirectories(const std::string &s)
    {
        std::vector<std::string> r;
        for (auto &p : std::filesystem::directory_iterator(s))
            if (p.is_directory())
                r.push_back(p.path().string());
        return r;
    }

    static std::filesystem::path GetExePath()
    {
#ifdef _WIN32
        wchar_t path[MAX_PATH] = {0};
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return path;
#elif __APPLE__
        char path[PATH_MAX] = {0};
        uint32_t size = PATH_MAX;
        if (_NSGetExecutablePath(path, &size) == 0)
            return path;
        return {};
#elif __linux__
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        return std::string(result, (count > 0) ? count : 0);
#endif
    }

    static std::vector<Template *> GetAllTemplates(Logger *loggerIn)
    {
        std::vector<Template *> templates;

        logger = loggerIn;

        std::string path = GetExePath();
        // remove exe from path
        path = path.substr(0, path.find_last_of('/'));
        path += "/Templates";
        for (auto dir : GetDirectories(path))
        {
            Template *tp = new Template(dir);
            templates.push_back(tp);
        }

        return templates;
    }

    void CopyRecursive(const std::filesystem::path &src, const std::filesystem::path &target) noexcept
    {
        try
        {
            std::filesystem::copy(src, target, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
        }
        catch (std::exception &e)
        {
            std::cout << e.what();
        }
    }
};

Logger *Template::logger = nullptr;
