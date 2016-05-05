#ifndef INCLUDED_CONCAT_DIRECTORYITERATOR_HXX
#define INCLUDED_CONCAT_DIRECTORYITERATOR_HXX

#include <string>
#include <windows.h>
#include "concat/Utilities/Filename.hxx"

namespace concat
{
	class DirectoryIterator
	{
	public:
		class Entry
		{
		public:
			Entry();
			Entry(const char *entryName, const char *pathName, bool isDirectory);

			bool isDirectory() const;

			const std::string &getName() const;
			const char *getNameAsCString() const;

			const std::string &getFullName() const;
			const char *getFullNameAsCString() const;

			Path getAsPath() const;
			Filename getAsFilename() const;

			bool hasExtension(const char *extensionNoPoint) const;

		private:
			bool isDirectory_;
			std::string entryName_; // name of entry (file or directory) only
			std::string pathName_; // name of path to entry
			std::string fullEntryName_; // path + entry
		};

	public:
		DirectoryIterator();
		DirectoryIterator(const char *absolutePath, const char *wildcard = "*", bool excludeSystemDirectories = true);
		~DirectoryIterator();

		bool isDirectoryEmpty() const;

		bool getNextEntry(Entry &entry);

	private:
		std::string absolutePath_;
		std::string wildcard_;
		bool excludeSystemDirectories_;
		HANDLE findHandle_;
		WIN32_FIND_DATA findData_;
	};
}

#endif