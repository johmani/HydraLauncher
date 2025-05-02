#include "HydraEngine/Base.h"
#include "icon.h"
#include <format>
#include "IconsFontAwesome.h"

import HE;
import std;
import Math;
import nvrhi;
import simdjson;
import ImGui;
import Utils;
import Git;

using namespace HE;

//////////////////////////////////////////////////////////////////////////
// UI
//////////////////////////////////////////////////////////////////////////
	
struct Color {

	enum
	{
		PrimaryButton,
		Selected,
		
		TextButtonHovered,
		TextButtonActive,

		Dangerous,
		DangerousHovered,
		DangerousActive,

		Info,
		Warn,
		Error,

		ChildBlock,
		
		Count
	};
};

struct FontType
{
	enum : uint8_t
	{
		Regular = 0,
		Blod = 1,
	};
};

struct FontSize
{
	enum : uint8_t
	{
		Header0 = 36,
		Header1 = 28,
		Header2 = 24,
		BodyLarge = 20,
		BodyMedium = 18,
		BodySmall = 16,
		Caption = 13,
	};
};

struct CreateProjectStage
{
	enum : uint8_t
	{
		Templates,
		Plugins,
	};
};

struct Page
{
	enum : uint8_t
	{
		Projects,
		Plugins,
		Install,
		Templates,
		Samples,
	};
};

//////////////////////////////////////////////////////////////////////////
// App
//////////////////////////////////////////////////////////////////////////

struct InstallationState
{
	enum : uint8_t
	{
		NotInstalled,
		Installing,
		Installed,
		Wait,
		Build,
		Failed
	};

	static const char* ToString(uint8_t state)
	{
		switch (state)
		{
		case NotInstalled: return "NotInstalled";
		case Installing:   return "Installing";
		case Installed:	   return "Installed";
		case Wait:		   return "Wait";
		case Failed:	   return "Failed";
		}

		HE_ASSERT(false);
		return "";
	}

	static uint8_t fromString(const std::string_view& stateStr)
	{
		if (stateStr == "NotInstalled") return NotInstalled;
		if (stateStr == "Installing")   return Installing;
		if (stateStr == "Installed")    return Installed;
		if (stateStr == "Wait")         return Wait;
		if (stateStr == "Failed")       return Failed;

		HE_ASSERT(false);
		return -1;
	}
};

enum class RemoteType : uint8_t
{
	Plugin,
	Template
};

struct BuildConfig
{
	enum : uint8_t
	{
		Debug,
		Release,
		Profile,
		Dist
	};
};

struct Info
{
	std::string name;
	std::string description;
	std::string URL;

	uint8_t installationState = InstallationState::NotInstalled;
	Git::ProgressInfo progress = { {0} };
};

struct Engine
{
	std::filesystem::path path = "path";
	std::string id;
	uint8_t installationState = InstallationState::NotInstalled;
	Git::ProgressInfo progress = { {0} };
};

struct Plugin
{
	Info info;
	bool enabledByDefault = false;
};

struct Template
{
	Info info;
	nvrhi::TextureHandle thumbnail;
};

struct Project
{
	std::string name;
	std::string path;
	std::string engineID = " null ";
	std::filesystem::path buildDir;
	bool includSourceCode = false;
	bool isBuilding = false;

	Utils::Process process;
};

constexpr const char* c_AppName = "Hydra Launcher";
constexpr const char* c_AppIconPath = "Resources/Icons/Hydra.png";

constexpr const char* c_EngineRemoteRepo = "https://github.com/johmani/HydraEngine";
constexpr const char* c_RemotePluginsURL = "https://drive.google.com/uc?export=download&id=1NCUiXzvrVO0Ujl47zZXacl2jcz3ZGGf_";
constexpr const char* c_EngineLibRemoteRepo = "https://github.com/johmani/HydraEngineLibs_Windows_x64";

constexpr const char* c_VSwherePath = "C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe";
constexpr const char* c_FindMsBuildCmd = "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe\" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe";

constexpr const char* c_ConfigStr[] = { "Debug", "Release", "Profile", "Dist" };

//////////////////////////////////////////////////////////////////////////
// Layer
//////////////////////////////////////////////////////////////////////////

class HydraLauncher : public Layer
{
public:

	// UI
	#define Icon_Box           ICON_FA_BOX
	#define Icon_Download      ICON_FA_DOWNLOAD
	#define Icon_CloudDownload ICON_FA_CLOUD_DOWNLOAD_ALT
	#define Icon_Cube          ICON_FA_CUBE
	#define Icon_Plus          ICON_FA_PLUS
	#define Icon_Search        ICON_FA_SEARCH
	#define Icon_Trash         ICON_FA_TRASH_ALT
	#define Icon_Recycle       ICON_FA_RECYCLE
	#define Icon_Warning       ICON_FA_EXCLAMATION_TRIANGLE
	#define Icon_X             ICON_FA_TIMES
	#define Icon_CheckCircle   ICON_FA_CHECK_CIRCLE
	#define Icon_InfoCircle    ICON_FA_INFO_CIRCLE
	#define Icon_FolderOpen    ICON_FA_FOLDER_OPEN
	#define Icon_Folder        ICON_FA_FOLDER
	#define Icon_Plugin        ICON_FA_PLUG
	#define Icon_Settings      ICON_FA_BAHAI
	#define Icon_Build         ICON_FA_HAMMER
	#define Icon_Code          ICON_FA_CODE
	const char* c_DeleteMessage = "This action will delete the files on disk and cannot be undone. Are you sure you want to delete?";
	ImVec4 colors[Color::Count];
	uint8_t selectedPage = Page::Projects;
	uint8_t createProjectStage = CreateProjectStage::Templates;
	ImGuiTextFilter textFilter;
	bool showDeletePopub = false;
	bool showCreateNewProjectPopub = false;
	std::function<void()> submitedFunction;
	std::string projectName = "NewProject";
	std::string projectPath = "";
	uint32_t selectedTemplate = 0;
	bool openOutputDirAfterProjectBuild = false;
	bool buildAndRunProject = false;
	bool showBuildOutput = false;

	// Graphics
	nvrhi::TextureHandle icon, close, min, max, res;
	nvrhi::DeviceHandle device;
	nvrhi::CommandListHandle commandList;
	
	// Appliction
	std::vector<Project> projects;
	std::vector<Template> templates;
	std::vector<Engine> instances;
	std::vector<Plugin> plugins;
	uint32_t installedInstances = 0;
	uint32_t installedTemplates = 0;
	uint32_t installedPlugins = 0;

	std::filesystem::path appData;
	std::filesystem::path remoteInfoFilePath;
	std::filesystem::path databaseFilePath;
	std::filesystem::path templatesDir;
	std::filesystem::path pluginsDir;
	std::string msBuildPath;
	
	std::mutex templatesMutex;
	std::mutex pluginsMutex;
	std::mutex projectsMutex;


#pragma region Engine Functions

	virtual void OnAttach() override
	{
		HE_PROFILE_FUNCTION();

		device = RHI::GetDevice();
		Git::Init();

		{
			HE_PROFILE_SCOPE("init paths");

			appData = Utils::GetAppDataPath(c_AppName);
			remoteInfoFilePath = std::filesystem::absolute(appData / "remoteInfo.json").lexically_normal();
			databaseFilePath = std::filesystem::absolute(appData / "db.json").lexically_normal();
			templatesDir = std::filesystem::absolute(appData / "Templates").lexically_normal();
			pluginsDir = std::filesystem::absolute(appData / "Plugins").lexically_normal();

			std::filesystem::create_directories(templatesDir);
			std::filesystem::create_directories(pluginsDir);

			static std::string str;
			Utils::ExecCommand(c_FindMsBuildCmd, &str, nullptr, true, [this]() {
				if (!str.empty())
				{
					msBuildPath = std::filesystem::path(str).parent_path().string();
				}
			});
		}

		commandList = device->createCommandList();
		commandList->open();

		{
			Plugins::LoadPlugin("Plugins/HEImGui/HEImGui.hplugin");
		}

		{
			HE_PROFILE_SCOPE("Load icons textures");

			icon = Utils::LoadTexture(Application::GetApplicationDesc().windowDesc.iconFilePath, device, commandList);
			close = Utils::LoadTexture(Buffer(g_icon_close, sizeof(g_icon_close)), device, commandList);
			min = Utils::LoadTexture(Buffer(g_icon_minimize, sizeof(g_icon_minimize)), device, commandList);
			max = Utils::LoadTexture(Buffer(g_icon_maximize, sizeof(g_icon_maximize)), device, commandList);
			res = Utils::LoadTexture(Buffer(g_icon_restore, sizeof(g_icon_restore)), device, commandList);
		}

		{
			HE_PROFILE_SCOPE("Load App info");

			Deserialize();
			FindAndAddPlugins();
			FindAndAddTemplates();
			GetRemoteInfo();
		}

		commandList->close();
		device->executeCommandList(commandList);

		{
			Utils::Theme();

			colors[Color::PrimaryButton] = { 0.278431f, 0.701961f, 0.447059f, 1.00f };
			colors[Color::TextButtonHovered] = { 0.278431f, 0.701961f, 0.447059f, 1.00f };
			colors[Color::TextButtonActive] = { 0.278431f, 0.801961f, 0.447059f, 1.00f };
			colors[Color::Dangerous] = { 0.8f, 0.3f, 0.2f, 1.0f };
			colors[Color::DangerousHovered] = { 0.9f, 0.2f, 0.2f, 1.0f };
			colors[Color::DangerousActive] = { 1.0f, 0.2f, 0.2f, 1.0f };
			colors[Color::Info] = { 0.278431f, 0.701961f, 0.447059f, 1.00f };
			colors[Color::Warn] = { 0.8f, 0.8f, 0.2f, 1.0f };
			colors[Color::Error] = { 0.8f, 0.3f, 0.2f, 1.0f };
			colors[Color::ChildBlock] = { 0.1f,0.1f ,0.1f ,1.0f };
			colors[Color::Selected] = { 0.3f, 0.3f, 0.3f , 1.0f };

			std::string layoutPath = (appData / "layout.ini").lexically_normal().string();
			ImGui::GetIO().IniFilename = nullptr;
			ImGui::LoadIniSettingsFromDisk(layoutPath.c_str());
		}
	}

