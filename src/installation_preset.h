class InstallationPreset{
    std::string name;
    std::string phpVersion;
    std::string sqlVersion;
    public:
    InstallationPreset(std::string name, std::string phpVersion, std::string sqlVersion){
        this->name = name;
        this->phpVersion = phpVersion;
        this->sqlVersion = sqlVersion;
    }

    std::string* GetNamePtr(){
        return &name;
    }

    std::string* GetPhpVersionPtr(){
        return &phpVersion;
    }

    std::string* GetSqlVersionPtr(){
        return &sqlVersion;
    }
};