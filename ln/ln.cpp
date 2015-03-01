// ln.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

enum FileStateExpectation
{
	AllowExists = 1,
	RequireExists = 2,
	RequireDoesNotExist = 4
};

struct MakeLinkArgs
{
	bool Overwrite = false;
	bool SymLink = false;
	LPCWCHAR LinkName;
	LPCWCHAR LinkTarget;
};

enum LnFileType
{
	None,
	File,
	Directory
};

struct FileInfo
{
	const LPCWCHAR Path;
	const bool Valid;
	const bool Exists;
	const bool IsSymLink;
	const LnFileType Type;
	const DWORD ErrorCode;

	FileInfo(LPCWCHAR path, DWORD errorCode) : FileInfo(path, errorCode, false, false, false, LnFileType::None) {}

	FileInfo(LPCWCHAR path, bool exists, bool isSymLink, LnFileType type) : FileInfo(path, 0, true, exists, isSymLink, type) {}

	FileInfo(LPCWCHAR path, DWORD errorCode, bool valid, bool exists, bool isSymLink, LnFileType type) : Path(path), ErrorCode(errorCode), Valid(valid), Exists(exists), IsSymLink(isSymLink), Type(type) {}

};

DWORD PrintErrorMessage(LPWSTR prefixMessage, DWORD errorCode)
{
	LPWSTR errorMessage = NULL;
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, errorCode, LANG_NEUTRAL, (LPWSTR)&errorMessage, 265, NULL);
	wprintf(L"%s%s", prefixMessage, errorMessage);
	LocalFree(errorMessage);
	return errorCode;
}

bool ValidateFileExpectations(FileInfo info, FileStateExpectation expectation)
{
	if (!info.Valid)
	{
		wprintf(L"The path '%s' is invalid\n", info.Path);
		PrintErrorMessage(L"  Underlying Error: ", info.ErrorCode);
		return false;
	}
	if (FileStateExpectation::RequireDoesNotExist == expectation && info.Exists)
	{
		wprintf(L"The file '%s' already exists\n", info.Path);
		return false;
	}
	if (FileStateExpectation::RequireExists == expectation && !info.Exists)
	{
		wprintf(L"The file '%s' does not exist\n", info.Path);
		return false;
	}
	return true;
}

DWORD PrintResultOfDeleteAttempt(bool result)
{
	Sleep(10);
	if (!result)
	{
		return PrintErrorMessage(L"Could not remove old link ", GetLastError());
	}
	return 0;
}

bool CheckPathValidity(LPCWCHAR path)
{
	HANDLE fileHandle = CreateFile(path,    // name of the file
		GENERIC_WRITE, // open for writing
		0,             // sharing mode, none in this case
		NULL,             // use default security descriptor
		CREATE_ALWAYS, // overwrite if exists
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (INVALID_HANDLE_VALUE != fileHandle)
	{
		CloseHandle(fileHandle);
		DeleteFile(path);
		return true;
	}
	return false;
}

FileInfo GetFileState(LPCWCHAR path)
{
	DWORD fileAttributes = GetFileAttributes(path);
	if (INVALID_FILE_ATTRIBUTES == fileAttributes)
	{
		if (!CheckPathValidity(path))
		{
			return FileInfo(path, GetLastError());
		}
		return FileInfo(path, false, false, LnFileType::File);
	}
	
	if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		return FileInfo(path, true, fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT, LnFileType::Directory);
	}
	return FileInfo(path, true, fileAttributes & FILE_ATTRIBUTE_REPARSE_POINT, LnFileType::File);
}

DWORD HandleLinkExistance(FileInfo info, MakeLinkArgs mklinkArgs)
{
	if (info.Exists && mklinkArgs.Overwrite)
	{
		wprintf(L"Link already exists but overwrite set ");
		if (!info.IsSymLink && mklinkArgs.SymLink)
		{
			wprintf(L" but the existing link was not a real file.\n   please remove it and run the command again\n");
			return -3;
		}
		if (!mklinkArgs.SymLink)
		{
			return PrintResultOfDeleteAttempt(DeleteFile(info.Path));
		}
		switch (info.Type)
		{
		case LnFileType::File:
			wprintf(L"attempting to remove directory\n");
			return PrintResultOfDeleteAttempt(DeleteFile(info.Path));
			break;
		case LnFileType::Directory:
			wprintf(L"attempting to remove directory\n");
			return PrintResultOfDeleteAttempt(RemoveDirectory(info.Path));
			break;
		default:
			wprintf(L" unknown file type to remove: aborting\n");
			return -4;
		}

	}
	return 0;
}

