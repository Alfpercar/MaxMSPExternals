#include "DirectoryIterator.hxx"
#include "concat/Utilities/StringHelpers.hxx"

#include <cassert>

namespace concat
{
	DirectoryIterator::Entry::Entry()
	{
	}

	DirectoryIterator::Entry::Entry(const char *entryName, const char *pathName, bool isDirectory) : entryName_(entryName), pathName_(pathName), isDirectory_(isDirectory)
	{
		fullEntryName_ = terminatePath(pathName) + entryName;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	bool DirectoryIterator::Entry::isDirectory() const
	{
		return isDirectory_;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	const std::string &DirectoryIterator::Entry::getName() const
	{
		return entryName_;
	}

	const char *DirectoryIterator::Entry::getNameAsCString() const
	{
		return getName().c_str();
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	const std::string &DirectoryIterator::Entry::getFullName() const
	{
		return fullEntryName_;
	}

	const char *DirectoryIterator::Entry::getFullNameAsCString() const
	{
		return getFullName().c_str();
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	Path DirectoryIterator::Entry::getAsPath() const
	{
		assert(isDirectory());
		return Path(getFullNameAsCString());
	}

	Filename DirectoryIterator::Entry::getAsFilename() const
	{
		assert(!isDirectory());
		return Filename(getFullNameAsCString());
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	bool DirectoryIterator::Entry::hasExtension(const char *extensionNoPoint) const
	{
		if (isDirectory_)
			return false;
		else
			return ciStrEqual(getAsFilename().getExtensionOnlyAsString(false), extensionNoPoint);
	}

	// -----------------------------------------------------------------------------------

	DirectoryIterator::DirectoryIterator()
	{
		findHandle_ = INVALID_HANDLE_VALUE;
	}

	DirectoryIterator::~DirectoryIterator()
	{
		if (findHandle_ != INVALID_HANDLE_VALUE)
		{
			::FindClose(findHandle_);
		}
	}

	DirectoryIterator::DirectoryIterator(const char *absolutePath, const char *wildcard, bool excludeSystemDirectories)
	{
		findHandle_ = INVALID_HANDLE_VALUE;

		absolutePath_ = absolutePath;
		wildcard_ = wildcard;
		excludeSystemDirectories_ = excludeSystemDirectories;

		if (absolutePath_.size() + wildcard_.size() >= MAX_PATH - 1)
		{
			absolutePath_ = "";
			wildcard_ = "";
		}
	}

	bool DirectoryIterator::isDirectoryEmpty() const
	{
		// Check if DirectoryIterator is properly initialized:
		if (absolutePath_ == "" || wildcard_ == "")
			return true; // failed, is empty

		// See if path exists:
		std::string fullSearchString = terminatePath(absolutePath_) + wildcard_;
		HANDLE findHandle = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATA findData;
		findHandle = ::FindFirstFile(fullSearchString.c_str(), &findData);
		if (findHandle == INVALID_HANDLE_VALUE)
		{
			return true; // failed, is empty
		}

		// If exists, see if not only containing system directories (empty):
		bool isEmpty;
		if (excludeSystemDirectories_)
		{
			isEmpty = true;
			do
			{
				std::string filename(findData.cFileName);
				if (filename != "." && filename != "..")
				{
					isEmpty = false;
					break;
				}
			} while (::FindNextFile(findHandle, &findData) != 0);
		}
		else
		{
			// even if only containing system directories consider non-empty
			isEmpty = false;
		}

		::FindClose(findHandle);
		return isEmpty;
	}

	bool DirectoryIterator::getNextEntry(DirectoryIterator::Entry &entry)
	{
		std::string fullSearchString = terminatePath(absolutePath_) + wildcard_;

		if (findHandle_ == INVALID_HANDLE_VALUE)
		{
			findHandle_ = ::FindFirstFile(fullSearchString.c_str(), &findData_);

			if (findHandle_ == INVALID_HANDLE_VALUE)
			{
				return false;
			}
			else
			{
				if (excludeSystemDirectories_)
				{
					std::string filename(findData_.cFileName);
					if (filename == "." || filename == "..")
					{
						return getNextEntry(entry); // skip (next)
					}
				}

				entry = Entry(findData_.cFileName, absolutePath_.c_str(), (findData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
				return true;
			}
		}
		else
		{
			bool ok = (::FindNextFile(findHandle_, &findData_) != 0);
			if (ok)
			{
				if (excludeSystemDirectories_)
				{
					std::string filename(findData_.cFileName);
					if (filename == "." || filename == "..")
					{
						return getNextEntry(entry); // skip (next)
					}
				}

				entry = Entry(findData_.cFileName, absolutePath_.c_str(), (findData_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
			}
			return ok;
		}
	}
}
