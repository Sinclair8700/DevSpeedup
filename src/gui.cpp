#include "gui.h"
#include <nfd.h>
#include <stdlib.h>
#include <array>
#include <filesystem>
#include "devilbox_installation.h"

#include "template.h"
#include "pastable.h"
#include "git.h"
#include "crypto.h"

GUI *PastablesCallbacks::gui = nullptr;

GUI::GUI()
{
    // Setup window
    glfwSetErrorCallback(GLFWErrorCallback);
    if (glfwInit())
    {
        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100
        const char *glsl_version = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
        // GL 3.2 + GLSL 150
        const char *glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // Required on Mac
#else
        // GL 3.0 + GLSL 130
        const char *glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

        // Create window with graphics context
        this->window = glfwCreateWindow(960, 540, "DevSpeedup", NULL, NULL);
        if (this->window != NULL)
        {
            PastablesCallbacks::SetGuiPointer(this);
            this->logger = new Logger("./logs.txt");
            GitRepository::SetLogger(this->logger);
            Load();

            glfwMakeContextCurrent(this->window);
            glfwSwapInterval(1); // Enable vsync

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO &io = ImGui::GetIO();
            (void)io;
            // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();
            // ImGui::StyleColorsLight();

            // Setup Platform/Renderer backends
            ImGui_ImplGlfw_InitForOpenGL(this->window, true);
            ImGui_ImplOpenGL3_Init(glsl_version);

            // Load Fonts
            // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
            // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
            // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
            // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
            // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
            // - Read 'docs/FONTS.md' for more instructions and details.
            // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
            // io.Fonts->AddFontDefault();
            // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
            // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
            // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
            // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
            // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
            // IM_ASSERT(font != NULL);
        }
    }
}

