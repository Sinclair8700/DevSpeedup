#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>

#include <ctime>  
#include "logger.h"

class Logger
{
    std::string outputFile;
    std::vector<std::string> logs;
public:
    Logger(std::string outputFile){
        this->outputFile = outputFile;
    }

    std::vector<std::string> GetLogs(){
        return logs;
    }

    void Log(std::string message){
        if(message.length() == 0){
            return;
        }

        auto time = std::chrono::system_clock::now();
        std::time_t end_time = std::chrono::system_clock::to_time_t(time);
        std::string date_time = std::ctime(&end_time);
        date_time = date_time.substr(0, date_time.size()-1);
        std::string alteredMessage = "Log <" + date_time + "> : " + message;


        std::cout << alteredMessage << std::endl;

        if(outputFile != ""){
            std::ofstream file;
            file.open(outputFile.c_str(), std::ios::app);
            file << alteredMessage << std::endl;
            file.close();
        }
        logs.push_back(alteredMessage);
    }
};