	virtual void OnEvent(Event& e) override
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowContentScaleEvent>([this](Event& e) { Utils::Theme(); return false; });
	}

	virtual void OnDetach() override
	{
		HE_PROFILE_FUNCTION();

		Git::Shutdown();
	}

	virtual void OnBegin(const FrameInfo& info) override
	{
		HE_PROFILE_FUNCTION();

		commandList->open();
		nvrhi::utils::ClearColorAttachment(commandList, info.fb, 0, nvrhi::Color(0.1f));
	}

	virtual void OnEnd(const FrameInfo& info) override
	{
		HE_PROFILE_FUNCTION();

		commandList->close();
		device->executeCommandList(commandList);
	}

	virtual void OnUpdate(const FrameInfo& info) override
	{
		HE_PROFILE_FUNCTION();

		// UI
		{
			HE_PROFILE_SCOPE_NC("UI", 0xFF0000FF);

			float dpi = ImGui::GetWindowDpiScale();
			float scale = ImGui::GetIO().FontGlobalScale * dpi;
			auto& style = ImGui::GetStyle();
			style.FrameRounding = Math::max(3.0f * scale,1.0f);

			ImVec2 mainWindowPadding = ImVec2(1, 1) * scale;
			ImVec2 mainItemSpacing = ImVec2(2, 2) * scale;
			
			ImVec2 headWindowPadding = ImVec2(18, 18) * scale;
			ImVec2 headFramePadding = ImVec2(14, 10) * scale;
			
			ImVec2 bodyWindowPadding = ImVec2(8, 8) * scale;
			ImVec2 bodyFramePadding = ImVec2(8, 10) * scale;
			ImVec2 cellPadding = ImVec2(20, 20) * scale;

			ImGui::ScopedStyle wb(ImGuiStyleVar_WindowBorderSize, 0.0f);

			auto dockFlags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar;
			ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID, ImGui::GetMainViewport(), dockFlags);

			// Titlebar
			{
				bool customTitlebar = Application::GetApplicationDesc().windowDesc.customTitlebar;
				bool isIconClicked = Utils::BeginMainMenuBar(customTitlebar, icon.Get(), close.Get(), min.Get(), max.Get(), res.Get());

				{
					ImGui::SetNextWindowPos(ImGui::GetCurrentContext()->CurrentWindow->Pos + ImVec2(0, ImGui::GetCurrentContext()->CurrentWindow->Size.y));

					if (isIconClicked)
					{
						ImGui::OpenPopup("Options");
					}

					if (ImGui::BeginPopup("Options"))
					{
						{
							ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, ImVec2{ 1, 1 });
							ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
							ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);

							float w = 120 * scale;
							{
								ImGui::ScopedFont sf(FontType::Blod);
								if (ImGui::TextButton("Options"))
								{
									ImGui::GetIO().FontGlobalScale = 1;
									Serialize();
								}
							}

							ImGui::Dummy({ w, 1 });

							if (ImGui::TextButton("Font Scale"))
							{
								ImGui::GetIO().FontGlobalScale = 1;
								Serialize();
							}

							ImGui::SameLine(0, w - ImGui::CalcTextSize("Font Scale").x);
							ImGui::DragFloat("##Font Scale", &ImGui::GetIO().FontGlobalScale, 0.01f, 0.1f, 2.0f);
							if (ImGui::IsItemDeactivatedAfterEdit()) { Serialize(); }
						}

						ImGui::EndPopup();
					}
				}
			
				if (ImGui::BeginMenu("Edit"))
				{
					if (ImGui::MenuItem("Exit", "ESC"))
						Application::Shutdown();

					if (ImGui::MenuItem("Full Screen", "F", Application::GetWindow().IsFullScreen()))
						Application::GetWindow().ToggleScreenState();

					ImGui::EndMenu();
				}
				
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("New Project", "Ctrl + N",nullptr,installedInstances > 0 ))
						showCreateNewProjectPopub = true;

					if (ImGui::MenuItem("Add Project", "Ctrl + A", nullptr, installedInstances > 0))
						AddProject();

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Project"))
				{
					if (ImGui::MenuItem("Open Output Directory After Project Build", nullptr, &openOutputDirAfterProjectBuild))
						Serialize();

					if (ImGui::MenuItem("Build Project And Run", nullptr, &buildAndRunProject))
						Serialize();
					
					if (ImGui::MenuItem("Show Build Output", nullptr, &showBuildOutput))
						Serialize();

					ImGui::EndMenu();
				}

				Utils::EndMainMenuBar();
			}

			// Main Content
			{
				ImGui::ScopedStyle scopedWindowPadding(ImGuiStyleVar_WindowPadding, mainWindowPadding);
				ImGui::ScopedStyle scopedItemSpacing(ImGuiStyleVar_ItemSpacing, mainItemSpacing);

				constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(viewport->WorkPos);
				ImGui::SetNextWindowSize(viewport->WorkSize);
				ImGui::Begin("Fullscreen window", nullptr, flags);

				{
					ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, bodyWindowPadding);
					ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, bodyFramePadding);
					ImGui::ScopedStyle bta(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.1f * scale, 0.5f));
					ImGui::ScopedColor sc(ImGuiCol_ButtonActive, colors[Color::PrimaryButton] * 0.9f);

					ImGui::BeginChild("Left", ImVec2(Math::max(ImGui::GetContentRegionAvail().x / 10, 150 * scale), 0), ImGuiChildFlags_AlwaysUseWindowPadding);

					if (ImGui::SelectableButton(Icon_Box"    Projects", { -1 , 0 }, selectedPage == Page::Projects))
					{
						textFilter.Clear();
						selectedPage = Page::Projects;
						Serialize();
					}

					if (ImGui::SelectableButton(Icon_Plugin"    Plugins", { -1 , 0 }, selectedPage == Page::Plugins))
					{
						textFilter.Clear();
						selectedPage = Page::Plugins;
						Serialize();
					}

					if (ImGui::SelectableButton(Icon_Download"    Install", { -1 , 0 }, selectedPage == Page::Install))
					{
						textFilter.Clear();
						selectedPage = Page::Install;
						Serialize();
					}

					if (ImGui::SelectableButton(Icon_Cube"    Templates", { -1 , 0 }, selectedPage == Page::Templates))
					{
						textFilter.Clear();
						selectedPage = Page::Templates;
						Serialize();
					}

					ImGui::EndChild();
				}

				ImGui::SameLine();

				switch (selectedPage)
				{
				case Page::Projects:
				{
					ImGui::BeginGroup();
					{
						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, headWindowPadding);
							ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, headFramePadding);

							ImGui::BeginChild("Head", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY);

							// Page Titile
							{
								ImGui::ScopedFont sf(FontType::Blod, FontSize::Header0);
								ImGui::TextUnformatted("Projects");
							}

							// Search
							{
								auto s1 = ImGui::CalcTextSize(Icon_Cube"  New Project").x + style.FramePadding.x * 2;
								auto s2 = ImGui::CalcTextSize(Icon_Plus"  Add").x + style.FramePadding.x * 2;
								float availableWidth = ImGui::GetContentRegionAvail().x - (s1 + s2) - style.WindowPadding.x;
								ImGui::SetNextItemWidth(availableWidth);
								if (ImGui::InputTextWithHint("##Search", Icon_Search " Search...", textFilter.InputBuf, sizeof(textFilter.InputBuf)))
									textFilter.Build();
							}

							ImGui::SameLine();

							{
								ImGui::ScopedDisabled sd(installedInstances == 0);
								ImGui::ScopedFont sf(FontType::Blod);
								ImGui::ScopedButtonColor sbc(colors[Color::PrimaryButton]);

								if (ImGui::Button(Icon_Cube"  New Project"))
									showCreateNewProjectPopub = true;

								ImGui::SameLine();

								if (ImGui::Button(Icon_Plus"  Add", { -1,0 }))
									AddProject();
							}

							ImGui::EndChild();
						}

						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, bodyWindowPadding);

							ImGui::BeginChild("Right", { 0, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding);

							ImGuiTableFlags tableFlags =
								ImGuiTableFlags_RowBg |
								ImGuiTableFlags_PadOuterX |
								ImGuiTableFlags_SizingStretchProp |
								ImGuiTableFlags_BordersInnerH |
								ImGuiTableFlags_ScrollY;

							ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(20,0));
							if (ImGui::BeginTable("Projects", 4, tableFlags))
							{
								{
									ImGui::ScopedFont sf(FontType::Blod, FontSize::BodyMedium);
									ImGui::ScopedStyle ss(ImGuiStyleVar_CellPadding, bodyWindowPadding);
									ImGui::TableSetupColumn("Name");
									ImGui::TableSetupColumn("Engine ID");
									ImGui::TableSetupColumn("Date modified");
									ImGui::TableSetupColumn("Actions");
									ImGui::TableHeadersRow();
								}

								for (int i = 0; i < projects.size(); i++)
								{
									auto& project = projects[i];

									auto ins = GetEngineInsByID(project.engineID);
									auto exists = std::filesystem::exists(project.path);
									bool valid = (bool)ins && exists;

									if (!textFilter.PassFilter(project.engineID.c_str()) && !textFilter.PassFilter(project.name.c_str()) && !textFilter.PassFilter(project.path.c_str())) 
										continue;

									ImGui::TableNextRow();
									ImGui::ScopedID sid(i);

									ImGui::TableSetColumnIndex(0);
									ImGui::SetNextItemAllowOverlap();
									auto cursor = ImGui::GetCursorPos();
									{
										ImGui::Selectable("##selectable", false, ImGuiSelectableFlags_AllowOverlap | ImGuiSelectableFlags_SpanAllColumns, { 0, 70 * scale });
										if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
										{
											auto sln = std::filesystem::path(std::format("{}/{}.sln", project.path, project.name)).lexically_normal();
											if (std::filesystem::exists(sln))
											{
												FileSystem::Open(sln);
											}
											else if (std::filesystem::exists(sln.parent_path()))
											{
												Jops::SubmitTask([this, sln, &project]() { RunProjectPremake(project); FileSystem::Open(sln); });
											}
										}
									}

									// project name
									ImGui::SetCursorPosY((cursor.y + (15 * scale)));
									{
										ImGui::ScopedFont sf(FontType::Blod, FontSize::BodyMedium);
										ImGui::TextUnformatted(project.name.c_str());
									}
									{
										if (!exists) ImGui::PushStyleColor(ImGuiCol_Text, colors[Color::Error]);
										{
											ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
											ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);
											if (ImGui::TextButton(project.path.c_str()) && exists)
												FileSystem::Open(project.path);
										}
										if (!exists) ImGui::PopStyleColor();
									}

									// engineID
									ImGui::TableSetColumnIndex(1);
									ImGui::ShiftCursorY(25 * scale);
									if(exists)
									{
										ImGui::SetNextItemWidth(ImGui::CalcTextSize(project.engineID.c_str()).x * 1.8f);
										if (!ins) ImGui::PushStyleColor(ImGuiCol_Text, colors[Color::Error]);
										if (ImGui::BeginCombo("##engineID", project.engineID.c_str()))
										{
											for (int i = 0; i < instances.size(); i++)
											{
												if (instances[i].id.empty())
													continue;

												ImGui::ScopedColor sc(ImGuiCol_Text, colors[Color::Info]);
												ImGui::ScopedID sid(i);
												if (ImGui::Selectable(instances[i].id.c_str()))
													ChangeEngineForProject(project, instances[i]);
											}
											ImGui::EndCombo();
										}
										if (!ins) ImGui::PopStyleColor();
									}
									else
									{
										ImGui::TextUnformatted("- - - - - -");
									}

									// Date modified
									ImGui::TableSetColumnIndex(2);
									ImGui::ShiftCursorY(25 * scale);
									ImGui::TextUnformatted(Utils::GetLastWriteTime(project.path));

									// actions
									ImGui::TableSetColumnIndex(3);
									ImGui::ShiftCursorY(25 * scale);
									{
										ImGui::SameLine(0, 8);

										{
											ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
											ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);
											ImGui::ScopedDisabled sd(!valid);

											if (ImGui::TextButton(Icon_Recycle))
												Jops::SubmitTask([this, &project]() { RunProjectPremake(project); });
										}
										ImGui::ToolTip("regenerate project files");

										ImGui::SameLine(0, 8);
										{
											ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
											ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);
											ImGui::ScopedDisabled sd(!valid);

											if (ImGui::TextButton(Icon_Settings))
												ImGui::OpenPopup("projectSettings");

											ImGui::ScopedStyle ss0(ImGuiStyleVar_WindowPadding, bodyWindowPadding);
											ImGui::ScopedStyle ss3(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
											if (ImGui::BeginPopup("projectSettings"))
											{
												if (ImGui::MenuItem("Includ Source Code",nullptr, project.includSourceCode))
													IncludeSourceCodeForProject(project);

												ImGui::Separator();

												if (ImGui::BeginMenu("Build"))
												{
													if (ImGui::MenuItem("Pick Output Directory"))
													{
														auto path = FileDialog::SelectFolder();
														if(!path.empty())
															project.buildDir = path;
													}
													
													ImGui::Separator();

													for (int i = 0; i < 4; i++)
													{
														if (ImGui::MenuItem(c_ConfigStr[i]))
															BuildProject(project, i);
													}

													ImGui::EndMenu();
												}

												ImGui::EndPopup();
											}
										}
										ImGui::ToolTip("project settings");

										ImGui::SameLine(0, 8);
										{
											ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
											ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);

											if (ImGui::TextButton(Icon_X))
												RemoveProject(i);
										}
										ImGui::ToolTip("remove from list");

										if(project.isBuilding)
										{
											auto cx = ImGui::GetCursorPosX();
											{
												static int index = 0;
												static float time = 0;
												static const char* arr[] = { "-", "- -", "- - -", "- - - -", "- - - - -" };

												ImGui::ScopedColor sc(ImGuiCol_Text, colors[Color::Info]);
												ImGui::ScopedFont sf(FontType::Blod, FontSize::BodySmall);

												ImGui::Text("%s %s", Icon_Build, arr[index]);

												time += info.ts;
												if (time > 1) { index++;  time = 0; }
												if (index > 4) { index = 0; }
											}

											{
												ImGui::ScopedFont sf(FontType::Blod);

												ImGui::ScopedColor sc0(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
												ImGui::ScopedColor sc1(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);

												ImGui::SameLine(0, 8);
												ImGui::SetCursorPosX(cx + ImGui::CalcTextSize(Icon_Build"- - - - -").x + 16);
												if (ImGui::TextButton("Cancel"))
													project.process.Kill();
											}

										}
									}
								}

								ImGui::EndTable();
							}
							ImGui::PopStyleVar();

							ImGui::EndChild();
						}
					}
					ImGui::EndGroup();
					break;
				}
				case Page::Install:
				{
					ImGui::BeginGroup();
					{
						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, headWindowPadding);
							ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, headFramePadding);

							ImGui::BeginChild("Head", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY);

							// Page Titile
							{
								ImGui::ScopedFont sf(FontType::Blod, FontSize::Header0);
								ImGui::TextUnformatted("Engine");
							}

							// Search
							{
								auto s = ImGui::CalcTextSize(Icon_Plus"  Add").x + style.FramePadding.x * 2;
								float availableWidth = ImGui::GetContentRegionAvail().x - s - style.WindowPadding.x;
								ImGui::SetNextItemWidth(availableWidth);
								if (ImGui::InputTextWithHint("##Search", Icon_Search " Search...", textFilter.InputBuf, sizeof(textFilter.InputBuf)))
								{
									textFilter.Build();
								}
							}

							ImGui::SameLine();

							{
								ImGui::ScopedFont sf(FontType::Blod);
								ImGui::ScopedButtonColor sbt(colors[Color::PrimaryButton]);

								if (ImGui::Button(Icon_Plus"  Add", { -1,0 }))
								{
									AddInstance();
								}
							}

							ImGui::EndChild();
						}

						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, bodyWindowPadding);

							// Visual Studio 2022
							{
								ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, ImVec2(16, 16) * scale);

								float shift = ImGui::CalcTextSize("Required").x;

								ImGui::BeginChild("Visual Studio 2022", { -1, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY);
								ImGui::TextLinkOpenURL("Visual Studio 2022", "https://visualstudio.microsoft.com");
								ImGui::SameLine();
								ImGui::ShiftCursorX(ImGui::GetContentRegionAvail().x - shift);
								if (IsVisualStudioInstalled())
									ImGui::TextColored(colors[Color::Info], "Installed");
								else
									ImGui::TextColored(colors[Color::Warn], "Required");
								ImGui::EndChild();
							}

							ImGui::BeginChild("Right", { 0, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding);

							ImGuiTableFlags tableFlags =
								ImGuiTableFlags_RowBg |
								ImGuiTableFlags_PadOuterX |
								ImGuiTableFlags_SizingStretchProp |
								ImGuiTableFlags_BordersInnerH |
								ImGuiTableFlags_ScrollY;

							ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cellPadding);
							if (ImGui::BeginTable("instances", 4, tableFlags))
							{
								{
									ImGui::ScopedFont sf(FontType::Blod, FontSize::BodyMedium);
									ImGui::ScopedStyle ss(ImGuiStyleVar_CellPadding, bodyWindowPadding);
									ImGui::TableSetupColumn("Path");
									ImGui::TableSetupColumn("engineID");
									ImGui::TableSetupColumn("Actions");
									ImGui::TableSetupColumn("State");
									ImGui::TableHeadersRow();
								}

								installedInstances = 0;
								for (int i = 0; i < instances.size(); i++)
								{
									auto& instance = instances[i];
									auto path = instance.path.string();

									auto exists = std::filesystem::exists(instance.path);

									bool isValidHydraDirectory = IsValidHydraDirectory(instance.path);
									if (isValidHydraDirectory)
										installedInstances++;


									if (!textFilter.PassFilter(instance.id.c_str()) && !textFilter.PassFilter(path.c_str()))
										continue;

									ImGui::TableNextRow();
									ImGui::ScopedID sid(i);

									ImGui::TableSetColumnIndex(0);

									// path
									{
										ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
										ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);
										if (ImGui::TextButton(path.c_str()) && exists)
											FileSystem::Open(instance.path);
									}

									// id
									ImGui::TableSetColumnIndex(1);
									
									if (!instance.id.empty())
									{
										ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
										ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);

										if (ImGui::TextButton(instance.id.c_str()))
											FileSystem::Open(std::format("{}/commit/{}", c_EngineRemoteRepo, instance.id));
									}
									else
									{
										ImGui::TextUnformatted("- - - - - - -");
									}

									// actions
									ImGui::TableSetColumnIndex(2);
									{
										switch (instance.installationState)
										{
										case InstallationState::NotInstalled:
										case InstallationState::Failed:
										{
											ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
											ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);

											{
												if (ImGui::TextButton(Icon_FolderOpen))
												{
													PickEngineDirectory(instance);
												}
												ImGui::ToolTip("pick Location");
											}
											
											ImGui::SameLine(0, 8);

											{
												ImGui::ToolTip("download");
												if (ImGui::TextButton(Icon_Download))
												{
													DownLoadEngine(instance);
												}
												ImGui::ToolTip("download");
											}

											ImGui::SameLine(0, 8);

											{
												if (ImGui::TextButton(Icon_X))
												{
													RemoveInstance(i);
												}
												ImGui::ToolTip("remove from list");
											}

											break;
										}
										case InstallationState::Build:
										case InstallationState::Installing:
										{
											float val = GetProgress(instance.progress) / 100.0f;
											ImGui::ProgressBar(val);
											ImGui::Text("%s %i/%i", instance.progress.stepName.c_str(), instance.progress.completedSteps, instance.progress.totalSteps);

											ImGui::SameLine();
											ImGui::ShiftCursorX(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(Icon_X).x - 2);
											if (ImGui::TextButton(Icon_X))
												Git::Cancel(instance.progress);

											ImGui::ToolTip("Cancel");
											break;
										}
										case InstallationState::Installed:
										{
											{
												ImGui::ScopedColor sc0(ImGuiCol_Text, colors[Color::Info]);
												ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
												ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonHovered]);

												if (ImGui::TextButton(Icon_Build))
												{
													BuildEngine(instance);
												}
												ImGui::ToolTip("build");
											}
											
											ImGui::SameLine(0, 8);

											{
												ImGui::ScopedColor sc0(ImGuiCol_Text, colors[Color::Dangerous]);
												ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::DangerousHovered]);
												ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::DangerousActive]);
												
												if (ImGui::TextButton(Icon_Trash))
												{
													submitedFunction = [this, &instance]() { DeleteEngine(instance); };
													showDeletePopub = true;
												}
												ImGui::ToolTip("delete from disk");
											}
											
											ImGui::SameLine(0, 8);
											
											{
												if (ImGui::TextButton(Icon_X))
												{
													RemoveInstance(i);
												}
												ImGui::ToolTip("remove from list");
											}

											break;
										}
										case InstallationState::Wait:
										{
											break;
										}
										default: HE_ASSERT(false);
										}
									}

									// state
									ImGui::TableSetColumnIndex(3);
									DrawInstallationState(instance.installationState);
								}

								ImGui::EndTable();
							}
							ImGui::PopStyleVar();

							

							ImGui::EndChild();
						}
					}
					ImGui::EndGroup();
					break;
				}
				case Page::Plugins:
				{
					ImGui::BeginGroup();
					{
						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, headWindowPadding);
							ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, headFramePadding);

							ImGui::BeginChild("Head", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY);

							// Page Titile
							{
								ImGui::ScopedFont sf(FontType::Blod, FontSize::Header0);
								ImGui::TextUnformatted("Plugins");
							}

							// Search
							{
								ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
								if (ImGui::InputTextWithHint("##Search", Icon_Search " Search...", textFilter.InputBuf, sizeof(textFilter.InputBuf)))
								{
									textFilter.Build();
								}
							}

							ImGui::EndChild();
						}

						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, bodyWindowPadding);

							ImGui::BeginChild("Right", { 0, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding);

							ImGuiTableFlags tableFlags =
								ImGuiTableFlags_RowBg |
								ImGuiTableFlags_PadOuterX |
								ImGuiTableFlags_SizingStretchProp |
								ImGuiTableFlags_BordersInnerH |
								ImGuiTableFlags_ScrollY;

							ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cellPadding);
							if (ImGui::BeginTable("Plgins", 4, tableFlags))
							{
								{
									ImGui::ScopedFont sf(FontType::Blod, FontSize::BodyMedium);
									ImGui::ScopedStyle ss(ImGuiStyleVar_CellPadding, bodyWindowPadding);
									ImGui::TableSetupColumn("Name");
									ImGui::TableSetupColumn("Description");
									ImGui::TableSetupColumn("Actions");
									ImGui::TableSetupColumn("State");
									ImGui::TableHeadersRow();
								}

								for (int i = 0; i < plugins.size(); i++)
								{
									auto& pluginInfo = plugins[i].info;

									if (!textFilter.PassFilter(pluginInfo.description.c_str()) && !textFilter.PassFilter(pluginInfo.name.c_str()) && !textFilter.PassFilter(pluginInfo.URL.c_str()))
										continue;

									ImGui::TableNextRow();
									ImGui::ScopedID sid(i);
									
									// plugin name
									{
										ImGui::TableSetColumnIndex(0);

										{
											ImGui::ScopedFont sf(FontType::Blod, FontSize::BodyMedium);
											ImGui::TextUnformatted(pluginInfo.name.c_str());
										}
										{
											ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
											ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);

											if (ImGui::TextButton(pluginInfo.URL.c_str()))
												FileSystem::Open(pluginInfo.URL);
										}
									}

									// desc
									ImGui::TableSetColumnIndex(1);
									ImGui::TextWrapped(pluginInfo.description.c_str());

									// actions
									{
										ImGui::TableSetColumnIndex(2);

										switch (pluginInfo.installationState)
										{
										case InstallationState::NotInstalled:
										case InstallationState::Failed:
										{
											{
												ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
												ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);

												if (ImGui::TextButton(Icon_Download))
												{
													DownLoad(pluginInfo, RemoteType::Plugin);
												}
												ImGui::ToolTip("download");
											}
											
											break;
										}
										case InstallationState::Installing:
										{
											float val = GetProgress(pluginInfo.progress) / 100.0f;
											ImGui::ProgressBar(val);
											ImGui::Text("%s %i/%i", pluginInfo.progress.stepName.c_str(), pluginInfo.progress.completedSteps, pluginInfo.progress.totalSteps);
											
											ImGui::SameLine();
											ImGui::ShiftCursorX(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(Icon_X).x  - 2);
											if (ImGui::TextButton(Icon_X))
												Git::Cancel(pluginInfo.progress);
											ImGui::ToolTip("Cancel");

											break;
										}
										case InstallationState::Installed:
										{
											{
												ImGui::ScopedColor sc0(ImGuiCol_Text, colors[Color::Dangerous]);
												ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::DangerousHovered]);
												ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::DangerousActive]);
												if (ImGui::TextButton(Icon_Trash))
												{
													submitedFunction = [this, &pluginInfo]() { Delete(pluginInfo, RemoteType::Plugin); };
													showDeletePopub = true;
												}
												ImGui::ToolTip("delete from disk");
											}
											break;
										}
										case InstallationState::Wait:
										{
											break;
										}
										default: HE_ASSERT(false);
										}
									}

									// state
									ImGui::TableSetColumnIndex(3);
									DrawInstallationState(pluginInfo.installationState);
								}

								ImGui::EndTable();
							}
							ImGui::PopStyleVar();

							ImGui::EndChild();
						}
					}
					ImGui::EndGroup();
					break;
				}
				case Page::Templates:
				{
					ImGui::BeginGroup();
					{
						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, headWindowPadding);
							ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, headFramePadding);

							ImGui::BeginChild("Head", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY);

							// Page Titile
							{
								ImGui::ScopedFont sf(FontType::Blod, FontSize::Header0);
								ImGui::TextUnformatted("Plugins");
							}

							// Search
							{
								ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
								if (ImGui::InputTextWithHint("##Search", Icon_Search " Search...", textFilter.InputBuf, sizeof(textFilter.InputBuf)))
								{
									textFilter.Build();
								}
							}

							ImGui::EndChild();
						}

						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, bodyWindowPadding);

							ImGui::BeginChild("Right", { 0, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding);

							ImGuiTableFlags tableFlags =
								ImGuiTableFlags_RowBg |
								ImGuiTableFlags_PadOuterX |
								ImGuiTableFlags_SizingStretchProp |
								ImGuiTableFlags_BordersInnerH |
								ImGuiTableFlags_ScrollY;

							ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cellPadding);
							if (ImGui::BeginTable("Templates", 4, tableFlags))
							{
								{
									ImGui::ScopedFont sf(FontType::Blod, FontSize::BodyMedium);
									ImGui::ScopedStyle ss(ImGuiStyleVar_CellPadding, bodyWindowPadding);
									ImGui::TableSetupColumn("Name");
									ImGui::TableSetupColumn("Description");
									ImGui::TableSetupColumn("Actions");
									ImGui::TableSetupColumn("State");
									ImGui::TableHeadersRow();
								}

								for (int i = 0; i < templates.size(); i++)
								{
									auto& templateInfo = templates[i].info;

									if (!textFilter.PassFilter(templateInfo.description.c_str()) && !textFilter.PassFilter(templateInfo.name.c_str()) && !textFilter.PassFilter(templateInfo.URL.c_str()))
										continue;

									ImGui::TableNextRow();
									ImGui::ScopedID sid(i);

									ImGui::TableSetColumnIndex(0);

									// plugin name
									{
										ImGui::ScopedFont sf(FontType::Blod, FontSize::BodyMedium);
										ImGui::TextUnformatted(templateInfo.name.c_str());
									}
									{
										ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
										ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);
										if (ImGui::TextButton(templateInfo.URL.c_str()))
											FileSystem::Open(templateInfo.URL);
									}

									// desc
									ImGui::TableSetColumnIndex(1);
									ImGui::TextWrapped(templateInfo.description.c_str());

									// actions
									ImGui::TableSetColumnIndex(2);
									{
										switch (templateInfo.installationState)
										{
										case InstallationState::NotInstalled:
										case InstallationState::Failed:
										{
											{
												ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::TextButtonHovered]);
												ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::TextButtonActive]);

												if (ImGui::TextButton(Icon_Download))
												{
													DownLoad(templateInfo, RemoteType::Template);
												}
												ImGui::ToolTip("download");
											}

											break;
										}
										case InstallationState::Installing:
										{
											float val = GetProgress(templateInfo.progress) / 100.0f;
											ImGui::ProgressBar(val);
											ImGui::Text("%s %i/%i", templateInfo.progress.stepName.c_str(), templateInfo.progress.completedSteps, templateInfo.progress.totalSteps);

											ImGui::SameLine();
											ImGui::ShiftCursorX(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(Icon_X).x - 2);
											if (ImGui::TextButton(Icon_X))
											{
												Git::Cancel(templateInfo.progress);
											}
											ImGui::ToolTip("Cancel");
											break;
										}
										case InstallationState::Installed:
										{
											{
												ImGui::ScopedColor sc0(ImGuiCol_Text, colors[Color::Dangerous]);
												ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::DangerousHovered]);
												ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::DangerousActive]);
												if (ImGui::TextButton(Icon_Trash))
												{
													submitedFunction = [this, &templateInfo]() { Delete(templateInfo, RemoteType::Template); };
													showDeletePopub = true;
												}
												ImGui::ToolTip("delete from disk");
											}

											break;
										}
										case InstallationState::Wait:
										{
											
											break;
										}
										default: HE_ASSERT(false);
										}
									}

									// state
									ImGui::TableSetColumnIndex(3);
									DrawInstallationState(templateInfo.installationState);
								}
								

								ImGui::EndTable();
							}
							ImGui::PopStyleVar();

							ImGui::EndChild();
						}
					}
					ImGui::EndGroup();
					break;
				}
				}

				ImGui::End();
			}

			// DeletePopub
			if (true) // just for visual studio
			{
				if (showDeletePopub) 
				{
					ImGui::OpenPopup("DeletePopub"); 
					showDeletePopub = false; 
				}

				ImGui::ScopedStyle ss(ImGuiStyleVar_WindowPadding, ImVec2{ 24, 24 } * scale);
				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImVec2 center = viewport->GetCenter();
				ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
				ImGui::SetNextWindowSize(ImVec2{ viewport->WorkSize.x * 0.3f, 0 });
				if (ImGui::BeginPopup("DeletePopub"))
				{
					if (!ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsAnyMouseDown())
					{
						submitedFunction = 0;
					}

					{
						ImGui::ScopedFont sf(FontType::Regular, FontSize::BodySmall);
						ImGui::TextWrapped(c_DeleteMessage);
					}

					ImGui::Dummy({ -1, 10 });
					ImGui::ShiftCursorX(ImGui::GetContentRegionAvail().x / 2 - (8 + ImGui::CalcTextSize("Cancel").x + ImGui::CalcTextSize("Delete").x + style.FramePadding.x * 4 + style.WindowPadding.x * 2) / 2);

					{
						ImGui::ScopedFont sf(FontType::Blod, FontSize::BodySmall);

						ImGui::ScopedStyle ss(ImGuiStyleVar_FramePadding, ImVec2{ 16, 4 } * scale);
						if (ImGui::Button("Cancel"))
						{
							submitedFunction = 0;
							ImGui::CloseCurrentPopup();
						}
						ImGui::SameLine();

						{
							ImGui::ScopedColor sc0(ImGuiCol_Button, colors[Color::Dangerous]);
							ImGui::ScopedColor sc1(ImGuiCol_ButtonHovered, colors[Color::DangerousHovered]);
							ImGui::ScopedColor sc2(ImGuiCol_ButtonActive, colors[Color::DangerousActive]);

							if (ImGui::Button("Delete"))
							{
								submitedFunction();
								submitedFunction = 0;
								showDeletePopub = false;
								ImGui::CloseCurrentPopup();
							}
						}
					}

					ImGui::EndPopup();
				}
			}

			if (showCreateNewProjectPopub)
			{
				constexpr ImGuiWindowFlags fullScreenWinFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
				{
					ImGui::SetNextWindowBgAlpha(0.8f);

					const ImGuiViewport* viewport = ImGui::GetMainViewport();
					ImGui::SetNextWindowPos(viewport->WorkPos);
					ImGui::SetNextWindowSize(viewport->WorkSize);
					ImGui::Begin("new Project Fullscreen window", nullptr, fullScreenWinFlags);
				}

				{
					ImGui::ScopedStyle wbs(ImGuiStyleVar_WindowBorderSize, 0.0f);
					ImGui::ScopedStyle wr(ImGuiStyleVar_WindowRounding, 4.0f);
					ImGui::ScopedStyle scopedWindowPadding(ImGuiStyleVar_WindowPadding, mainWindowPadding);
					ImGui::ScopedStyle scopedItemSpacing(ImGuiStyleVar_ItemSpacing, mainItemSpacing);

					const ImGuiViewport* viewport = ImGui::GetMainViewport();
					ImGui::SetNextWindowPos(viewport->WorkPos + viewport->WorkSize * 0.1f / 2);
					ImGui::SetNextWindowSize(viewport->WorkSize * 0.9f);
					ImGui::Begin("CreateNewProject", nullptr, fullScreenWinFlags);

					if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
						showCreateNewProjectPopub = false;

					{
						ImGui::BeginGroup();

						float buttomSize = 50 * scale;

						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, headWindowPadding);
							ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, headFramePadding);

							ImGui::BeginChild("Head", ImVec2(-ImGui::GetContentRegionAvail().x / 3, 0), ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY);

							// Page Titile
							{
								ImGui::ScopedFont sf(FontType::Blod, FontSize::Header0);

								switch (createProjectStage)
								{
								case CreateProjectStage::Templates: ImGui::TextUnformatted("Templates");  break;
								case CreateProjectStage::Plugins:  ImGui::TextUnformatted("Plugins");   break;
								}
							}

							// Search
							{
								float availableWidth = ImGui::GetContentRegionAvail().x;
								ImGui::SetNextItemWidth(availableWidth > 100.0f * scale ? availableWidth : 100.0f * scale);
								if (ImGui::InputTextWithHint("##Search", Icon_Search " Search...", textFilter.InputBuf, sizeof(textFilter.InputBuf)))
								{
									textFilter.Build();
								}
							}

							ImGui::EndChild();
						}

						// Stage Contanet
						switch (createProjectStage)
						{
						case CreateProjectStage::Templates:
						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, bodyWindowPadding);
							ImGui::BeginChild("Left", ImVec2(-ImGui::GetContentRegionAvail().x / 3, -buttomSize), ImGuiChildFlags_AlwaysUseWindowPadding);

							for (int i = 0; i < templates.size(); i++)
							{
								auto& t = templates[i].info;

								if (t.installationState != InstallationState::Installed)
									continue;

								if (!textFilter.PassFilter(t.name.c_str()) && !textFilter.PassFilter(t.description.c_str()))
									continue;

								ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, ImVec2(24, 24)* scale);
								ImGui::ScopedColor sc(ImGuiCol_ChildBg, colors[Color::ChildBlock]);

								if (selectedTemplate == i) ImGui::PushStyleColor(ImGuiCol_ChildBg, colors[Color::Selected]);
								ImGui::BeginChild(t.name.c_str(), { -1, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY);
								if (selectedTemplate == i) ImGui::PopStyleColor();

								if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
								{
									selectedTemplate = i;
								}

								{
									ImGui::ScopedFont sf(FontType::Blod, FontSize::Header2);
									ImGui::TextUnformatted(t.name.c_str());
								}
								ImGui::SameLine();


								ImGui::Dummy({ -1,10 });

								ImGui::TextWrapped(t.description.c_str());

								ImGui::EndChild();
							}

							if (installedTemplates == 0)
							{
								const char* m = "No templates have been installed yet.";
								ImGui::ScopedFont sf(FontType::Blod, FontSize::BodyMedium);
								ImGui::ScopedColor sc(ImGuiCol_Text, colors[Color::Warn]);
								ImGui::ShiftCursor(ImGui::GetContentRegionAvail() / 2 - ImGui::CalcTextSize(Icon_Warning) / 2);
								ImGui::TextWrapped(Icon_Warning);
								ImGui::ShiftCursorX(ImGui::GetContentRegionAvail().x / 2 - ImGui::CalcTextSize(m).x / 2);
								ImGui::TextWrapped(m);
							}

							ImGui::EndChild();

							break;
						}
						case CreateProjectStage::Plugins:
						{
							float scale = ImGui::GetIO().FontGlobalScale;

							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, bodyWindowPadding);
							ImGui::BeginChild("Plugins", ImVec2(-ImGui::GetContentRegionAvail().x / 3, -buttomSize), ImGuiChildFlags_AlwaysUseWindowPadding);

							for (auto& plugin : plugins)
							{
								if (plugin.info.installationState != InstallationState::Installed)
									continue;

								if (!textFilter.PassFilter(plugin.info.name.c_str()) && !textFilter.PassFilter(plugin.info.description.c_str()))
									continue;

								ImGui::ScopedColor sc(ImGuiCol_ChildBg, colors[Color::ChildBlock]);
								ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, ImVec2(24, 24) * scale);

								ImGui::BeginChild(plugin.info.name.c_str(), { -1, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeY);

								{
									ImGui::ScopedFont sf(FontType::Blod, FontSize::Header2);
									ImGui::TextUnformatted(plugin.info.name.c_str());
								}
								ImGui::SameLine();

								ImGui::ShiftCursorX(ImGui::GetContentRegionAvail().x - 30);
								ImGui::Checkbox("##plugin", &plugin.enabledByDefault);

								ImGui::Dummy({ -1, 10 });

								ImGui::TextWrapped(plugin.info.description.c_str());

								ImGui::EndChild();
							}

							if (installedPlugins == 0)
							{
								const char* m = "No plugins have been installed yet.";
								ImGui::ScopedFont sf(FontType::Blod, FontSize::BodyMedium);
								ImGui::ScopedColor sc(ImGuiCol_Text, colors[Color::Warn]);
								ImGui::ShiftCursor(ImGui::GetContentRegionAvail() / 2 - ImGui::CalcTextSize(Icon_Warning) / 2);
								ImGui::TextWrapped(Icon_Warning);
								ImGui::ShiftCursorX(ImGui::GetContentRegionAvail().x / 2 - ImGui::CalcTextSize(m).x / 2);
								ImGui::TextWrapped(m);
							}

							ImGui::EndChild();
							break;
						}
						}

						// Stage Buttons
						{
							ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, bodyWindowPadding);

							ImGui::BeginChild("buttom", { -ImGui::GetContentRegionAvail().x / 3, 0 }, ImGuiChildFlags_AlwaysUseWindowPadding);

							ImGui::SameLine();
							{
								float buttonSize = 100.0f;
								float buttonCount = 2;
								float size = (buttonSize * scale + style.FramePadding.x * 2.0f) * buttonCount;
								float avail = ImGui::GetContentRegionAvail().x;

								float off = (avail - size) * 0.5f;
								if (off > 0.0f)
									ImGui::ShiftCursorX(off);

								if (ImGui::SelectableButton("Templates", { buttonSize * scale, -1 }, createProjectStage == 0))
								{
									createProjectStage = CreateProjectStage::Templates;
								}

								ImGui::SameLine();

								if (ImGui::SelectableButton("PLugins", { buttonSize * scale, -1 }, createProjectStage == 1))
								{
									createProjectStage = CreateProjectStage::Plugins;
								}
							}
							ImGui::EndChild();
						}


						ImGui::EndGroup();
					}

					ImGui::SameLine();

					// Right
					{
						ImGui::ScopedStyle swp(ImGuiStyleVar_WindowPadding, ImVec2(8, 8) * scale);

						ImGui::BeginChild("Right", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding);

						bool validPath = std::filesystem::exists(projectPath);
						bool validName = !std::filesystem::exists(std::filesystem::path(projectPath) / projectName);
						bool validTemplate = selectedTemplate < installedTemplates;

						nvrhi::ITexture* t = nullptr;
						if(validTemplate)
						{
							nvrhi::ITexture* t = templates[selectedTemplate].thumbnail;
							HE_ASSERT(t);

							ImVec2 avail = ImGui::GetContentRegionAvail();
							float width = avail.x;
							float height = width * (9.0f / 16.0f);
							ImGui::Image(t, { width, height });

							ImGui::Dummy({ -1, 10 });
							ImGui::Separator();
							ImGui::Dummy({ -1, 10 });
						}

						{
							ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

							{
								ImGui::ScopedFont sf(FontType::Blod);
								ImGui::TextUnformatted("Project Name");
							}
							ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

							if (!validName) ImGui::PushStyleColor(ImGuiCol_Text, colors[Color::Error]);
							ImGui::InputTextWithHint("##Project Name", "Project Name", &projectName);
							if (!validName) ImGui::PopStyleColor();
						}

						ImGui::Dummy({ -1, 10 });

						{
							ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

							{
								ImGui::ScopedFont sf(FontType::Blod);
								ImGui::TextUnformatted("Location");
							}
							ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 50);

							if (!validPath) ImGui::PushStyleColor(ImGuiCol_Text, colors[Color::Error]);
							ImGui::InputTextWithHint("##Location", "Location", &projectPath);
							if (!validPath) ImGui::PopStyleColor();

							ImGui::SameLine();
							if (ImGui::Button(Icon_Folder, { -1,0 }))
							{
								auto path = FileDialog::SelectFolder().string();
								if (!path.empty())
									projectPath = path;
							}
						}

						ImGui::Dummy({ -1, 10 });

						{
							ImGui::ScopedFont sf(FontType::Blod);
							ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

							{
								ImGui::ScopedDisabled sd(!validName || !validPath || !validTemplate);
								ImGui::ScopedButtonColor sbs(colors[Color::PrimaryButton]);
								if (ImGui::Button("Create Project", { -1,0 }))
								{
									CreateNewProject(projectName, projectPath, selectedTemplate);
									showCreateNewProjectPopub = false;
								}
							}

							if (ImGui::Button("Cancel", { -1,0 }))
							{
								showCreateNewProjectPopub = false;
							}
						}

						ImGui::EndChild();
					}

					ImGui::End();
				}
				ImGui::End();
			}
		}

		// shortcuts
		if(true) // just for visual studio
		{
			if (Input::IsKeyDown(Key::LeftShift) && Input::IsKeyPressed(Key::Escape))
				Application::Restart();

			if (Input::IsKeyPressed(Key::Escape))
				Application::Shutdown();

			if (Input::IsKeyDown(Key::LeftShift) && Input::IsKeyReleased(Key::F))
				Application::GetWindow().ToggleScreenState();

			if (Input::IsKeyDown(Key::LeftShift) && Input::IsKeyReleased(Key::M))
			{
				if(!Application::GetWindow().IsMaximize())
					Application::GetWindow().MaximizeWindow();
				else
					Application::GetWindow().RestoreWindow();
			}
			
			if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed(Key::N) && installedInstances > 0)
				showCreateNewProjectPopub = true;

			if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed(Key::A) && installedInstances > 0)
				AddProject();
		}
	}

	void DrawInstallationState(uint8_t installationState)
	{
		switch (installationState)
		{
		case InstallationState::NotInstalled:
		{
			ImGui::ScopedColor sc(ImGuiCol_Text, colors[Color::Info]);
			ImGui::TextUnformatted(Icon_CloudDownload);
			ImGui::ToolTip("not installed yet");
			break;
		}
		case InstallationState::Failed:
		{
			ImGui::ScopedColor sc(ImGuiCol_Text, colors[Color::Error]);
			ImGui::TextUnformatted(Icon_X);
			ImGui::ToolTip("failed");
			break;
		}
		case InstallationState::Installing:
		{
			ImGui::ScopedColor sc(ImGuiCol_Text, colors[Color::Info]);
			ImGui::TextUnformatted(Icon_InfoCircle);
			ImGui::ToolTip("Installing");
			break;
		}
		case InstallationState::Build:
		{
			ImGui::ScopedColor sc(ImGuiCol_Text, colors[Color::Info]);
			ImGui::TextUnformatted(Icon_Build);
			ImGui::ToolTip("Building");
			break;
		}
		case InstallationState::Installed:
		{
			ImGui::ScopedColor sc(ImGuiCol_Text, colors[Color::Info]);
			ImGui::TextUnformatted(Icon_CheckCircle);
			ImGui::ToolTip("Installed");
			break;
		}
		case InstallationState::Wait:
		{
			ImGui::ScopedColor sc(ImGuiCol_Text, colors[Color::Info]);
			ImGui::TextUnformatted("- - -");
			break;
		}
		}
	}