void GUI::RenderLoop()
{

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        ImGui::SetNextWindowSize(ImVec2(width, height)); // ensures ImGui fits the GLFW window
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        Render();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

void GUI::InstallationDropdown()
{
    if (!DevilboxInstallation::devilboxInstallations.size())
    {
        return;
    }

    if (ImGui::BeginCombo("Select Installation##Select Installation Combo", (DevilboxInstallation::selectedInstallation ? (*(DevilboxInstallation::selectedInstallation->getAliasPtr())).c_str() : "None selected")))
    {
        for (int n = 0; n < DevilboxInstallation::devilboxInstallations.size(); n++)
        {
            bool is_selected = (DevilboxInstallation::selectedInstallation == DevilboxInstallation::devilboxInstallations[n]); // You can store your selection however you want, outside or inside your objects
            if (ImGui::Selectable((*(DevilboxInstallation::devilboxInstallations[n]->getAliasPtr())).c_str(), is_selected))
            {
                DevilboxInstallation::selectedInstallation = DevilboxInstallation::devilboxInstallations[n];
                this->logger->Log("Selected installation: " + (*(DevilboxInstallation::devilboxInstallations[n]->getAliasPtr())));
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
        }

        ImGui::EndCombo();
    }
}

void GUI::AddInstallationButton()
{
    static int clickTime = 0;
    if (ImGui::Button("Add a devilbox installation"))
    {
        nfdchar_t *outPath = NULL;
        nfdresult_t result = NFD_PickFolder(NULL, &outPath);

        if (result == NFD_OKAY) // we opened a folder successfully
        {

            DevilboxInstallation *newInstallation = new DevilboxInstallation(outPath, outPath);
            if (newInstallation->GetPhpVersion())
            { // we found the env folder and php version
                this->logger->Log("PHP version found");
                auto isPathEqual = [newInstallation](DevilboxInstallation *installation)
                { return *(installation->getPathPtr()) == *(newInstallation->getPathPtr()); };
                if (std::find_if(DevilboxInstallation::devilboxInstallations.begin(), DevilboxInstallation::devilboxInstallations.end(), isPathEqual) == DevilboxInstallation::devilboxInstallations.end())
                {
                    this->logger->Log("Added Devilbox installation at: \"" + std::string(outPath) + "\"");
                    DevilboxInstallation::devilboxInstallations.push_back(newInstallation);
                    DevilboxInstallation::selectedInstallation = newInstallation;
                }
                else
                {
                    delete newInstallation;
                }
            }
            else
            {
                delete newInstallation;
            }

            free(outPath);
        }
        else if (result == NFD_CANCEL)
        {
            this->logger->Log("Cancelled adding Devilbox installation");
        }
        else
        {
            // printf("Error: %s\n", NFD_GetError());
            this->logger->Log("Error when adding Devilbox installation: " + std::string(NFD_GetError()));
        }
    }
}

void GUI::PHPVersionDropdown()
{
    if (!DevilboxInstallation::devilboxInstallations.size() || !DevilboxInstallation::selectedInstallation)
    {
        return;
    }

    if (ImGui::BeginCombo("Select PHP Version##Select PHP Combo", (DevilboxInstallation::selectedInstallation->getPhpVersionPtr() ? (*(DevilboxInstallation::selectedInstallation->getPhpVersionPtr())).c_str() : "None selected")))
    {
        for (int n = 0; n < numberOfPhpVersions; n++)
        {
            bool is_selected = (*(DevilboxInstallation::selectedInstallation->getPhpVersionPtr()) == phpVersions[n]); // You can store your selection however you want, outside or inside your objects
            if (ImGui::Selectable((phpVersions[n]).c_str(), is_selected))
            {
                DevilboxInstallation::selectedInstallation->SetPhpVersion(phpVersions[n]);
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
        }

        ImGui::EndCombo();
    }
}

void GUI::NewInstallationPresetNameTextInput()
{

    ImGui::InputText("New preset name", DevilboxInstallation::selectedInstallation->getNewPresetNamePtr());
}

void GUI::NewInstallationPresetPHPVersionDropdown()
{
    if (!DevilboxInstallation::devilboxInstallations.size() || !DevilboxInstallation::selectedInstallation)
    {
        return;
    }

    if (ImGui::BeginCombo("New preset PHP version##New preset PHP version Combo", ((*(DevilboxInstallation::selectedInstallation->getNewPresetPhpVersionPtr())) != "" ? (*(DevilboxInstallation::selectedInstallation->getNewPresetPhpVersionPtr())).c_str() : "None selected")))
    {
        for (int n = 0; n < numberOfPhpVersions; n++)
        {
            bool is_selected = (*(DevilboxInstallation::selectedInstallation->getNewPresetPhpVersionPtr()) == phpVersions[n]); // You can store your selection however you want, outside or inside your objects
            if (ImGui::Selectable((phpVersions[n]).c_str(), is_selected))
            {
                *(DevilboxInstallation::selectedInstallation->getNewPresetPhpVersionPtr()) = phpVersions[n];
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
        }

        ImGui::EndCombo();
    }
}

void GUI::NewInstallationPresetSQLVersionDropdown()
{
    if (!DevilboxInstallation::devilboxInstallations.size() || !DevilboxInstallation::selectedInstallation)
    {
        return;
    }
}

void GUI::AddNewInstallationPresetButton()
{
    if (ImGui::Button("Add new installation preset"))
    {
        DevilboxInstallation::selectedInstallation->AddNewInstallationPreset();
    }
}

void GUI::ProjectPresetsTree()
{
    InstallationPreset *presetToDelete = nullptr;

    for (auto &preset : *(DevilboxInstallation::selectedInstallation->getPresetsPtr()))
    {
        // preset->GetNamePtr();
        auto tree = ImGui::TreeNodeEx(std::string(*(preset->GetNamePtr()) + "###" + std::to_string((long)preset->GetNamePtr())).c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);

        ImGui::SameLine();
        if (ImGui::Button(("Select##" + std::to_string((long)preset->GetNamePtr())).c_str()))
        {
            // set the installation to the selected preset
            DevilboxInstallation::selectedInstallation->SetPhpVersion(*(preset->GetPhpVersionPtr()));
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.f, 0.6f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(0.f, 0.8f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0.f, 0.7f, 0.f));
        if (ImGui::Button(("Select & Start##" + std::to_string((long)preset->GetNamePtr())).c_str()))
        {
            // set the installation to the selected preset
            DevilboxInstallation::selectedInstallation->SetPhpVersion(*(preset->GetPhpVersionPtr()));
            DevilboxInstallation::Start();
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.6f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(1.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0.8f, 0.f, 0.f));
        if (ImGui::Button(("Delete##" + std::to_string((long)preset->GetNamePtr())).c_str()))
        {
            // pastableToDelete = pastable;
            ImGui::OpenPopup(("DeletePresetPopup###" + std::to_string((long)preset->GetNamePtr())).c_str());
        }
        ImGui::PopStyleColor(3);

        if (ImGui::BeginPopup(("DeletePresetPopup###" + std::to_string((long)preset->GetNamePtr())).c_str()))
        {
            ImGui::Text(std::string("Are you sure you wish to delete preset: " + (*(preset->GetNamePtr()))).c_str());
            if (ImGui::Button(("Cancel##" + std::to_string((long)preset->GetNamePtr())).c_str()))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.6f, 0.f, 0.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(1.f, 0.f, 0.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0.8f, 0.f, 0.f));
            if (ImGui::Button(("Delete##2" + std::to_string((long)preset->GetNamePtr())).c_str()))
            {
                presetToDelete = preset;
            }
            ImGui::PopStyleColor(3);
            ImGui::EndPopup();
        }

        if (tree)
        {
            if (ImGui::BeginCombo(("Preset PHP version###" + std::to_string((long)preset->GetNamePtr())).c_str(), ((*(preset->GetPhpVersionPtr())) != "" ? (*(preset->GetPhpVersionPtr())).c_str() : "None selected")))
            {
                for (int n = 0; n < numberOfPhpVersions; n++)
                {
                    bool is_selected = (*(preset->GetPhpVersionPtr()) == phpVersions[n]); // You can store your selection however you want, outside or inside your objects
                    if (ImGui::Selectable((phpVersions[n]).c_str(), is_selected))
                    {
                        *(preset->GetPhpVersionPtr()) = phpVersions[n];
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }

                ImGui::EndCombo();
            }
            ImGui::TreePop();
        }

        if (presetToDelete)
        {
            auto iter = std::find((*(DevilboxInstallation::selectedInstallation->getPresetsPtr())).begin(), (*(DevilboxInstallation::selectedInstallation->getPresetsPtr())).end(), presetToDelete);
            if (iter != (*(DevilboxInstallation::selectedInstallation->getPresetsPtr())).end())
            {
                this->logger->Log("Removed preset: " + *(presetToDelete->GetNamePtr()));
                delete *iter;
                (*(DevilboxInstallation::selectedInstallation->getPresetsPtr())).erase(iter);
            }
        }
    }
}

void GUI::ProjectPresets()
{
    // New presets tree
    // New preset name

    // New preset php version

    // New preset sql version

    // Presets tree
    // for all presets of current devilbox installation
    // preset tree (use) (delete)
    // preset name
    // preset php version
    // preset sql version

    if (!DevilboxInstallation::devilboxInstallations.size() || !DevilboxInstallation::selectedInstallation)
    {
        return;
    }

    if (ImGui::TreeNode("Add a devilbox preset"))
    {
        NewInstallationPresetNameTextInput();
        NewInstallationPresetPHPVersionDropdown();

        // NewInstallationPresetSQLVersionDropdown();
        AddNewInstallationPresetButton();
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Presets", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ProjectPresetsTree();
        ImGui::TreePop();
    }
}

void GUI::StartDevilboxButton()
{
    if (!DevilboxInstallation::selectedInstallation || !DevilboxInstallation::devilboxInstallations.size())
    {
        return;
    }

    if (ImGui::Button("Start devilbox"))
    {

        DevilboxInstallation::Start();

        // system(("cd " + selectedInstallation->getPath() + "; docker-compose up -d; ./shell.sh"  ).c_str());
    }
}

void GUI::KillAllDevilboxesButton()
{
    if (!DevilboxInstallation::devilboxInstallations.size())
    {
        return;
    }

    if (ImGui::Button("Kill all devilboxes"))
    {

        DevilboxInstallation::StopAll();

        // system(("cd " + selectedInstallation->getPath() + "; docker-compose up -d; ./shell.sh"  ).c_str());
    }
}

void GUI::RemoveInstallationButton()
{
    if (!DevilboxInstallation::selectedInstallation || !DevilboxInstallation::devilboxInstallations.size())
    {
        return;
    }

    if (ImGui::Button("Remove installation"))
    {
        DevilboxInstallation::devilboxInstallations.erase(std::find(DevilboxInstallation::devilboxInstallations.begin(), DevilboxInstallation::devilboxInstallations.end(), DevilboxInstallation::selectedInstallation));
        this->logger->Log("Removed devilbox installation: " + *(DevilboxInstallation::selectedInstallation->getAliasPtr()));
        delete DevilboxInstallation::selectedInstallation;
        DevilboxInstallation::selectedInstallation = nullptr;
    }
}

void GUI::AliasTextInput()
{
    if (!DevilboxInstallation::selectedInstallation || !DevilboxInstallation::devilboxInstallations.size())
    {
        return;
    }

    ImGui::InputText("Alias##inputjsdifjs", DevilboxInstallation::selectedInstallation->getAliasPtr());
}

void GUI::TemplatesDropdown()
{
    if (templates.size() == 0)
    {
        templates = Template::GetAllTemplates(logger);
    }

    if (ImGui::BeginCombo("Select Template##Select Template Combo", (selectedTemplate ? (selectedTemplate->getAlias()).c_str() : "None selected")))
    {
        for (int n = 0; n < templates.size(); n++)
        {
            bool is_selected = (selectedTemplate == templates[n]); // You can store your selection however you want, outside or inside your objects
            if (ImGui::Selectable((templates[n]->getAlias()).c_str(), is_selected))
            {
                selectedTemplate = templates[n];
                this->logger->Log("Selected template: " + templates[n]->getAlias());
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus(); // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
        }

        ImGui::EndCombo();
    }
}

void GUI::TemplatesInputs()
{
    if (templates.size() == 0 || !selectedTemplate)
    {
        return;
    }
    for (auto replaceable : selectedTemplate->GetReplaceables())
    {

        std::string inputName = replaceable + "##" + replaceable;
        ImGui::InputText(inputName.c_str(), selectedTemplate->GetReplacedTextPtr(replaceable));
    }
}

void GUI::OutputDirectoryButton()
{
    if (templates.size() == 0 || !selectedTemplate)
    {
        return;
    }

    if (ImGui::Button(*(selectedTemplate->GetOutputDirectoryPtr()) != "" ? ("Output directory: " + *(selectedTemplate->GetOutputDirectoryPtr())).c_str() : "Select an output directory"))
    {
        nfdchar_t *outPath = NULL;
        nfdresult_t result = NFD_PickFolder(NULL, &outPath);

        if (result == NFD_OKAY) // we opened a folder successfully
        {

            *(selectedTemplate->GetOutputDirectoryPtr()) = outPath;
            this->logger->Log("Set output directory to: \"" + std::string(outPath) + "\"");
            free(outPath);
        }
        else if (result == NFD_CANCEL)
        {
            this->logger->Log("Cancelled adding template output directory");
        }
        else
        {
            this->logger->Log("Error when adding template output directory: " + std::string(NFD_GetError()));
        }
    }
}

void GUI::OutputFileNameTextInput()
{
    if (templates.size() == 0 || !selectedTemplate)
    {
        return;
    }
    ImGui::InputText("Output folder name##Output Folder Name Input", selectedTemplate->GetOutputFolderPtr());
}

void GUI::GenerateFilesButton()
{
    if (templates.size() == 0 || !selectedTemplate)
    {
        return;
    }

    if (ImGui::Button("Generate files"))
    {
        selectedTemplate->GenerateFiles();
    }
}

void GUI::NewPastablesNameTextInput()
{
    ImGui::InputText("New pastable name##New Pastable Name Input", &newPastableName);
}

void GUI::NewPastablesTextInput()
{
    ImGui::InputTextMultiline("New pastable text##New Pastable Text Input", &newPastableText /*, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16)*/);
}

void GUI::PastablesTags(std::string name, std::vector<std::string> *output, bool searchCallback)
{
    ImGuiStyle &style = ImGui::GetStyle();

    if (ImGui::TreeNode(name.c_str()))
    {
        ImVec2 ourDrawAreaMin = ImGui::GetWindowContentRegionMin();
        ourDrawAreaMin.x += style.IndentSpacing * 2.f;
        ImVec2 ourDrawAreaMax = ImGui::GetWindowContentRegionMax();
        int widthAvailable = ourDrawAreaMax.x - ourDrawAreaMin.x;
        int counter = 0;
        for (auto tag : allPastableTags)
        {
            auto iter = std::find((*output).begin(), (*output).end(), tag);
            bool changeColour = iter != (*output).end();
            if (changeColour)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonHovered]);
            }

            int widthOfButton = ImGui::CalcTextSize(tag.c_str()).x + style.FramePadding.x * 2;
            widthAvailable -= widthOfButton + style.ItemSpacing.x;
            // ImGui::GetWindowDrawList()->AddRect(ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax(), IM_COL32( 255, 255, 0, 255 ));

            // ImGui::GetWindowDrawList()->AddRect(ourDrawAreaMin, ourDrawAreaMax, IM_COL32( 255, 0, 0, 255 ));

            if (counter != 0 && counter < allPastableTags.size() && widthAvailable > 0)
            {
                ImGui::SameLine();
            }
            if (widthAvailable <= 0)
            {
                widthAvailable = ourDrawAreaMax.x - ourDrawAreaMin.x - (widthOfButton + style.ItemSpacing.x);
            }

            if (ImGui::Button(tag.c_str()))
            {
                if (iter == (*output).end())
                {
                    (*output).push_back(tag);
                    this->logger->Log("Added pastable tag: " + tag);
                }
                else
                {
                    (*output).erase(iter);
                    this->logger->Log("Removed pastable tag: " + tag);
                }
                if (searchCallback)
                {
                    PastablesSearch();
                }
            }

            counter++;

            if (changeColour)
            {
                ImGui::PopStyleColor();
            }
        }

        ImGui::TreePop();
    }
}

void GUI::AddNewPastableButton()
{
    if (ImGui::Button("Add new pastable##Add New Pastable"))
    {
        if (newPastableName == "" || newPastableText == "")
            return;

        Pastable *pastable = new Pastable(newPastableName, newPastableText, newPastableTags);
        this->logger->Log("Added new pastable: " + newPastableName);
        newPastableName = "";
        newPastableText = "";
        newPastableTags.clear();
        pastables.push_back(pastable);

        PastablesSearch();
    }
}

void GUI::NewPastablesTagTextInput()
{
    ImGui::InputText("New pastable tag##New Pastable Tag Input", &newPastableTag);
}
void GUI::AddNewPastableTag()
{

    if (ImGui::Button("Add new pastable tag##Add New Pastable Tag"))
    {
        if (newPastableTag == "")
        {
            return;
        }

        auto iter = std::find(allPastableTags.begin(), allPastableTags.end(), newPastableTag);
        if (iter == allPastableTags.end())
        {
            allPastableTags.push_back(newPastableTag);
            this->logger->Log("Added new pastable tag: " + newPastableTag);
            newPastableTag = "";
        }
    }
}

void GUI::RemovePastableTag()
{

    if (ImGui::Button("Remove pastable tag##Remove Pastable Tag"))
    {
        if (newPastableTag == "")
        {
            return;
        }

        auto iter = std::find(allPastableTags.begin(), allPastableTags.end(), newPastableTag);
        if (iter != allPastableTags.end())
        {
            allPastableTags.erase(iter);
            this->logger->Log("Removed pastable tag: " + newPastableTag);
            newPastableTag = "";
        }
    }
}

void GUI::PastablesSearchInput()
{
    ImGui::InputText("Pastable name", &pastableSearchInput, ImGuiInputTextFlags_CallbackEdit, PastablesCallbacks::SearchCallback);
}
/*void GUI::PastablesSearchTags()
{
    if (ImGui::TreeNode("Pastable tags##Pastable Tags Tree"))
    {
        for (auto tag : allPastableTags)
        {
            auto iter = std::find(pastableSearchTags.begin(), pastableSearchTags.end(), tag);
            bool isSelected = iter != pastableSearchTags.end();
            bool isSelectedBefore = isSelected;

            ImGui::Selectable(tag.c_str(), &isSelected);

            if (isSelectedBefore != isSelected)
            { // we have a change in state
                if (isSelected)
                {
                    pastableSearchTags.push_back(tag);
                }
                else
                {
                    pastableSearchTags.erase(iter);
                }
            }

            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::TreePop();
    }
}*/
void GUI::PastablesSearch(char *buf)
{

    std::string searchString;
    if (buf != nullptr)
    {
        searchString = std::string(buf);
    }
    else
    {
        searchString = pastableSearchInput;
    }

    pastablesSearchResult.clear();
    for (auto pastable : pastables)
    {
        if (searchString != "" && (*(pastable->GetNamePtr())).find(searchString) == std::string::npos)
        {
            continue;
        }
        if (pastableSearchTags.size() > 0)
        {
            bool allTagsMatch = true;
            for (auto searchTag : pastableSearchTags)
            {
                auto tagsPtr = pastable->GetTagsPtr();
                if (std::find((*tagsPtr).begin(), (*tagsPtr).end(), searchTag) == (*tagsPtr).end())
                { // tag is missing
                    allTagsMatch = false;
                    break;
                }
            }
            if (!allTagsMatch)
            {
                continue;
            }
        }
        pastablesSearchResult.push_back(pastable);
    }
}

void GUI::Pastables()
{
    Pastable *pastableToDelete = nullptr;
    for (auto pastable : pastablesSearchResult)
    {

        auto tree = ImGui::TreeNodeEx((*(pastable->GetNamePtr()) + "###" + std::to_string((long)pastable->GetNamePtr())).c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);

        ImGui::SameLine();
        if (ImGui::Button(("Copy##" + std::to_string((long)pastable->GetNamePtr())).c_str()))
        {
            ImGui::SetClipboardText((*(pastable->GetPastableTextPtr())).c_str());
        }
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.6f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(1.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0.8f, 0.f, 0.f));
        if (ImGui::Button(("Delete##" + std::to_string((long)pastable->GetNamePtr())).c_str()))
        {
            // pastableToDelete = pastable;
            ImGui::OpenPopup(("DeletePastablePopup###" + std::to_string((long)pastable->GetNamePtr())).c_str());
        }
        ImGui::PopStyleColor(3);

        if (ImGui::BeginPopup(("DeletePastablePopup###" + std::to_string((long)pastable->GetNamePtr())).c_str()))
        {
            ImGui::Text(std::string("Are you sure you wish to delete pastable: " + (*(pastable->GetNamePtr()))).c_str());
            if (ImGui::Button(("Cancel##" + std::to_string((long)pastable->GetPastableTextPtr())).c_str()))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(0.6f, 0.f, 0.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor(1.f, 0.f, 0.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor(0.8f, 0.f, 0.f));
            if (ImGui::Button(("Delete##" + std::to_string((long)pastable->GetPastableTextPtr())).c_str()))
            {
                pastableToDelete = pastable;
            }
            ImGui::PopStyleColor(3);
            ImGui::EndPopup();
        }

        if (tree)
        {

            ImGui::InputText("Pastable name##4444323", pastable->GetNamePtr());
            ImGui::InputTextMultiline("Pastable text##4444323", pastable->GetPastableTextPtr());
            PastablesTags("Pastable tags##fsbfbndddd", pastable->GetTagsPtr());

            ImGui::TreePop();
        }
    }
    if (pastableToDelete)
    {
        auto iter = std::find(pastables.begin(), pastables.end(), pastableToDelete);
        if (iter != pastables.end())
        {
            this->logger->Log("Removed pastable: " + *(pastableToDelete->GetNamePtr()));
            delete *iter;
            pastables.erase(iter);
        }
        // research
        PastablesSearch();
    }
}

void GUI::GitRootPath(){
    if(GitRepository::GetRootPath()!= ""){
        ImGui::Text(GitRepository::GetRootPath().c_str());
    }
    else{
        ImGui::Text("No git root path set");
    }
}
void GUI::SetGitRootPathButton(){
    if (ImGui::Button("Set git root path"))
    {
        nfdchar_t *outPath = NULL;
        nfdresult_t result = NFD_PickFolder(NULL, &outPath);

        if (result == NFD_OKAY) // we opened a folder successfully
        {

            GitRepository::SetRootPath(outPath);

            free(outPath);
        }
        else if (result == NFD_CANCEL)
        {
            this->logger->Log("Cancelled setting root path");
        }
        else
        {
            // printf("Error: %s\n", NFD_GetError());
            this->logger->Log("Error when setting root path: " + std::string(NFD_GetError()));
        }
    }
}
void GUI::SetGitRootPathAndSearchButton(){
    if (ImGui::Button("Set git root path and search"))
    {
        nfdchar_t *outPath = NULL;
        nfdresult_t result = NFD_PickFolder(NULL, &outPath);

        if (result == NFD_OKAY) // we opened a folder successfully
        {

            GitRepository::SetRootPathAndSearch(outPath);

            free(outPath);
        }
        else if (result == NFD_CANCEL)
        {
            this->logger->Log("Cancelled setting root path and searching");
        }
        else
        {
            // printf("Error: %s\n", NFD_GetError());
            this->logger->Log("Error when setting root path and searching: " + std::string(NFD_GetError()));
        }
    }
}
void GUI::GetGitRepositoriesButton(){


    if (ImGui::Button("Get git repositories")){
        GitRepository::Search();
    }
}
void GUI::GitRepoButtons(){
    ImGuiStyle &style = ImGui::GetStyle();
    bool tree = ImGui::TreeNodeEx("Repositories", ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
    ImGui::SameLine();
    if(ImGui::Button("Select all")){
        GitRepository::SelectAll();
    }
    ImGui::SameLine();
    if(ImGui::Button("Deselect all")){
        GitRepository::DeselectAll();
    }
    if (tree)
    {


        ImVec2 ourDrawAreaMin = ImGui::GetWindowContentRegionMin();
        ourDrawAreaMin.x += style.IndentSpacing * 2.f;
        ImVec2 ourDrawAreaMax = ImGui::GetWindowContentRegionMax();
        int widthAvailable = ourDrawAreaMax.x - ourDrawAreaMin.x;
        int counter = 0;

        std::vector<GitRepository*> repos = GitRepository::GetRepositories();

        for (auto repo : repos)
        {

            bool changeColour = repo->IsSelected();
            if (changeColour)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_ButtonHovered]);
            }

            int widthOfButton = ImGui::CalcTextSize(repo->GetName().c_str()).x + style.FramePadding.x * 2;
            widthAvailable -= widthOfButton + style.ItemSpacing.x;
            // ImGui::GetWindowDrawList()->AddRect(ImGui::GetWindowContentRegionMin(), ImGui::GetWindowContentRegionMax(), IM_COL32( 255, 255, 0, 255 ));

            // ImGui::GetWindowDrawList()->AddRect(ourDrawAreaMin, ourDrawAreaMax, IM_COL32( 255, 0, 0, 255 ));

            if (counter != 0 && counter < repos.size() && widthAvailable > 0)
            {
                ImGui::SameLine();
            }
            if (widthAvailable <= 0)
            {
                widthAvailable = ourDrawAreaMax.x - ourDrawAreaMin.x - (widthOfButton + style.ItemSpacing.x);
            }

            if (ImGui::Button(repo->GetName().c_str()))
            {
                if (!changeColour)
                {
                    repo->Select();
                }
                else
                {
                    repo->Deselect();
                }

            }

            counter++;

            if (changeColour)
            {
                ImGui::PopStyleColor();
            }
        }
        ImGui::TreePop();
    }
}

