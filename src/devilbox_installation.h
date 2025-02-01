#pragma once
#ifdef _WIN32
#include <windows.h> //GetModuleFileNameW
#elif __APPLE__
#include <mach-o/dyld.h>
#elif __linux__
#include <limits.h>
#include <unistd.h> //readlink
#endif

#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include "logger.h"

#include "../json/single_include/nlohmann/json.hpp"
#include "installation_preset.h"

class DevilboxInstallation
{
    std::string alias;
    std::string path;
    std::string phpVersion;
    static Logger *logger;
    std::string defaultPhpVersion = "8.0";

    std::string newPresetName = "";
    std::string newPresetPhpVersion = "";
    std::string newPresetSqlVersion = "";

    std::vector<InstallationPreset *> installationPresets = {};

    

public:
    DevilboxInstallation(std::string alias, std::string path)
    {
        this->alias = alias;
        this->path = path;
    }

    ~DevilboxInstallation(){
        for (auto preset : installationPresets)
        {
            delete preset;
        }
        installationPresets.clear();
    }

    void AddNewInstallationPreset()
    {
        if (newPresetName == "")
        {
            return;
        }
        if (newPresetPhpVersion == "")
        {
            return;
        }
        /*if(newPresetSqlVersion == ""){
            return;
        }*/

        auto isNameEqual = [this](InstallationPreset *preset)
        { return *(preset->GetNamePtr()) == newPresetName; };

        if (std::find_if(installationPresets.begin(), installationPresets.end(), isNameEqual) != installationPresets.end())
        {
            return;
        }

        InstallationPreset *temp = new InstallationPreset(newPresetName, newPresetPhpVersion, newPresetSqlVersion);
        installationPresets.push_back(temp);
        newPresetName = "";
        newPresetPhpVersion = "";
        newPresetSqlVersion = "";
    }

