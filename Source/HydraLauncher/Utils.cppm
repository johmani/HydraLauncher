module;

#include "HydraEngine/Base.h"


#ifdef HE_PLATFORM_WINDOWS
#include <windows.h>
#endif

export module Utils;

import HE;
import std;
import Math;
import nvrhi;
import ImGui;

export namespace Utils {

	class Process
	{
	public:
		~Process();
	
		bool Start(const char* command, bool showOutbut = false, const char* workingDir = nullptr);
		void Wait();
		void Kill();

	private:
		void* hProcess;
		void* hThread;
		uint32_t dwProcessId;
		uint32_t dwThreadId;
	};
	
	bool ExecCommand(const char* command, std::string* output = nullptr, const char* workingDir = nullptr, bool async = false, const std::function<void()>& onComplete = {});

	enum class AppDataType 
	{
		Roaming,
		Local
	};

	std::filesystem::path GetAppDataPath(const std::string& appName, AppDataType type = AppDataType::Roaming)
	{
#ifdef HE_PLATFORM_WINDOWS
		const char* userProfile = std::getenv("USERPROFILE");
		if (!userProfile) return {};
		std::filesystem::path base(userProfile);

		std::filesystem::path appDataPath;
		switch (type)
		{
		case AppDataType::Roaming: {
			const char* appdata = std::getenv("APPDATA");
			appDataPath = appdata ? std::filesystem::path(appdata) : (base / "AppData" / "Roaming");
			break;
		}
		case AppDataType::Local: {
			const char* localAppData = std::getenv("LOCALAPPDATA");
			appDataPath = localAppData ? std::filesystem::path(localAppData) : (base / "AppData" / "Local");
			break;
		}
		}
#else
		const char* home = std::getenv("HOME");
		if (!home) return {};
		std::filesystem::path base(home);

		std::filesystem::path appDataPath;
		switch (type) {
		case AppDataType::Roaming:
		case AppDataType::Local: {
			const char* config = std::getenv("XDG_CONFIG_HOME");
			appDataPath = config ? std::filesystem::path(config) : (base / ".config");
			break;
		}
		}
#endif

		appDataPath /= appName;

		std::filesystem::create_directories(appDataPath);

		return appDataPath;
	}

	void CopyDepenDlls(const std::filesystem::path& to, bool debug)
	{
		const char* dlls[] = {
			"vcruntime140_1.dll", 
			"vcruntime140.dll", 
			"msvcp140_atomic_wait.dll", 
			"msvcp140.dll"
		};

		if (debug)
		{
			dlls[0] = "vcruntime140_1d.dll";
			dlls[1] = "vcruntime140d.dll";
			dlls[2] = "ucrtbased.dll";
			dlls[3] = "msvcp140d.dll";
		}
		
		for (auto name : dlls)
		{
			std::filesystem::copy_file("C:/Windows/System32/" + std::string(name), to / name, std::filesystem::copy_options::overwrite_existing);
		}
	}

