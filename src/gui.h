#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <string>
#include <vector>
#include "imgui_stdlib.h"
 
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

class Logger;
class DevilboxInstallation;
class Template;
class Pastable;
class GitRepository;

class GUI{
    GLFWwindow* window = nullptr;
    Logger* logger = nullptr;

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
 
    const static int numberOfPhpVersions = 13;
    const std::string phpVersions[numberOfPhpVersions] = {"5.2", "5.3", "5.4", "5.5", "5.6", "7.0", "7.1", "7.2", "7.3", "7.4", "8.0", "8.1", "8.2"};

    

    std::vector<Template*> templates = {};
    Template* selectedTemplate = nullptr;

    
    std::vector<std::string> allPastableTags = {};

    std::string newPastableName = "";
    std::string newPastableText = "";
    std::vector<std::string> newPastableTags = {};
    std::vector<Pastable*> pastables = {};

    std::string newPastableTag = "";

    std::string pastableSearchInput = "";
    std::vector<std::string> pastableSearchTags = {};
    std::vector<Pastable*> pastablesSearchResult = {};

    

public:
    GUI();
    ~GUI();
    void Render();
    void RenderLoop();
    void InstallationDropdown();
    void AddInstallationButton();
    void NewInstallationPresetPHPVersionDropdown();
    void NewInstallationPresetSQLVersionDropdown();
    void AddNewInstallationPresetButton();
    void ProjectPresetsTree();
    void ProjectPresets();
    void StartDevilboxButton();
    void KillAllDevilboxesButton();
    void RemoveInstallationButton();
    void AliasTextInput();
    void TemplatesDropdown();
    void TemplatesInputs();
    void OutputDirectoryButton();
    void OutputFileNameTextInput();
    void GenerateFilesButton();
    void NewPastablesNameTextInput();
    void NewPastablesTextInput();
    void PastablesTags(std::string name, std::vector<std::string>* output, bool searchCallback = false);
    void AddNewPastableButton();
    void NewPastablesTagTextInput();
    void AddNewPastableTag();
    void RemovePastableTag();
    void PastablesSearchInput();
    void PastablesSearch(char* buf = nullptr);
    void Pastables();
    void GitRootPath();
    void SetGitRootPathButton();
    void SetGitRootPathAndSearchButton();
    void GetGitRepositoriesButton();
    void GitRepoButtons();
    void LogGitCommitDataButton();
    void ShowGitCommitData();
    void LogGitUncommittedChangesButton();
    void ShowGitUncommittedChanges();
    void Save();
    void Load();
    void MainMenu();
    void EnterEncryptionKey();
    void CreateFromTemplates();
    void PHPVersionDropdown();
    void NewInstallationPresetNameTextInput();
    static void GLFWErrorCallback(int error, const char *description);
};  

class PastablesCallbacks{
        static GUI* gui;
    public:
        static void SetGuiPointer(GUI* guiPtr){
            PastablesCallbacks::gui = guiPtr;
        }
        static int SearchCallback(ImGuiInputTextCallbackData* data){
            if(data->EventFlag == ImGuiInputTextFlags_CallbackEdit && PastablesCallbacks::gui)
			    PastablesCallbacks::gui->PastablesSearch(data->Buf);
		    return 0;
        }
    };
//extern GUI* PastablesCallbacks::gui;