    bool GetPhpVersion()
    {
        logger->Log("Getting php version for: " + alias);
        std::ifstream outFile;
        outFile.open(std::string(this->path) + "/.env");
        if (!outFile.is_open())
        { // check for and open the .env file of devilbox
            logger->Log("Could not open the .env file of the Devilbox installation.");
            return false;
        }

        while (!outFile.eof())
        {
            std::string line;
            std::getline(outFile, line);
            if (line.find("PHP_SERVER") != std::string::npos && line.find("#") == std::string::npos)
            {

                auto found = line.find("=");
                if (found == std::string::npos)
                {
                    logger->Log("Malformed Devilbox .env file. Repairing with default PHP version (8.0).");

                    outFile.close();
                    if (SetPhpVersion(defaultPhpVersion))
                    {
                        return GetPhpVersion();
                    }
                    else
                    {
                        return false;
                    }
                    // return false;
                }
                // todo - add a check for if this value is in the list of versions

                phpVersion = line.substr(found + 1, line.length() - found);
                logger->Log("Found PHP version: " + phpVersion);
                break;
            }
        }
        outFile.close();

        return true;
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

    static void Start()
    {
        std::string output = Command("docker ps --filter \"name=devilbox\"");

        if (output.find("daemon") != std::string::npos)
        {
            logger->Log("Please open docker first");
        }
        else
        {
            if (output.find("devilbox") != std::string::npos)
            { // check if devilbox is already running
                for (auto installation : devilboxInstallations)
                {
                    logger->Log(Command(("cd " + *(installation->getPathPtr()) + " && docker-compose down").c_str())); // kill all of the devilboxes
                }
            }

            logger->Log(Command(("cd " + *(selectedInstallation->getPathPtr()) + " && docker-compose up -d").c_str())); // start
            logger->Log("Started Devilbox installation " + *(selectedInstallation->getAliasPtr()));
        }
    }

    static void StopAll()
    {
        std::string output = Command("docker ps --filter \"name=devilbox\"");

        if (output.find("daemon") != std::string::npos)
        {
            logger->Log("Please open docker first");
        }
        else
        {
            if (output.find("devilbox") != std::string::npos)
            { // check if devilbox is already running
                for (auto installation : devilboxInstallations)
                {
                    logger->Log(Command(("cd " + *(installation->getPathPtr()) + " && docker-compose down").c_str())); // kill all of the devilboxes
                    logger->Log("Attempted to kill Devilbox installation " + *(installation->getAliasPtr()));
                }
            }
            else
            {
                logger->Log("All Devilboxes are already killed");
            }
        }
    }

    bool SetPhpVersion(std::string newPHPVersion)
    {
        
        this->logger->Log(Command(("cd " + path + " && docker-compose down").c_str())); // kill all of the devilboxes and log it
        
        logger->Log("Setting PHP version to: " + newPHPVersion);
        std::fstream originalFile;
        originalFile.open(std::string(this->path) + "/.env");
        if (!originalFile.is_open())
        { // check for and open the .env file of devilbox
            logger->Log("Could not open the.env file of the Devilbox installation.");
            return false;
        }

        std::vector<std::string> lines;
        std::string input;
        while (std::getline(originalFile, input)) // copy the whole file to memory
        {
            lines.push_back(input);
        }
        originalFile.close();

        // std::remove(std::string(this->path + "/.env_backup").c_str()); // remove the old backup file
        if (std::rename(std::string(this->path + "/.env").c_str(), std::string(this->path + "/.env_backup").c_str()) != 0)
        {
            logger->Log("Could not rename the .env file of the Devilbox installation for backup.");
            return false;
        } // rename the current file to backup

        
        std::vector<std::string> linesBackup;
        std::copy(lines.begin(), lines.end(),std::back_inserter(linesBackup));
        bool oldVersionSet = false;
        bool newVersionSet = false;
        for (auto &line : lines)
        {
            // Found a line that contains/starts with "PHP_SERVER=" , didn't find a # and didn't find the php version we want to set,  
            if (line.find("PHP_SERVER") != std::string::npos && line.find("#") == std::string::npos && line.find(newPHPVersion) == std::string::npos)
            {
                // block off the old version line cos we didnt find the version but this one is on
                oldVersionSet = true;
                line = "#" + line;            
            }
            // Found a line that contains/starts with "PHP_SERVER=" and found the php version we want to set
            if (line.find("PHP_SERVER")!= std::string::npos && line.find("#") == std::string::npos && line.find(newPHPVersion)!= std::string::npos)
            {
                // this is both the old version and the new version because we have set it to the same one !!! oh no
                newVersionSet = true;
                oldVersionSet = true;
                continue;
            }
            
            if (line.find("PHP_SERVER") != std::string::npos && line.find(newPHPVersion) != std::string::npos)
            {
                newVersionSet = true;
                line = "PHP_SERVER=" + newPHPVersion;
                continue;
            }
        }
        if (!oldVersionSet || !newVersionSet)
        {
            logger->Log("Could not set the PHP version of the Devilbox installation.");
            lines = linesBackup;
        }

        std::ofstream outputFile(std::string(this->path) + "/.env");
        if (!outputFile.is_open())
        {
            logger->Log("Could not open the.env file of the Devilbox installation. Please change .env_backup to .env in the devilbox installation.");
            return false;
        }

        for (auto const &line : lines)
            outputFile << line << '\n';

        outputFile.close();
        logger->Log("Successfully changed the php version to: " + newPHPVersion);

        phpVersion = newPHPVersion;
        return true;
    }

    static void Save()
    {
        logger->Log("Saving Devilbox installations.");
        nlohmann::json wholeJson;
        for (auto installation : devilboxInstallations)
        {
            nlohmann::json installationJson;
            installationJson["alias"] = *(installation->getAliasPtr());
            installationJson["path"] = *(installation->getPathPtr());

            for(auto preset : *(installation->getPresetsPtr())){
                nlohmann::json presetJson;
                presetJson["name"] = *(preset->GetNamePtr());
                presetJson["phpVersion"] = *(preset->GetPhpVersionPtr());
                installationJson["presets"].push_back(presetJson);
            }

            wholeJson["devilboxInstallations"].push_back(installationJson);
        }

        std::string path = GetExePath();
        // remove exe from path
        path = path.substr(0, path.find_last_of('/'));
        path += "/devilboxInstallations.json";
        std::ofstream o(path);
        o << std::setw(4) << wholeJson << std::endl;
        o.close();
    }
    static void Load(Logger *loggerIn)
    {
        logger = loggerIn;

        for (auto devilboxInstallation : (devilboxInstallations))
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
        }
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

    std::string *getAliasPtr() { return &alias; }
    std::string *getPathPtr() { return &path; }
    std::string *getPhpVersionPtr() { return &phpVersion; }
    std::string *getNewPresetNamePtr() { return &newPresetName; }
    std::string *getNewPresetPhpVersionPtr() { return &newPresetPhpVersion; }
    std::string *getNewPresetSqlVersionPtr() { return &newPresetSqlVersion; }
    std::vector<InstallationPreset *> *getPresetsPtr() { return &installationPresets; }

    static std::vector<DevilboxInstallation*> devilboxInstallations;
    static DevilboxInstallation* selectedInstallation;
};

Logger *DevilboxInstallation::logger = nullptr;
std::vector<DevilboxInstallation *> DevilboxInstallation::devilboxInstallations = {};
DevilboxInstallation* DevilboxInstallation::selectedInstallation = nullptr;