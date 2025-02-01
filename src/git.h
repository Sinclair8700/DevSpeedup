#pragma once
#include "logger.h"
#include <vector>
#include <algorithm>    

#include "crypto.h"
#include "../json/single_include/nlohmann/json.hpp"

class GitRepository {
    std::string name = "";
    std::string path = "";
    bool selected = false;
    std::vector<int> changes = {0, 0, 0};
    static std::vector<std::string> ignoredFiles;
    static Logger* logger;
    static std::string rootPath;
    static std::vector<int> allChanges;
    std::string uncommitedChanges = "";
    
    //static int last;
    public:
    static std::vector<GitRepository*> gitRepositories;
    GitRepository(std::string path){
        this->path = path;
        size_t last_pos = path.find_last_of('/');
        this->name = path.substr(last_pos + 1, path.length() - last_pos - 1);
   
    }
    ~GitRepository(){

    }
    GitRepository(GitRepository &g){
        this->name = g.name;
        this->path = g.path;
        this->selected = g.selected;
    }
    std::string GetName()
    {
        return name;
    }
    std::string GetPath()
    {
        return path;
    }

    static std::string GetRootPath()
    {
        return rootPath;
    }
    static void SetRootPath(std::string path){
        rootPath = path;
    }
    static void SetRootPathAndSearch(std::string path){
        SetRootPath(path);
        Search();
    }
    static void SetLogger(Logger* loggerIn){
        logger = loggerIn;
    }
    static void Search(){
        if(rootPath == ""){
            logger->Log("You need to set the root path first");
            return;
        }

        for(auto gitRepository : gitRepositories){
            delete gitRepository;
        }

        gitRepositories.clear();

        for (auto &p : std::filesystem::recursive_directory_iterator(rootPath))
        {
            if (!p.is_directory())
            {
                continue;
            }
            
            if(p.path().string().find(".git") != std::string::npos 
            && p.path().string().find(".git/") == std::string::npos 
            && p.path().string().find(".github") == std::string::npos
            ){
                
                std::string tempPath = p.path().string();
                size_t lastPos = tempPath.find_last_of('/');
                tempPath = tempPath.substr(0, std::min(std::max(1, (int)lastPos), 10000));
                GitRepository* gitRepository = new GitRepository(tempPath);
                gitRepositories.push_back(gitRepository);
            }
            

        }


        
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

    static void LogCommitData(){
        std::string ignoredFilesString = "";

        for(auto ignoredFile : ignoredFiles){
            ignoredFilesString.append("':(exclude)" + ignoredFile + "' ");
        }

        allChanges.clear();
        allChanges.push_back(0);
        allChanges.push_back(0);
        allChanges.push_back(0);

        for(auto repo : gitRepositories){
            if(!repo->IsSelected()){
                continue;
            }


            //output format 
            // files changed, lines inserted, lines deleted

            std::string command = "cd " + repo->GetPath() + " && git log --shortstat --no-merges --author \"Alex Davies\" -p -- . " + ignoredFilesString + " | grep \"files changed\" | awk '{files+=$1; inserted+=$4; deleted+=$6} END {print files,\",\",inserted,\",\",deleted}'";
            std::string output = Command(command);
            
            std::vector<int> changes = {};

            size_t pos = 0;

            std::string::iterator endPos = std::remove(output.begin(), output.end(), ' ');
            output.erase(endPos, output.end());
            logger->Log(output);

            if(output.length() < 5) {
                continue;
            }

            while ((pos = output.find(',')) != std::string::npos) {
                changes.push_back(std::stoi(output.substr(0, pos)));
                output.erase(0, pos + 1);
            }
            changes.push_back(std::stoi(output));

            allChanges[0] += changes[0];
            allChanges[1] += changes[1];
            allChanges[2] += changes[2];

            repo->SetChanges(changes);

            
            
        }
    }

    static void LogUncommitedChanges(){
        std::string ignoredFilesString = "";

        for(auto ignoredFile : ignoredFiles){
            ignoredFilesString.append("':(exclude)" + ignoredFile + "' ");
        }

        for(auto repo : gitRepositories){
            if(!repo->IsSelected()){
                continue;
            }

            std::string command = "cd " + repo->GetPath() + " && git status -suno -- . " + ignoredFilesString;
            repo->SetUncommitedChanges(Command(command));
        }
    }

    void SetUncommitedChanges(std::string uncommitedChanges){
        this->uncommitedChanges = uncommitedChanges;
    }

    std::string GetUncommitedChanges(){
        return this->uncommitedChanges;
    }

    void SetChanges(std::vector<int> changes){
        this->changes = changes;
    }

    std::vector<int> GetChanges(){
        return changes;
    }

    static std::vector<int> GetAllChanges(){
        return allChanges;
    }

    static std::vector<GitRepository*> GetRepositories(){
        return gitRepositories;
    }

    static void SelectAll(){
        for(auto gitRepository : gitRepositories){
            gitRepository->selected = true;
        }
    }

    static void DeselectAll(){
        for(auto gitRepository : gitRepositories){
            gitRepository->selected = false;
        }
    }

    bool IsSelected()
    {
        return selected;
    }
    void Select(){
        selected = true;
    }
    void Deselect(){
       selected = false;
    }


    static void Save()
    {
        logger->Log("Saving Git Data");
        nlohmann::json wholeJson;
        wholeJson['gitUsername'] = 'cheese gamer';

        std::string path = GetExePath();
        // remove exe from path
        path = path.substr(0, path.find_last_of('/'));
        path += "/git.json";
        std::ofstream o(path);
        o << std::setw(4) << wholeJson << std::endl;
        o.close();
    }
    static void Load(Logger *loggerIn)
    {
        logger = loggerIn;

        /*for (auto devilboxInstallation : (devilboxInstallations))
        {
            delete devilboxInstallation;
        }
        (devilboxInstallations).clear();
        selectedInstallation = nullptr; // set to null, dont delete as that installation needs to persist

        std::string path = GetExePath();
        // remove exe from path
        path = path.substr(0, path.find_last_of('/'));
        path += "/devilboxInstallations.json";
        logger->Log(path);
        std::ifstream i(path);
        if (!i.is_open())
        {
            return;
        }

        logger->Log("Save file loaded");

        nlohmann::json wholeJson;
        wholeJson << i;
        !wholeJson["devilboxInstallations"].is_null(); // this is a hack to create the desired json classes if they do not already exist

        i.close();
        for (auto json : wholeJson["devilboxInstallations"])
        {
            !json["alias"].is_null();
            !json["path"].is_null();
            DevilboxInstallation *devilboxInstallation = new DevilboxInstallation(json["alias"], json["path"]);
            devilboxInstallation->GetPhpVersion();
            !json["presets"].is_null(); // this is a hack to create the desired json classes if they do not already exist
            for(auto presetJson : json["presets"]){
                !presetJson["name"].is_null();
                !presetJson["phpVersion"].is_null();
                InstallationPreset *temp = new InstallationPreset(presetJson["name"], presetJson["phpVersion"], "newPresetSqlVersion");
                (*(devilboxInstallation->getPresetsPtr())).push_back(temp);
            }
            

            devilboxInstallations.push_back(devilboxInstallation);
        }*/
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

};
Logger* GitRepository::logger = nullptr;
std::string GitRepository::rootPath = "";
std::vector<GitRepository*> GitRepository::gitRepositories = {};
std::vector<std::string> GitRepository::ignoredFiles = {"imgui", "code-review", "dist", "npm-shrinkwrap.json", ".gitignore", ".prettierignore", "package-lock.json", "package.json", "gist.sh", "gulpfile.js", ".prettierrc", ".editorconfig", "webpack.config.js", "*/vendor/*", "composer.json", "composer.lock", "*.js.map", "*.css.map", "*.LICENSE.txt", "*/plugins/*", ".DS_Store"};
std::vector<int> GitRepository::allChanges = {0, 0, 0};