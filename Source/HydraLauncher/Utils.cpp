module;

#include "HydraEngine/Base.h"
#include <windows.h>
#include <stdio.h>

module Utils;
import HE;

namespace Utils {

	Process::~Process()
	{
		if (hProcess) 
		{
			CloseHandle(hProcess);
			CloseHandle(hThread);
		}
	}

	bool Process::Start(const char* command, bool showOutput, const char* workingDir)
	{
		PROCESS_INFORMATION pi;

		STARTUPINFOA si = { sizeof(si) };
		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	
		BOOL result = CreateProcessA(
			NULL, (LPSTR)command,
			NULL, NULL, TRUE,
			showOutput ? NULL : CREATE_NO_WINDOW,
			NULL,
			workingDir,
			&si, &pi
		);

		hProcess = pi.hProcess;
		hThread = pi.hThread;
		dwProcessId = pi.dwProcessId;
		dwThreadId = pi.dwThreadId;
	
		return result;
	}

	void Process::Wait()
	{
		if (hProcess)
		{
			WaitForSingleObject(hProcess, INFINITE);

			CloseHandle(hProcess);
			CloseHandle(hThread);
			hProcess = nullptr;
			hThread = nullptr;
		}
	}
	
	void Process::Kill()
	{
		if (hProcess)
		{
			TerminateProcess(hProcess, 1);
			CloseHandle(hProcess);
			CloseHandle(hThread);
			hProcess = nullptr;
			hThread = nullptr;
		}
	}

	bool ExecCommand(const char* command, std::string* output, const char* workingDir, bool async, bool showOutput, const std::function<void()>& onComplete)
	{
		const char* shellPrefix = command ? "cmd.exe /C " : "";
		constexpr size_t bufferSize = 2048;
		char fullCmdBuffer[bufferSize];
		memset(fullCmdBuffer, 0, sizeof(fullCmdBuffer));
		snprintf(fullCmdBuffer, sizeof(fullCmdBuffer), "%s%s", shellPrefix, command);

		PROCESS_INFORMATION pi = {};
		HANDLE hRead, hWrite;
		SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
	
		if (output)
		{
			if (!CreatePipe(&hRead, &hWrite, &sa, 0))
				return false;

			SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);
		}
	
		STARTUPINFOA si = { sizeof(si) };
		if (output)
		{
			si.cb = sizeof(si);
			si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_NORMAL;//SW_HIDE;
			si.hStdOutput = hWrite;
			si.hStdError = hWrite; // also capture stderr
		}
		else
		{
			ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
		}

		BOOL result = CreateProcessA(
			NULL, (LPSTR)fullCmdBuffer,
			NULL, NULL, TRUE,
			showOutput ? NULL : CREATE_NO_WINDOW,
			NULL,
			workingDir,
			&si, &pi
		);
	
		if (output)
		{
			CloseHandle(hWrite);
		}

		if (async)
		{
			std::thread([result, onComplete, pi, output, hRead]() {
				
				WaitForSingleObject(pi.hProcess, INFINITE);

				if (output)
				{
					if (!result)
					{
						CloseHandle(hRead);
					}

					char buffer[4096];
					DWORD bytesRead;

					while (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
						output->append(buffer, bytesRead);

					CloseHandle(hRead);
				}


				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				if (onComplete)
					onComplete();

			}).detach();
		}
		else
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			
			if (output)
			{
				if (!result)
				{
					CloseHandle(hRead);
				}

				char buffer[4096];
				DWORD bytesRead;

				while (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
					output->append(buffer, bytesRead);

				CloseHandle(hRead);
			}

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			if (onComplete)
				onComplete();
		}

		return result;
	}
}