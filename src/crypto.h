#pragma once
#include "../cryptopp890/cryptlib.h"
#include "../cryptopp890/rijndael.h"
#include "../cryptopp890/modes.h"
#include "../cryptopp890/files.h"
#include "../cryptopp890/osrng.h"
#include "../cryptopp890/hex.h"
#include "../cryptopp890/eax.h"
#include "../cryptopp890/secblock.h"
#include "../json/single_include/nlohmann/json.hpp"
#include <sstream>
#include <iomanip>
#include <string>

class Crypto{

    static std::string key;
    static bool keySet;
    static CryptoPP::SecByteBlock iv;

    public:
        static std::string* GetKeyPtr(){
            return &key;
        }
        static bool HasKey(){
            return keySet;
        }
        static void SetKey(){
            //Load();
            std::cout << "yoooo we loadin an savin" << std::endl;
            keySet = true;
            /*if(iv.empty()){
                CryptoPP::AutoSeededRandomPool rng;
                rng.GenerateBlock( iv, sizeof(iv) );
            }*/
            Save();
        }

        static std::string Encrypt(std::string data){
            std::string cipher;

            try
            {
                CryptoPP::SecByteBlock keyObj(reinterpret_cast<const CryptoPP::byte*>(&key[0]), key.size());

                CryptoPP::EAX< CryptoPP::AES >::Encryption e;
                e.SetKeyWithIV(keyObj, keyObj.size(), iv, sizeof(iv));


                CryptoPP::StringSource(data, true, 
                    new CryptoPP::AuthenticatedEncryptionFilter(e,
                        new CryptoPP::StringSink(cipher)
                    ) // AuthenticatedEncryptionFilter
                ); // StringSource
            }
            catch(const CryptoPP::Exception& e)
            {
                std::cerr << e.what() << std::endl;
                exit(1);
            }

            return cipher;
        }
        static std::string Decrypt(std::string data){

            std::string recovered;

            try
            {
                CryptoPP::SecByteBlock keyObj(reinterpret_cast<const CryptoPP::byte*>(&key[0]), key.size());

                CryptoPP::EAX< CryptoPP::AES >::Decryption d;
                d.SetKeyWithIV(keyObj, keyObj.size(), iv, sizeof(iv));

                CryptoPP::StringSource s(data, true, 
                    new CryptoPP::AuthenticatedDecryptionFilter(d,
                        new CryptoPP::StringSink(recovered)
                    ) // AuthenticatedDecryptionFilter
                ); // StringSource
            }
            catch(const CryptoPP::Exception& e)
            {
                std::cerr << e.what() << std::endl;
                exit(1);
            }

            return recovered;
        }

    static void Save()
    {
        //logger->Log("Saving Git Data");
        nlohmann::json wholeJson;
        wholeJson['iv'] = "Cheese ahhh, googoo aahhh bitch";//SecByteBlockToString(iv);

        std::string path = GetExePath();
        // remove exe from path
        path = path.substr(0, path.find_last_of('/'));
        path += "/encryption.json";
        std::ofstream o(path);
        o << std::setw(4) << wholeJson << std::endl;
        o.close();
    }
    static void Load()
    {
        std::string path = GetExePath();
        // remove exe from path
        path = path.substr(0, path.find_last_of('/'));
        path += "/encryption.json";
        //logger->Log(path);
        std::ifstream i(path);
        if (!i.is_open())
        {
            return;
        }

        //logger->Log("Save file loaded");

        nlohmann::json wholeJson;
        wholeJson << i;
        !wholeJson["iv"].is_null(); // this is a hack to create the desired json classes if they do not already exist

        i.close();
        if(!wholeJson["iv"].is_null()){
            iv = StringToSecByteBlock(wholeJson["iv"]);
        }

        //key = wholeJson["key"];
    }

    static CryptoPP::SecByteBlock StringToSecByteBlock(const std::string& str) {
        CryptoPP::SecByteBlock block(reinterpret_cast<const CryptoPP::byte*>(str.data()), str.size());
        return block;
    }

    static std::string SecByteBlockToString(const CryptoPP::SecByteBlock& block) {
        std::string str(reinterpret_cast<const char*>(block.data()), block.size());
        return str;
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

std::string Crypto::key = "";
bool Crypto::keySet = false;
CryptoPP::SecByteBlock Crypto::iv(CryptoPP::AES::BLOCKSIZE);