void PrintHelp() {
	wprintf(L"Help for ln:\n\n");
	wprintf(L"Syntax\n");
	wprintf(L"  ln [OPTION]... OriginalSourceFile NewLinkFile\n");
	wprintf(L"  ln [OPTION]... OriginalSourceFile... DIRECTORY\n\n");
	wprintf(L"Options\n");
	wprintf(L"   -f, --force\n       Remove existing destination files\n");
	wprintf(L"   -s, --symbolic\n       Make symbolic links instead of hard links\n");
	wprintf(L"   --help\n       Display this help and exit\n");
}

MakeLinkArgs ParseArgs(int argCount, LPCWCHAR arguments[])
{
	int argPosition;
	MakeLinkArgs mklinkArgs;
	for (argPosition = 1; argPosition < argCount; argPosition++)
	{
		LPCWCHAR currentArg = arguments[argPosition];
		int currentArgLength = lstrlen(currentArg);
		if (currentArgLength > 0 && '-' == currentArg[0])
		{
			if (0 == lstrcmp(currentArg, L"--help"))
			{
				PrintHelp();
				exit(0);
			}
			if (0 == lstrcmp(currentArg, L"--force"))
			{
				mklinkArgs.Overwrite = true;
				continue;
			}
			if (0 == lstrcmp(currentArg, L"--symbolic"))
			{
				mklinkArgs.SymLink = true;
				continue;
			}
			for (int i = 0; i < currentArgLength; i++)
			{
				switch (currentArg[i])
				{
				case 'f':
					mklinkArgs.Overwrite = true;
					break;
				case 's':
					mklinkArgs.SymLink = true;
					break;
				}
			}
		}
		else
		{
			break;
		}
	}

	if (2 == argCount - argPosition)
	{
		mklinkArgs.LinkTarget = arguments[argPosition++];
		mklinkArgs.LinkName = arguments[argPosition++];
	}
	else
	{
		wprintf(L"Invalid parameters spcified\n\n");
		PrintHelp();
		exit(1);
	}
	return mklinkArgs;
}

DWORD GetLinkPath(MakeLinkArgs mklinkArgs, LPTSTR linkPath, DWORD maxPathLength)
{
	return GetFullPathName(
		mklinkArgs.LinkTarget,
		maxPathLength,
		linkPath,
		NULL);
}

DWORD CreateSymLink(MakeLinkArgs mklinkArgs, LPCWSTR linkPath, LnFileType targetType)
{
	if (!CreateSymbolicLink(mklinkArgs.LinkName, linkPath, (targetType == LnFileType::Directory) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0))
	{
		return PrintErrorMessage(L"Failed to create link: ", GetLastError());
	}
	return 0;
}

DWORD CreateHardlink(MakeLinkArgs mklinkArgs, LPCWSTR linkPath)
{
	if (!CreateHardLink(mklinkArgs.LinkName, linkPath, NULL))
	{
		return PrintErrorMessage(L"Failed to create link: ", GetLastError());
	}
	return 0;
}

DWORD CreateLink(MakeLinkArgs mklinkArgs, FileInfo targetState)
{
	WCHAR linkPath[MAX_PATH] = L"\0";
	DWORD result = GetLinkPath(mklinkArgs, linkPath, MAX_PATH);
	if (FAILED(result))
	{
		return PrintErrorMessage(L"Could not resolve the path between the link and target", GetLastError());
	}
	if (mklinkArgs.SymLink)
	{
		return CreateSymLink(mklinkArgs, linkPath, targetState.Type);
	}
	else
	{
		return CreateHardlink(mklinkArgs, linkPath);
	}
}


int _tmain(int argc, LPCWCHAR argv[])
{
	DWORD result;
	MakeLinkArgs mklinkArgs = ParseArgs(argc, argv);

	FileInfo linkState = GetFileState(mklinkArgs.LinkName);
	if (!ValidateFileExpectations(linkState, mklinkArgs.Overwrite ? FileStateExpectation::AllowExists : FileStateExpectation::RequireDoesNotExist))
	{
		return linkState.ErrorCode ? linkState.ErrorCode : -1;
	}

	FileInfo targetState = GetFileState(mklinkArgs.LinkTarget);
	if (!ValidateFileExpectations(targetState, FileStateExpectation::RequireExists))
	{
		return targetState.ErrorCode ? targetState.ErrorCode : -2;
	}

	result = HandleLinkExistance(linkState, mklinkArgs);
	if (0 != result)
	{
		return result;
	}

	// Delete simply marks the file for deletion so there may have some delay before the file is actually removed.
	// This can lead to the newly created symlink being removed. :(
	Sleep(100);


	result = CreateLink(mklinkArgs, targetState);
	return result;
}