void GUI::LogGitCommitDataButton(){
    ImGuiStyle &style = ImGui::GetStyle();
    bool tree = ImGui::TreeNodeEx("Commit data", ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
    ImGui::SameLine();
    if(ImGui::Button("Log commit data")){
        GitRepository::LogCommitData();
    }
    if(tree){
        ShowGitCommitData();
        ImGui::TreePop();
    }
}

void GUI::ShowGitCommitData(){
    std::vector<int> allChanges = GitRepository::GetAllChanges();
    if(allChanges.size() == 0)
        return;

    std::vector<GitRepository*> repos = GitRepository::GetRepositories();

    for (auto repo : repos)
    {
        if(!repo->IsSelected())
        {
            continue;
        }

        std::vector<int> data = repo->GetChanges();
        if(data.size() == 0)
        {
            continue;
        }

        ImGui::Text(std::string(repo->GetName() + ": Files changed [" +  std::to_string(data[0]) + "] Lines inserted [" +  std::to_string(data[1]) + "] Lines deleted [" +  std::to_string(data[2]) + "]").c_str());

    }

    ImGui::Text(std::string("Total: Files changed [" + std::to_string(allChanges[0]) + "] Lines inserted [" +  std::to_string(allChanges[1]) + "] Lines deleted [" +  std::to_string(allChanges[2]) + "]").c_str());

}

void GUI::LogGitUncommittedChangesButton(){
    ImGuiStyle &style = ImGui::GetStyle();
    bool tree = ImGui::TreeNodeEx("Uncommitted changes", ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
    ImGui::SameLine();
    if(ImGui::Button("Log uncommited changes")){
        GitRepository::LogUncommitedChanges();
    }
    if(tree){
        ShowGitUncommittedChanges();
        ImGui::TreePop();
    }
}

void GUI::ShowGitUncommittedChanges(){

    std::vector<GitRepository*> repos = GitRepository::GetRepositories();

    for (auto repo : repos)
    {
        if(!repo->IsSelected() || repo->GetUncommitedChanges() == "")
        {
            continue;
        }

        bool tree = ImGui::TreeNodeEx(repo->GetName().c_str(), ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding);
        if(tree){
            ImGui::Text(repo->GetUncommitedChanges().c_str());
            ImGui::TreePop();
        }
    }
}

void GUI::Save()
{
    DevilboxInstallation::Save();
    Pastable::Save(pastables, allPastableTags);
}

void GUI::Load()
{
    DevilboxInstallation::Load(logger);
    this->logger->Log("Loaded Devilbox installations");

    Pastable::Load(&pastables, &allPastableTags, logger);
    this->logger->Log("Loaded pastables");

    PastablesSearch();
}

void GUI::MainMenu(){
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;

    if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Devilbox"))
        {
            AddInstallationButton();
            KillAllDevilboxesButton();
            InstallationDropdown();
            RemoveInstallationButton();
            StartDevilboxButton();
            AliasTextInput();
            PHPVersionDropdown();
            ProjectPresets();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Templates"))
        {

            TemplatesDropdown();
            OutputDirectoryButton();
            OutputFileNameTextInput();
            TemplatesInputs();
            GenerateFilesButton();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Pastables"))
        {
            if (ImGui::TreeNode("Add a new pastable##Add New Pastable Tree"))
            {

                NewPastablesNameTextInput();
                NewPastablesTextInput();
                PastablesTags("New pastable tags##dfijidjs", &newPastableTags);
                AddNewPastableButton();
                ImGui::TreePop();
            }
            ImGui::Separator();

            if (ImGui::TreeNode("Add/remove a pastable tag"))
            {
                NewPastablesTagTextInput();
                AddNewPastableTag();
                RemovePastableTag();
                ImGui::TreePop();
            }
            ImGui::Separator();

            if (ImGui::TreeNodeEx("Search for a pastable", ImGuiTreeNodeFlags_DefaultOpen))
            {
                PastablesSearchInput();
                PastablesTags("Pastable tags##fsdfgsfbfbndddd", &pastableSearchTags, true);
                // PastablesSearchButton();
                ImGui::TreePop();
            }
            ImGui::Separator();
            if (ImGui::BeginChild("Scrolling Log"))
            {
                Pastables();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Git"))
        {
            GitRootPath();
            SetGitRootPathButton();
            SetGitRootPathAndSearchButton();
            GetGitRepositoriesButton();
            GitRepoButtons();
            LogGitCommitDataButton();
            LogGitUncommittedChangesButton();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Logs"))
        {
            if (ImGui::BeginChild("Logs"))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

                for (auto log : logger->GetLogs())
                {
                    ImGui::Text(log.c_str());
                }

                ImGui::PopStyleVar();
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::TabItemButton("Save"))
        {
            Save();
        }

        if (ImGui::TabItemButton("Load"))
        {
            Load();
        }

        ImGui::EndTabBar();
    }
}

void GUI::EnterEncryptionKey(){
    ImGui::InputText("Encryption key", Crypto::GetKeyPtr());
    if (ImGui::Button("Enter")){
        Crypto::SetKey();
    }
}

void GUI::Render()
{
    ImGuiStyle &style = ImGui::GetStyle();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    // if (show_demo_window)
    // ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {

        ImGuiWindowFlags windowFlags = 0;
        windowFlags |= ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("DevSpeedup", NULL, windowFlags); // Create a window called "Hello, world!" and append into it.

        if(Crypto::HasKey()){
            MainMenu();
        }
        else{
            EnterEncryptionKey();
        }

        ImGui::End();
    }
}

void GUI::GLFWErrorCallback(int error, const char *description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

GUI::~GUI()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