#pragma endregion

#pragma region Commands and Helpers functions

	void IncludeSourceCodeForProject(Project& project)
	{
		project.includSourceCode = project.includSourceCode ? false : true;
		Serialize();
	}

	void RemoveProject(int index)
	{
		HE_VERIFY(index < projects.size());
		projects.erase(projects.begin() + index);
		Serialize();
	}

	bool IsVisualStudioInstalled()
	{
		// TODO : do real investigation
		return std::filesystem::exists(c_VSwherePath);
	}

	const Engine* GetEngineInsByID(const std::string_view& id)
	{
		for (auto& ins : instances)
			if (ins.id == id)
				return &ins;

		return nullptr;
	}

	void RunProjectPremake(Project& project)
	{
		auto ins = GetEngineInsByID(project.engineID);
		auto premakeDir = (ins->path / "ThirdParty" / "Premake" / c_System).string();
		auto projectPremake = (std::filesystem::path(project.path) / "premake.lua").string();

		bool b0 = std::filesystem::exists(project.path);
		bool b1 = std::filesystem::exists(premakeDir);
		bool b2 = std::filesystem::exists(projectPremake);
		bool b3 = std::filesystem::exists(ins->path.string());

		if (!b0 || !b1 || !b2 || !b3)
		{
			HE_ERROR(
				"invalid path \n{} : {}\n{} : {}\n{} : {}\n{} : {}", 
				project.path, b0, 
				premakeDir, b1, 
				projectPremake, b2, 
				ins->path.string(), b3
			);
			return;
		}
		
		std::string cmd = std::format(
			"premake5 --file=\"{}\" vs2022 --enginePath=\"{}\" --includSourceCode={}",
			projectPremake,
			ins->path.string(),
			project.includSourceCode ? "true" : "false"
		);

		Utils::ExecCommand(cmd.c_str(), nullptr, premakeDir.c_str(), false);
	}

	void ChangeEngineForProject(Project& project, Engine& InstanceInfo)
	{
		project.engineID = InstanceInfo.id;
		auto file = std::filesystem::path(project.path) / std::format("{}.hproject", project.name);

		SerializeProject(project);
	}

	void RemoveInstance(int index)
	{
		HE_VERIFY(index < instances.size());
		instances.erase(instances.begin() + index);
		Serialize();
	}

	void AddInstance()
	{
		auto& ins = instances.emplace_back();
		ins.path = "path";
		ins.installationState = InstallationState::NotInstalled;

		Serialize();
	}

	void DeleteEngine(Engine& InstanceInfo, bool removeFromList = false)
	{
		if(std::filesystem::exists(InstanceInfo.path))
		{
			InstanceInfo.installationState = InstallationState::Wait;

			Jops::SubmitTask([this, removeFromList,&InstanceInfo]()
			{
				FileSystem::Delete(InstanceInfo.path); 

				if (removeFromList)
				{
					for (int i = 0; i < instances.size(); i++)
						if (&InstanceInfo == &instances[i])
							instances.erase(instances.begin() + i);
				}
				else
				{
					InstanceInfo.installationState = InstallationState::NotInstalled;
				}
				Serialize();
			});
		}
	}

	void Delete(Info& info, RemoteType type)
	{
		Jops::SubmitTask([this, &info, type](){

			std::filesystem::path path;
			switch (type)
			{
			case RemoteType::Plugin: path = pluginsDir / info.name; installedPlugins--; break;
			case RemoteType::Template: path = templatesDir / info.name; installedTemplates--; break;
			}

			{
				info.installationState = InstallationState::Wait;
				FileSystem::Delete(path);
				info.installationState = InstallationState::NotInstalled;
			}
		});
	}

	void PickEngineDirectory(Engine& instanceInfo)
	{
		auto path = FileDialog::SelectFolder();
		if (!path.empty())
		{
			// exsting 
			if (IsValidHydraDirectory(path))
			{
				instanceInfo.path = std::filesystem::absolute(path);
				instanceInfo.installationState = InstallationState::Installed;
				instanceInfo.id = Git::GetCurrentCommitId(instanceInfo.path);
			}
			else // new
			{
				instanceInfo.path = std::filesystem::absolute(path) / "HydraEngine";
				instanceInfo.installationState = InstallationState::NotInstalled;
			}

			Serialize();
		}
	}

	void BuildEngine(Engine& instanceInfo)
	{
		if (!std::filesystem::exists(instanceInfo.path))
			return;

		Jops::SubmitTask([this, &instanceInfo](){

			instanceInfo.installationState = InstallationState::Build;
			instanceInfo.progress.fetchProgress.total_objects = 5;
			instanceInfo.progress.fetchProgress.received_objects = 0;
			instanceInfo.progress.totalSteps++;
			instanceInfo.progress.completedSteps++;

			{
				auto premake = (instanceInfo.path / "ThirdParty" / "Premake" / c_System).string();
				auto enginePremake = instanceInfo.path / "premake.lua";

				instanceInfo.progress.stepName = "Setup...";

				std::string cmd = std::format("premake5{} --file=\"{}\" vs2022", c_ExecutableExtension, enginePremake.string());
				auto originalPath = std::filesystem::current_path();
				std::filesystem::current_path(premake);
				std::system(cmd.c_str());
				std::filesystem::current_path(originalPath);

				instanceInfo.progress.fetchProgress.received_objects++;
			}

			{
				static const char* config[] = { "Debug", "Release", "Profile", "Dist" };
				for (int i = 0; i < 4; i++)
				{
					if (instanceInfo.progress.cloneState == Git::CloneState::Canceled)
						break;

					instanceInfo.progress.stepName = std::format("Build {}", config[i]);
					std::string cmd = std::format(
						"MSBuild \"{}/HydraEngine.sln\" /p:Configuration={} /verbosity:minimal",
						instanceInfo.path.string(),
						config[i]
					);

					auto originalPath = std::filesystem::current_path();
					std::filesystem::current_path(msBuildPath);
					std::system(cmd.c_str());
					std::filesystem::current_path(originalPath);

					instanceInfo.progress.fetchProgress.received_objects++;
				}
			}

			instanceInfo.installationState = InstallationState::Installed;
			Serialize();

			instanceInfo.progress = {};
		});
	}

	void BuildProject(Project& proj, uint8_t config)
	{
		if (!std::filesystem::exists(proj.path))
			return;

		Jops::SubmitTask([this, &proj, config]() {

			proj.isBuilding = true;

			auto sln = std::filesystem::path(proj.path) / (proj.name + ".sln");
			
			if (!std::filesystem::exists(sln))
			{
				RunProjectPremake(proj);
			}

			if (!std::filesystem::exists(sln))
			{
				HE_ERROR("project not exist {}", sln.string());
				return;
			}

			// build
			std::string cmd = std::format("MSBuild \"{}\" /p:Configuration={} /verbosity:minimal", sln.string(), c_ConfigStr[config]);
			proj.process.Start(cmd.c_str(), showBuildOutput, msBuildPath.c_str());
			proj.process.Wait();

			proj.isBuilding = false;

			auto projPath = std::filesystem::path(proj.path);
			std::filesystem::path BuildDir = projPath / "Build" / std::format("{}-{}", c_System, c_Architecture) / c_ConfigStr[config] / "Bin";
			std::filesystem::path projectPluginsDir = projPath / "Plugins";
			std::filesystem::path projectResources = projPath / "Resources";
			
			std::filesystem::path BuildTargetDir = projPath / "Build" / "Out" / c_ConfigStr[config];
			std::filesystem::path pluginBin = std::filesystem::path("Binaries") / std::format("{}-{}", c_System, c_Architecture) / c_ConfigStr[config];
			
			std::filesystem::path currentOutputDir;

			if (proj.buildDir.empty())
				currentOutputDir = BuildTargetDir;
			else
				currentOutputDir = proj.buildDir / c_ConfigStr[config];

			std::filesystem::create_directories(currentOutputDir);
			
			if (!std::filesystem::exists(BuildDir))
			{
				HE_ERROR("project BuildDir not exist {}", BuildDir.string());
				return;
			}

			auto copyOptions = std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing;

			// copy app binaries
			for (const auto& entry : std::filesystem::directory_iterator(BuildDir))
			{
				if (entry.is_regular_file() && entry.path().extension() != ".exp" && entry.path().extension() != ".lib" && entry.path().extension() != ".pdb")
				{
					FileSystem::Copy(entry.path(), currentOutputDir, copyOptions);
				}
			}

			// copy Resources
			if(std::filesystem::exists(projectResources))
				FileSystem::Copy(projectResources, currentOutputDir / "Resources", copyOptions);

			// copy plugins
			if (std::filesystem::exists(projectPluginsDir))
			{
				for (const auto& entry : std::filesystem::directory_iterator(projectPluginsDir))
				{
					auto name = entry.path().stem();
					auto binDir = entry.path() / pluginBin;

					auto pluginOutDir = currentOutputDir / "Plugins" / name;
					auto pluginOutBinaries = pluginOutDir / pluginBin;

					std::filesystem::create_directories(pluginOutBinaries);

					// copy desc
					auto pluginsDescFilePath = entry.path() / (entry.path().stem().string() + Plugins::c_PluginDescriptorExtension);
					FileSystem::Copy(pluginsDescFilePath, pluginOutDir, copyOptions);

					// copy Assets
					auto assetsDir = entry.path() / "Assets";
					if (std::filesystem::exists(assetsDir))
						FileSystem::Copy(assetsDir, pluginOutDir / "Assets", copyOptions);

					// copy plugins binaries
					if (std::filesystem::exists(binDir))
					{
						for (const auto& e : std::filesystem::directory_iterator(binDir))
						{
							if (e.is_regular_file() && e.path().extension() == c_SharedLibExtension)
							{
								FileSystem::Copy(e.path(), pluginOutBinaries, copyOptions);
							}
						}
					}
					else
					{
						HE_ERROR("{} not exists", binDir.string());
					}
				}
			}

			Utils::CopyDepenDlls(currentOutputDir, !(bool)config);

			if(openOutputDirAfterProjectBuild && std::filesystem::exists(currentOutputDir))
				FileSystem::Open(currentOutputDir);
			
			auto executable = currentOutputDir / (proj.name + c_ExecutableExtension);
			auto executableStr = std::format("\"{}\"", executable.string());
			if (buildAndRunProject && std::filesystem::exists(executable))
				Utils::ExecCommand(executableStr.c_str(),nullptr, currentOutputDir.string().c_str(), true);
		});
	}

	void DownLoadEngine(Engine& instanceInfo)
	{
		if (!std::filesystem::exists(instanceInfo.path.parent_path()))
			return;

		Jops::SubmitTask([this,&instanceInfo]()
		{
			// 1. clone the engine
			instanceInfo.installationState = InstallationState::Installing;
			instanceInfo.progress.totalSteps = 3;
			instanceInfo.progress.stepName = "HydraEngine";
			auto state = Git::RecursiveClone(c_EngineRemoteRepo, instanceInfo.path.string().c_str(), instanceInfo.progress);
			switch (state)
			{
			case Git::CloneState::Completed:
			{
				instanceInfo.id = Git::GetCurrentCommitId(instanceInfo.path.string());
				break;
			}
			case Git::CloneState::Canceled:
			{
				instanceInfo.installationState = InstallationState::NotInstalled;
				DeleteEngine(instanceInfo);
				break;
			}
			case Git::CloneState::Faild:
			{
				instanceInfo.installationState = InstallationState::Failed;
				DeleteEngine(instanceInfo);
				break;
			}
			case Git::CloneState::None:
			case Git::CloneState::Cloning: HE_ASSERT(false); break;
			default: HE_ASSERT(false); break;
			}

			// 2. clone libs
			if(state == Git::CloneState::Completed)
			{
				instanceInfo.progress.stepName = "ThirdParty/Lib (this could take a while)";
				auto lib = instanceInfo.path / "ThirdParty" / "Lib";
				state = Git::RecursiveClone(c_EngineLibRemoteRepo, lib.string().c_str(), instanceInfo.progress);
				switch (state)
				{
				case Git::CloneState::Completed:
				{
					for (auto file : std::filesystem::directory_iterator(lib))
					{
						if (file.is_regular_file() && file.path().extension() == ".zip")
						{
							auto& zip = file.path();
							instanceInfo.progress.stepName = "Extracting " + zip.stem().string() + " ...";
							FileSystem::ExtractZip(zip, lib);
							FileSystem::Delete(zip);
							instanceInfo.installationState = InstallationState::Installed;
							Serialize();
						}
					}
					break;
				}
				case Git::CloneState::Canceled:
				{
					instanceInfo.installationState = InstallationState::NotInstalled;
					DeleteEngine(instanceInfo);
					break;
				}
				case Git::CloneState::Faild:
				{
					instanceInfo.installationState = InstallationState::Failed;
					DeleteEngine(instanceInfo);
					break;
				}
				case Git::CloneState::None:
				case Git::CloneState::Cloning: HE_ASSERT(false); break;
				default: HE_ASSERT(false); break;
				}
			}

			instanceInfo.progress = { {0} };

			// 3. build all config
			if (state == Git::CloneState::Completed)
			{
				BuildEngine(instanceInfo);
			}
		});
	}

	void DownLoad(Info& info, RemoteType type)
	{
		Jops::SubmitTask([this, &info, type](){
		
			std::filesystem::path path;
			switch (type)
			{
			case RemoteType::Plugin: path = pluginsDir / info.name; break;
			case RemoteType::Template: path = templatesDir / info.name; break;
			}


			info.installationState = InstallationState::Installing;
			info.progress.totalSteps = 1;
			info.progress.stepName = info.name;

			auto state = Git::RecursiveClone(info.URL.c_str(), path.string().c_str(), info.progress);
			switch (state)
			{
			case Git::CloneState::Completed:
			{
				info.installationState = InstallationState::Installed;
				break;
			}
			case Git::CloneState::Canceled:
			{
				Delete(info, type);
				break;
			}
			case Git::CloneState::Faild:
			{
				info.installationState = InstallationState::Failed;
				Delete(info, type);
				break;
			}
			case Git::CloneState::None:
			case Git::CloneState::Cloning: HE_ASSERT(false); break;
			default: HE_ASSERT(false); break;
			}

			switch (type)
			{
			case RemoteType::Plugin: installedPlugins++; break;
			case RemoteType::Template: installedTemplates++; break;
			}

			info.progress = { {0} };
		});
	}

	bool IsPluginInstalled(const std::string_view& pluginName)
	{
		for (auto& plugin : plugins)
			if (pluginName == plugin.info.name)
				return true;
		return false;
	}

	Template* GetTemplateByName(const std::string_view& pluginName)
	{
		for (auto& t : templates)
			if (pluginName == t.info.name)
				return &t;
		return nullptr;
	}

	void FindAndAddTemplates()
	{
		templates.clear();
		for (auto entry : std::filesystem::directory_iterator(templatesDir))
		{
			auto& t = templates.emplace_back();
			DeserializeTemplate(entry.path() / "config.json", t);
			installedTemplates++;
		}
	}

	void FindAndAddPlugins()
	{
		plugins.clear();
		for (auto entry : std::filesystem::directory_iterator(pluginsDir))
		{
			auto descFile = entry.path() / std::format("{}.hplugin", entry.path().stem().string());

			if (std::filesystem::exists(descFile))
			{
				auto& p = plugins.emplace_back();
				DeserializePluginDesc(descFile, p);
				installedPlugins++;
			}
		}
	}

	void GetRemoteInfo()
	{
		Jops::SubmitTask([this]() {
			
			// Load from existing file
			if (std::filesystem::exists(remoteInfoFilePath))
			{
				DeserializeAndAddRemoteInfo();
			}

			// Fetch updated file , Reload from updated file
			{
				std::string cmd = std::format("curl -L -o \"{}\" \"{}\"", remoteInfoFilePath.string(), c_RemotePluginsURL); // >nul 2>&1
				Utils::ExecCommand(cmd.c_str(), nullptr, nullptr, true, [this]() {
					DeserializeAndAddRemoteInfo();
				});
			}
		});
	}

	bool IsValidHydraDirectory(const std::filesystem::path& path)
	{
		bool valid = true;

		valid &= std::filesystem::exists(path);
		valid &= std::filesystem::exists(path / "premake.lua");
		valid &= std::filesystem::exists(path / "build.lua");
		valid &= path.stem() == "HydraEngine";

		return valid;
	}

	void CreateNewProject(const std::string& name, const std::string& newProjectPath, int index)
	{
		Jops::SubmitTask([this, &name, &newProjectPath, index]() {

			auto& t = templates[index];

			std::string normalizedName = name;
			normalizedName.erase(std::remove(normalizedName.begin(), normalizedName.end(), ' '), normalizedName.end());

			std::filesystem::path newProjectDirectory = std::filesystem::path(newProjectPath) / name;
			std::filesystem::path premake = newProjectDirectory / "premake.lua";
			std::filesystem::path source = newProjectDirectory / "Source";
			std::filesystem::path cppFile = source / t.info.name / std::format("{}.cpp", t.info.name);
			std::filesystem::path projectPluginDir = newProjectDirectory / "Plugins";

			if (std::filesystem::exists(newProjectDirectory))
			{
				HE_ERROR("project directory already exists {}", newProjectDirectory.string());
				return;
			}

			std::filesystem::create_directories(projectPluginDir);

			FileSystem::Copy(templatesDir / t.info.name, newProjectDirectory);
			FileSystem::Delete(newProjectDirectory / "thumbnail.jpg");
			FileSystem::Delete(newProjectDirectory / "config.json");
			FileSystem::Delete(newProjectDirectory / ".git");

			FileSystem::GenerateFileWithReplacements(premake, premake, { { "PROJECT_NAME", name } });
			FileSystem::GenerateFileWithReplacements(cppFile, cppFile, { { "PROJECT_NAME", normalizedName } });

			FileSystem::Rename(cppFile, source / t.info.name / std::format("{}.cpp", normalizedName));
			FileSystem::Rename(source / t.info.name, source / normalizedName);

			for (int i = 0; i < plugins.size(); i++)
			{
				if (!plugins[i].enabledByDefault)
					continue;

				FileSystem::Copy(pluginsDir / plugins[i].info.name, projectPluginDir / plugins[i].info.name);
			}

			Project* proj = nullptr;
			{
				std::lock_guard<std::mutex> lock(projectsMutex);
				proj = &projects.emplace_back();
				proj->name = name;
				proj->path = newProjectDirectory.string();
				proj->engineID = instances.size() >= 1 ? instances[0].id : "";
			}

			Serialize();
			SerializeProject(*proj);
		});
	}

	void AddProject()
	{
		auto projectDir = FileDialog::SelectFolder().lexically_normal();
		if (projectDir.empty())
			return;

		if (!std::filesystem::exists(projectDir / "premake.lua") && !std::filesystem::exists(projectDir / "Source"))
			return;

		auto& proj = projects.emplace_back();
		proj.name = projectDir.stem().string();
		proj.path = projectDir.string();

		DeserializeProject(proj);

		Serialize();
	}

	void Serialize()
	{
		HE_PROFILE_FUNCTION();

		std::ofstream file(databaseFilePath);
		if (!file.is_open())
		{
			HE_ERROR("Unable to open file for writing, {}" , databaseFilePath.string());
		}

		std::ostringstream oss;
		oss << "{\n";
		
		{
			oss << "\t\"ui\" : {\n";
			oss << "\t\t\"fontScale\" : " << ImGui::GetIO().FontGlobalScale << ",\n";
			oss << "\t\t\"selectedPage\" : " << (int)selectedPage << "\n";
			oss << "\t},\n";
		}

		{
			oss << "\t\"settings\" : {\n";
			oss << "\t\t\"openOutputDirAfterProjectBuild\" : " << (openOutputDirAfterProjectBuild ? "true" : "false") << ",\n";
			oss << "\t\t\"buildAndRunProject\" : " << (buildAndRunProject ? "true" : "false") << ",\n";
			oss << "\t\t\"showBuildOutput\" : " << (showBuildOutput ? "true" : "false") << "\n";
			oss << "\t},\n";
		}

		{
			oss << "\t\"engine\" : [\n";
			for (size_t i = 0; i < instances.size(); ++i)
			{
				uint8_t state = instances[i].installationState == InstallationState::Installed ? InstallationState::Installed : InstallationState::NotInstalled;

				oss << "\t\t{\n";
				oss << "\t\t\t\"path\" : " << instances[i].path << ",\n";
				oss << "\t\t\t\"state\" : \"" << InstallationState::ToString(instances[i].installationState) << "\"\n";
				oss << "\t\t}";
				if (i < instances.size() - 1) oss << ",";
				oss << "\n";
			}
			oss << "\t],\n";
		}
		
		{
			oss << "\t\"projects\" : [\n";
			for (size_t i = 0; i < projects.size(); ++i)
			{
				oss << "\t\t{\n";
				oss << "\t\t\t\"name\" : \"" << projects[i].name << "\",\n";
				oss << "\t\t\t\"path\" : " << std::filesystem::path(projects[i].path).lexically_normal() << ",\n";
				oss << "\t\t\t\"includSourceCode\" : " << (projects[i].includSourceCode ? "true" : "false") << "\n";
				oss << "\t\t}";
				if (i < projects.size() - 1) oss << ",";
				oss << "\n";
			}
			oss << "\t]\n";
		}

		oss << "}\n";

		file << oss.str();
		file.close();

		std::string layoutPath = (appData / "layout.ini").lexically_normal().string();
		ImGui::GetIO().IniFilename = nullptr;
		ImGui::SaveIniSettingsToDisk(layoutPath.c_str());
	}

	void Deserialize()
	{
		HE_PROFILE_FUNCTION();

		static simdjson::dom::parser parser;
		auto doc = parser.load(databaseFilePath.string());

		// ui
		{
			auto ui = doc["ui"];
			if (!ui.error())
			{
				ImGui::GetIO().FontGlobalScale = (float)ui["fontScale"].get_double().value();
				selectedPage = (int)ui["selectedPage"].get_int64().value();
			}
		}

		// settings
		{
			auto settings = doc["settings"];
			if (!settings.error())
			{
				openOutputDirAfterProjectBuild = settings["openOutputDirAfterProjectBuild"].get_bool().value();
				buildAndRunProject = settings["buildAndRunProject"].get_bool().value();
				showBuildOutput = settings["showBuildOutput"].get_bool().value();
			}
		}

		// engine
		{
			auto engine = doc["engine"].get_array();
			if (!engine.error())
			{
				installedInstances = 0;
				for (auto e : engine)
				{
					auto path = e["path"].get_c_str().value();
					auto state = e["state"].get_c_str().value();

					auto& ins = instances.emplace_back();
					ins.path = path;
					ins.installationState = InstallationState::fromString(state);

					if (IsValidHydraDirectory(path))
					{
						ins.id = Git::GetCurrentCommitId(path);
						installedInstances++;
					}
					else
					{
						ins.installationState = InstallationState::NotInstalled;
					}
				}
			}
		}

		// projects
		auto loadedProjects = doc["projects"].get_array();
		if (!loadedProjects.error())
		{
			projects.reserve(loadedProjects.size());
			for (auto p : loadedProjects)
			{
				auto& proj = projects.emplace_back();

				proj.name = p["name"].get_c_str().value();
				proj.path = p["path"].get_c_str().value();
				proj.includSourceCode = !p["includSourceCode"].error() ? p["includSourceCode"].get_bool().value() : proj.includSourceCode;

				if (std::filesystem::exists(proj.path))
				{
					DeserializeProject(proj);
				}
			}
		}
	}

	void SerializeProject(const Project& project)
	{
		HE_PROFILE_FUNCTION();

		auto filePath = std::filesystem::path(project.path) / std::format("{}.hproject", project.name);

		std::ofstream file(filePath);
		if (!file.is_open())
		{
			HE_ERROR("Unable to open file for writing, {}", filePath.string());
		}

		std::ostringstream oss;
		oss << "{\n";
		
		{
			oss << "\t\"engineID\" : \"" << project.engineID << "\"\n";
		}

		oss << "}\n";

		file << oss.str();
		file.close();
	}

	void DeserializeProject(Project& project)
	{
		HE_PROFILE_FUNCTION();

		auto filePath = std::filesystem::path(project.path) / std::format("{}.hproject", project.name);
		
		static simdjson::dom::parser parser;
		auto doc = parser.load(filePath.string());

		if (doc.error())
			return;

		if (!doc["engineID"].error())
		{
			project.engineID = doc["engineID"].get_c_str().value();
		}
	}

	void DeserializePluginDesc(const std::filesystem::path& filePath, Plugin& desc)
	{
		HE_PROFILE_FUNCTION();

		static simdjson::dom::parser parser;
		auto doc = parser.load(filePath.string());

		if (doc.error())
			return;

		std::string_view name;
		desc.info.name = (doc["name"].get(name) != simdjson::SUCCESS) ? "" : name;

		std::string_view description;
		desc.info.description = (doc["description"].get(description) != simdjson::SUCCESS) ? "" : description;

		std::string_view URL;
		desc.info.URL = (doc["URL"].get(URL) != simdjson::SUCCESS) ? "" : URL;

		desc.info.installationState = InstallationState::Installed;
	}

	void DeserializeTemplate(const std::filesystem::path& filePath, Template& temp)
	{
		HE_PROFILE_FUNCTION();

		static simdjson::dom::parser parser;
		auto doc = parser.load(filePath.string());

		if (doc.error())
			return;

		nvrhi::TextureHandle texture;

		std::filesystem::path thumbnailPath = filePath.parent_path() / "thumbnail.jpg";
		if (std::filesystem::exists(thumbnailPath))
			texture = Utils::LoadTexture(thumbnailPath, device, commandList);

		temp.info.name = doc["name"].get_c_str().value();
		temp.info.description = doc["description"].get_c_str().value();
		temp.info.URL = doc["URL"].get_c_str().value();
		temp.info.installationState = InstallationState::Installed;
		temp.thumbnail = texture;
	}

	void DeserializeAndAddRemoteInfo()
	{
		HE_PROFILE_FUNCTION();

		static simdjson::dom::parser parser;

		auto doc = parser.load(remoteInfoFilePath.string());

		auto pluginsArray = doc["plugins"].get_array();
		if (!pluginsArray.error())
		{
			plugins.reserve(pluginsArray.size());
			for (auto p : pluginsArray)
			{
				Plugin plugin;
				plugin.info.name = p["name"].get_c_str().value();
				plugin.info.description = p["description"].get_c_str().value();
				plugin.info.URL = p["URL"].get_c_str().value();

				if (IsPluginInstalled(plugin.info.name))
					continue;

				std::lock_guard<std::mutex> lock(pluginsMutex);
				plugins.push_back(std::move(plugin));
			}
		}

		auto templatesArray = doc["templates"].get_array();
		if (!templatesArray.error())
		{
			std::lock_guard<std::mutex> lock(templatesMutex);
			templates.reserve(templatesArray.size());
			for (auto t : templatesArray)
			{
				auto installedTemplate = GetTemplateByName(t["name"].get_c_str().value());
				if (installedTemplate)
				{
					installedTemplate->info.name = t["name"].get_c_str().value();
					installedTemplate->info.description = t["description"].get_c_str().value();
					installedTemplate->info.URL = t["URL"].get_c_str().value();
				}
				else
				{
					auto& ins = templates.emplace_back();
					ins.info.name = t["name"].get_c_str().value();
					ins.info.description = t["description"].get_c_str().value();
					ins.info.URL = t["URL"].get_c_str().value();
				}
			}
		}
	}
	
#pragma endregion

};

HE::ApplicationContext* HE::CreateApplication(ApplicationCommandLineArgs args)
{
	HE_PROFILE_FUNCTION();

	ApplicationDesc desc;
	desc.deviceDesc.api = {
		nvrhi::GraphicsAPI::D3D11,
		nvrhi::GraphicsAPI::D3D12,
		nvrhi::GraphicsAPI::VULKAN,
	};

	auto log = (Utils::GetAppDataPath(c_AppName) / c_AppName).string();

	desc.windowDesc.customTitlebar = true;
	desc.windowDesc.iconFilePath = c_AppIconPath;
	desc.windowDesc.title = c_AppName;
	desc.windowDesc.minWidth = 960;
	desc.windowDesc.minHeight = 540;
	desc.logFile = log.c_str();

	ApplicationContext* ctx = new ApplicationContext(desc);
	Application::PushLayer(new HydraLauncher());

	return ctx;
}

#include "HydraEngine/EntryPoint.h"