	const char* GetLastWriteTime(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
			return "- - -  - - -  - - -  - - -  - - -";

		static char buffer[20]; // Buffer for formatted time
		auto t = std::filesystem::last_write_time(path);
		auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(t);
		std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
		std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&tt));
		return buffer;
	}

	nvrhi::TextureHandle LoadTexture(const std::filesystem::path filePath, nvrhi::IDevice* device, nvrhi::ICommandList* commandList)
	{
		HE::Image image(filePath);
		nvrhi::TextureDesc desc;
		desc.width = image.GetWidth();
		desc.height = image.GetHeight();
		desc.format = nvrhi::Format::RGBA8_UNORM;
		desc.debugName = filePath.string();

		auto texture = device->createTexture(desc);
		commandList->beginTrackingTextureState(texture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common);
		commandList->writeTexture(texture, 0, 0, image.GetData(), desc.width * 4);
		commandList->setPermanentTextureState(texture, nvrhi::ResourceStates::ShaderResource);
		commandList->commitBarriers();

		return texture;
	}

	nvrhi::TextureHandle LoadTexture(HE::Buffer buffer, nvrhi::IDevice* device, nvrhi::ICommandList* commandList)
	{
		HE::Image image(buffer);

		nvrhi::TextureDesc desc;
		desc.width = image.GetWidth();
		desc.height = image.GetHeight();
		desc.format = nvrhi::Format::RGBA8_UNORM;

		nvrhi::TextureHandle texture = device->createTexture(desc);
		commandList->beginTrackingTextureState(texture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common);
		commandList->writeTexture(texture, 0, 0, image.GetData(), desc.width * 4);
		commandList->setPermanentTextureState(texture, nvrhi::ResourceStates::ShaderResource);
		commandList->commitBarriers();

		return texture;
	}

	ImVec4 ImLerp(const ImVec4& a, const ImVec4& b, float t) { return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t); }

	void Theme()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0,0,0,1);
		colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.15f, 0.15f, 1);
		colors[ImGuiCol_PopupBg] = ImVec4(0.094118f, 0.094118f, 0.094118f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.329412f, 0.329412f, 0.329412f, 1.0f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.429412f, 0.429412f, 0.429412f, 1.0f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.133333f, 0.133333f, 0.133333f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.113725f, 0.113725f, 0.113725f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.113725f, 0.113725f, 0.113725f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.113725f, 0.113725f, 0.113725f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.113725f, 0.113725f, 0.113725f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.329412f, 0.329412f, 0.329412f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.429412f, 0.429412f, 0.429412f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.278431f, 0.701961f, 0.447059f, 1.00f);
		colors[ImGuiCol_CheckMark] = ImVec4(0.278431f, 0.701961f, 0.447059f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.278431f, 0.801961f, 0.547059f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.278431f, 0.701961f, 0.447059f, 1.00f);
		colors[ImGuiCol_Button] = ImVec4(0.329412f, 0.329412f, 0.329412f, 1.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.429412f, 0.429412f, 0.429412f, 1.0f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.278431f, 0.701961f, 0.447059f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.329412f, 0.329412f, 0.329412f, 1.0f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.429412f, 0.429412f, 0.429412f, 1.0f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.278431f, 0.701961f, 0.447059f, 1.00f);
		colors[ImGuiCol_Separator] = ImVec4(0.329412f, 0.329412f, 0.329412f, 1.0f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.429412f, 0.429412f, 0.429412f, 1.0f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.278431f, 0.701961f, 0.447059f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.329412f, 0.329412f, 0.329412f, 1.0f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.429412f, 0.429412f, 0.429412f, 1.0f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.278431f, 0.701961f, 0.447059f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.213725f, 0.213725f, 0.213725f, 1.0f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.429412f, 0.429412f, 0.429412f, 1.0f);
		colors[ImGuiCol_TabSelected] = ImVec4(0.288235f, 0.288235f, 0.288235f, 1.00f);
		colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
		colors[ImGuiCol_TabDimmed] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
		colors[ImGuiCol_TabDimmedSelected] = ImLerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
		colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
		colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_HeaderActive] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.278431f, 0.701961f, 0.447059f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

		style.WindowRounding = 3.0f;
		style.ChildRounding = 3.0f;
		style.FrameRounding = 2.0f;
		style.PopupRounding = 2.0f;
		style.GrabRounding = 3.0f;
		style.TabRounding = 3.0f;
		style.HoverStationaryDelay = 0.4f;
		style.WindowBorderSize = 1.0f;
		style.ScrollbarSize = 10.0f;
		style.ScrollbarRounding = 4.0f;
		style.FrameBorderSize = 0.0f;
	}

	bool BeginMainMenuBar(bool customTitlebar, ImTextureRef icon, ImTextureRef close, ImTextureRef min, ImTextureRef max, ImTextureRef res)
	{
		if (customTitlebar)
		{
			float dpi = ImGui::GetWindowDpiScale();
			float scale = ImGui::GetIO().FontGlobalScale * dpi;
			ImGuiStyle& style = ImGui::GetStyle();
			static float yframePadding = 8.0f * dpi;
			bool isMaximized = HE::Application::GetWindow().IsMaximize();
			auto& title = HE::Application::GetApplicationDesc().windowDesc.title;
			bool isIconClicked = false;

			{
				ImGui::ScopedStyle fbs(ImGuiStyleVar_FrameBorderSize, 0.0f);
				ImGui::ScopedStyle wbs(ImGuiStyleVar_WindowBorderSize, 0.0f);
				ImGui::ScopedStyle wr(ImGuiStyleVar_WindowRounding, 0.0f);
				ImGui::ScopedStyle wp(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 6.0f * dpi));
				ImGui::ScopedStyle fp(ImGuiStyleVar_FramePadding, ImVec2{ 0, yframePadding });
				ImGui::ScopedColor scWindowBg(ImGuiCol_WindowBg, ImVec4{ 1.0f, 0.0f, 0.0f, 0.0f });
				ImGui::ScopedColor scMenuBarBg(ImGuiCol_MenuBarBg, ImVec4{ 0.0f, 1.0f, 0.0f, 0.0f });
				ImGui::BeginMainMenuBar();
			}

			const ImVec2 windowPadding = style.WindowPadding;
			const ImVec2 titlebarMin = ImGui::GetCursorScreenPos() - ImVec2(windowPadding.x,0);
			const ImVec2 titlebarMax = ImGui::GetCursorScreenPos() + ImGui::GetWindowSize() - ImVec2(windowPadding.x, 0);
			auto* fgDrawList = ImGui::GetForegroundDrawList();
			auto* bgDrawList = ImGui::GetBackgroundDrawList();

			ImGui::SetCursorPos(titlebarMin);

			//fgDrawList->AddRect(titlebarMin, titlebarMax, ImColor32(255, 0, 0, 255));
			//fgDrawList->AddRect(
			//	ImVec2{ titlebarMax.x / 2 - ImGui::CalcTextSize(title.c_str()).x, titlebarMin.y },
			//	ImVec2{ titlebarMax.x / 2 + ImGui::CalcTextSize(title.c_str()).x, titlebarMax.y },
			//	ImColor32(255, 0, 0, 255)
			//);
			
			bgDrawList->AddRectFilledMultiColor(
				titlebarMin,
				titlebarMax,
				ImColor32(50, 70, 50, 255),
				ImColor32(50, 50, 50, 255),
				ImColor32(50, 50, 50, 255),
				ImColor32(50, 50, 50, 255)
			);
		
			bgDrawList->AddRectFilledMultiColor(
				titlebarMin,
				ImGui::GetCursorScreenPos() + ImGui::GetWindowSize() - ImVec2(ImGui::GetWindowSize().x * 3 / 4, 0),
				ImGui::GetColorU32(ImVec4(0.278431f, 0.701961f, 0.447059f, 1.00f)),
				ImColor32(50, 50, 50, 0),
				ImColor32(50, 50, 50, 0),
				ImGui::GetColorU32(ImVec4(0.278431f, 0.701961f, 0.447059f, 1.00f))
			);


			if (!ImGui::IsAnyItemHovered() && !HE::Application::GetWindow().IsFullScreen() && ImGui::IsWindowHovered())
				HE::Application::GetWindow().SetTitleBarState(true);
			else
				HE::Application::GetWindow().SetTitleBarState(false);

			float ySpace = ImGui::GetContentRegionAvail().y;

			// Text
			{
				ImGui::ScopedFont sf(1, 20);
				ImGui::ScopedStyle fr(ImGuiStyleVar_FramePadding, ImVec2{ 8.0f , 8.0f });

				ImVec2 cursorPos = ImGui::GetCursorPos();

				float size = ImGui::CalcTextSize(title.c_str()).x;
				float avail = ImGui::GetContentRegionAvail().x;

				float off = (avail - size) * 0.5f;
				if (off > 0.0f)
					ImGui::ShiftCursorX(off);

				//ImGui::ShiftCursorY(-2 * scale);
				ImGui::TextUnformatted(title.c_str());
				//ImGui::ShiftCursorY(2 * scale);

				ImGui::SetCursorPos(cursorPos);
			}

			// icon
			{
				ImVec2 cursorPos = ImGui::GetCursorPos();

				ImGui::ScopedColor sc0(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
				ImGui::ScopedColor sc1(ImGuiCol_ButtonActive, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
				ImGui::ScopedColor sc2(ImGuiCol_ButtonHovered, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });

				if (ImGui::ImageButton("icon", icon, { ySpace, ySpace }))
				{
					isIconClicked = true;
				}

				ImGui::SetCursorPos(cursorPos);
				ImGui::SameLine();

			}
		
			// buttons
			{
				ImGui::ScopedStyle fr(ImGuiStyleVar_FrameRounding, 0.0f);
				ImGui::ScopedStyle is(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f));
				ImGui::ScopedStyle iis(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.0f));

				ImVec2 cursorPos = ImGui::GetCursorPos();
				float size = ySpace * 3 + style.FramePadding.x * 6.0f;
				float avail = ImGui::GetContentRegionAvail().x;

				float off = (avail - size);
				if (off > 0.0f)
					ImGui::ShiftCursorX(off);

				{
					ImGui::ScopedColor sc0(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
					if(ImGui::ImageButton("min", min, { ySpace, ySpace })) HE::Application::GetWindow().MinimizeWindow();

					ImGui::SameLine();
					if (ImGui::ImageButton("max_res", isMaximized ? res : max, { ySpace, ySpace })) 
						isMaximized ? HE::Application::GetWindow().RestoreWindow() : HE::Application::GetWindow().MaximizeWindow();
				}

				{
					ImGui::SameLine();
					ImGui::ScopedColor sc0(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
					ImGui::ScopedColor sc1(ImGuiCol_ButtonActive, ImVec4{ 1.0f, 0.3f, 0.2f, 1.0f });
					ImGui::ScopedColor sc2(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.3f, 0.2f, 1.0f });
					if (ImGui::ImageButton("close", close, { ySpace, ySpace })) HE::Application::Shutdown();
				}

				ImGui::SetCursorPos(cursorPos);
			}

			return isIconClicked;
		}
		else
		{
			ImGui::BeginMainMenuBar();
		}

		return false;
	}

	void EndMainMenuBar()
	{
		ImGui::EndMainMenuBar();
	}
}