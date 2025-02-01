#ifdef _WIN32
#include <windows.h> //GetModuleFileNameW
#elif __APPLE__
#include <mach-o/dyld.h>
#elif __linux__
#include <limits.h>
#include <unistd.h> //readlink
#endif
#include "../json/single_include/nlohmann/json.hpp"

class Pastable {
    std::string name;
    std::string pastableText;
    std::vector<std::string> tags;
    std::string jsonoutin = "";
    static Logger* logger;
public:
    Pastable(std::string name, std::string pastableText, std::vector<std::string> tags){
        this->name = name;
        this->pastableText = pastableText;
        this->tags = tags;

    }

    std::string* GetNamePtr(){
        return &name;
    }
    std::string* GetPastableTextPtr(){
        return &pastableText;
    }
    std::vector<std::string>* GetTagsPtr(){
        return &tags;
    }
    static void Save(std::vector<Pastable*> pastables,  std::vector<std::string> allPastableTags){
        
        nlohmann::json wholeJson;
        for(auto pastable : pastables){
            nlohmann::json json;
            json["name"] = *(pastable->GetNamePtr());
            json["pastableText"] = *(pastable->GetPastableTextPtr());
            json["tags"] = *(pastable->GetTagsPtr());

            wholeJson["pastables"].push_back(json);
            
        }
        wholeJson["allPastableTags"] = allPastableTags;

        std::string path = GetExePath();
        // remove exe from path
        path = path.substr(0, path.find_last_of('/'));
        path += "/pastables.json";
        std::ofstream o(path);
        o << std::setw(4) << wholeJson << std::endl;
        o.close();
    }
    static void Load(std::vector<Pastable*>* pastables, std::vector<std::string>* allPastableTags, Logger* loggerIn){
        logger = loggerIn;
        
        for(auto& pastable : (*pastables)){
            delete pastable;
        }
        (*pastables).clear();

        std::string path = GetExePath();
        // remove exe from path
        path = path.substr(0, path.find_last_of('/'));
        path += "/pastables.json";
        std::ifstream i(path);
        if(!i.is_open()) {
            return;
        }
        nlohmann::json wholeJson;
        wholeJson << i;
        !wholeJson["pastables"].is_null(); // this is a hack to create the desired json classes if they do not already exist 
        !wholeJson["allPastableTags"].is_null();
        
        i.close();
        for(auto json : wholeJson["pastables"]){
            Pastable* pastable = new Pastable(json["name"], json["pastableText"], json["tags"]);

            (*pastables).push_back(pastable);
        }
        (*allPastableTags) = wholeJson["allPastableTags"];
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

Logger* Pastable::logger = nullptr;