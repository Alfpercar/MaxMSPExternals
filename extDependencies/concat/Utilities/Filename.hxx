#ifndef INCLUDED_CONCAT_FILENAME_HXX
#define INCLUDED_CONCAT_FILENAME_HXX

#include <cassert>
#include <string>

#define NOGDI
#define NOGDICAPMASKS
#define NOMETAFILE
#define NOMINMAX
#include <windows.h>

#include "StringHelpers.hxx"

namespace concat
{
	class Filename;
	class Path;
	Filename operator+(const Path &lhs, const Filename &rhs);
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	// Platform-specific file system stuff.
	class FileSystem
	{
	public:
		static const char *folderSeparator()
		{
			return "\\";
		}

		static const char *otherFolderSeparator() // also allowed, but not preferred folder separator character
		{
			return "/";
		}

		static const char *filenameExtensionSeparator()
		{
			return ".";
		}

		// note on some file systems there's no local drive separator
		// '/bla' means absolute '/bla'
		// './bla' means relative '/bla'
		static const char *localDriveSeparator()
		{
			return ":\\";
		}

		static const char *networkDriveSeparator()
		{
			return "\\\\";
		}

		// .. invalid characters as single string ..

		// .. max path (name of path? what about filename lenght?) ..

		static const char *getCurrentWorkingPath()
		{
			static char cwd[MAX_PATH];
			::GetCurrentDirectory(MAX_PATH, cwd); // cwd is not terminated with slash

			return cwd;
		}
	};
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	inline bool isPathTerminated(const std::string &path)
	{
		if (path.size() == 0)
			return false;

		std::string::size_type pos = path.rfind('\\');
		if (pos == path.size() - 1)
			return true;
		else
			return false;
	}

	// end path with '\\' (if not already ending in '\\')
	inline std::string terminatePath(const std::string &path)
	{
		std::string result;
		if (!isPathTerminated(path))
			result = path + "\\";
		else
			result = path;

		return result;
	}
}

// ---------------------------------------------------------------------------------------

#include <list>
#include <string>

namespace concat
{
	inline std::string stringWideReplace(std::string str, std::string substr, std::string with)
	{
		std::string::size_type pos;
		while ((pos = str.find(substr)) != std::string::npos)
		{
			str.replace(pos, substr.size(), with);
		}
		return str;
	}

	// searchWithWildcard allows '?' char to represent "any character"
	inline bool stringStartsWith(std::string str, std::string searchWithWildcard)
	{
		if (searchWithWildcard.empty())
			return false;

		std::string::size_type pos = 0;
		while (1)
		{
			pos = searchWithWildcard.find_first_not_of("?", pos);
			if (pos == std::string::npos)
				break;

			if (pos >= str.size())
				return false;

			if (str[pos] == searchWithWildcard[pos])
			{
				pos += 1;
				continue;
			}
			else
			{
				return false;
			}
		}

		return true;
	}
}

namespace concat
{
	// path may begin and/or end in separator, e.g. 'foo', 'foo/', '/foo' or '/foo/'
	// path may be null ('', Path::NONE)
	// path may be relative (not starting with 'letter:' or '//')
	class Path
	{
	public:
		enum NullPath
		{
			NONE
		};

		Path()
		{
			isNullPath_ = true;
		}

		explicit Path(const char *str)
		{
			if (str == '\0')
			{
				isNullPath_ = true;
			}
			else
			{
				isNullPath_ = false;
				fromString(str);
			}
		}

		explicit Path(const std::string &str)
		{
			if (str == "")
			{
				isNullPath_ = true;
			}
			else
			{
				isNullPath_ = false;
				fromString(str.c_str());
			}
		}

		Path &operator=(const NullPath &)
		{
			isNullPath_ = true;
			isRelative_ = false; // should not be used
			isLocal_ = true; // should not be used
			driveName_ = ""; // should not be used
			pathTokens_.clear(); // should not be used

			return *this;
		}

		Path &operator=(const Path &rhs)
		{
			isNullPath_ = rhs.isNullPath_;
			isRelative_ = rhs.isRelative_;
			isLocal_ = rhs.isLocal_;
			driveName_ = rhs.driveName_;
			pathTokens_ = rhs.pathTokens_;
			
			return *this;
		}

		bool operator==(const NullPath &)
		{
			return isNullPath_;
		}

		bool operator!=(const NullPath &other)
		{
			return !(*this == other);
		}

		// .. normal == and != operators ..

		bool isRelative() const
		{
			return isRelative_;
		}

		bool isLocal() const
		{
			return isLocal_;
		}

		bool isNullPath() const
		{
			return isNullPath_;
		}

		// returns the path as a string as it is, relative or absolute
		std::string getAsString(bool terminatePath = true) const
		{
			if (isNullPath_)
			{
				return "";
			}

			std::string result;
			if (!isRelative_)
			{
				if (isLocal_)
					result += driveName_ + ":\\";
				else
					result += "\\\\";
			}

			std::list<std::string>::const_iterator iter;
			for (iter = pathTokens_.begin(); iter != pathTokens_.end(); ++iter)
			{
				if (iter != pathTokens_.begin())
					result += "\\";
				result += (*iter);
			}

			if (terminatePath)
				result += "\\";

			return result;
		}

		std::string getDriveName() const
		{
			return driveName_;
		}

//		std::list<std::string>::const_iterator beginTokens() const;
//		std::list<std::string>::const_iterator endTokens() const;
//		non-const too
//		erase(), insert()

		Path &toAbsolute()
		{
			if (isNullPath_)
				return *this;

			if (!isRelative_)
				return *this;

			Path cwd(FileSystem::getCurrentWorkingPath());

			Path result = cwd + *this;

			*this = result;

			isRelative_ = false;
			isLocal_ = true;

			return *this;
		}

		// toRelative(Path base) ?

		inline Path &operator+(const Path &rhs)
		{
			if (isNullPath_)
			{
				*this = rhs;
			}
			else if (rhs.isNullPath_)
			{
				// do nothing
			}
			else
			{
				for (std::list<std::string>::const_iterator i = rhs.pathTokens_.begin(); i != rhs.pathTokens_.end(); ++i)
				{
					pathTokens_.push_back((*i));
				}

				resolveRelativePath();
			}

			// XXX: check if lhs/rhs are relative, local, remote, absolute, etc. (some combinations 
			// are invalid)

			return *this;
		}

		void makeExistent()
		{
			if (isNullPath_)
				return;

			Path tmp = *this;
			tmp.toAbsolute();

			std::string curPath;
			if (isLocal_)
				curPath = tmp.driveName_ + ":\\";
			else
				curPath = "\\\\";

			for (std::list<std::string>::iterator i = tmp.pathTokens_.begin(); i != tmp.pathTokens_.end(); ++i)
			{
				curPath += (*i) + "\\";
				::CreateDirectory(curPath.c_str(), NULL);
			}
		}

		bool getAsRelative(Path basePath, Path &target) const // returns false if not possible
		{
			if (isRelative() || basePath.isRelative())
				return false; // paths must both be absolute

			if (basePath.getDriveName() != getDriveName())
				return false; // paths must be on same drive, else there is no relative path between them

			std::string relPathStr = "";
			std::list<std::string>::const_iterator pathTokensIter = pathTokens_.begin();
			std::list<std::string>::const_iterator basePathTokensIter = basePath.pathTokens_.begin();

			while (1)
			{
				// No remaining path tokens, 
				// path is same as base path:
				if (pathTokensIter == pathTokens_.end())
				{
					relPathStr = ".\\";
					break; // end
				}

				// No remaining base path tokens, 
				// path is sub-path of base path:
				if (basePathTokensIter == basePath.pathTokens_.end())
				{
					relPathStr = ".\\";
					for (pathTokensIter = pathTokensIter; pathTokensIter != pathTokens_.end(); ++pathTokensIter)
					{
						relPathStr += (*pathTokensIter);
						relPathStr += "\\";
					}
					break; // end
				}

				std::string pathToken = (*pathTokensIter);
				std::string basePathToken = (*basePathTokensIter);

				// Find first token (from beginning) that's different:
				if (pathToken != basePathToken)
				{
					const int numRemainingBaseTokens = (int)std::distance<std::list<std::string>::const_iterator>(basePathTokensIter, basePath.pathTokens_.end());

					// No remaining base path tokens, 
					// path is a sub-path of base path:
					// XXX: already handled above?
					if (numRemainingBaseTokens == 0)
					{
						relPathStr = ".\\";
					}
					// Remaining base path tokens, path is 
					// in a 'higher' path than base path, 
					// go back:
					else
					{
						for (int i = 0; i < numRemainingBaseTokens; ++i)
						{
							relPathStr += "..\\";
						}
					}

					// Add remaining path tokens to relative path in order reach path:
					for (std::list<std::string>::const_iterator i = pathTokensIter; i != pathTokens_.end(); ++i)
					{
						relPathStr += (*i) + "\\";
					}

					break; // end
				}

				// Next tokens:
				++pathTokensIter;
				++basePathTokensIter;
			}

			target = Path(relPathStr.c_str());

			return true;
		}

		typedef std::list<std::string>::const_iterator ConstPathTokenIterator;

		ConstPathTokenIterator beginPathTokens() const { return pathTokens_.begin(); }
		ConstPathTokenIterator endPathTokens() const { return pathTokens_.end(); }
		int getNumPathTokens() const { return (int)pathTokens_.size(); }

		std::string getLastPathTokenAsString(bool terminatePath = true) const
		{
			if (getNumPathTokens() == 0)
				return "";

			ConstPathTokenIterator i = endPathTokens();
			--i;

			if (terminatePath)
				return (*i) + FileSystem::folderSeparator();
			else
				return (*i);
		}

		Path getParentPath()
		{
			Path parent = *this; // make copy
			if (!parent.pathTokens_.empty())
				parent.pathTokens_.pop_back();
			return parent;
		}

	private:
		bool isNullPath_;
		bool isRelative_; // because on unix systems can't just see if driveName_ is empty
		bool isLocal_;
		std::string driveName_; // unterminated
		std::list<std::string> pathTokens_; // each token is unterminated

		void fromString(const char *str)
		{
			std::string s(str);

			// replace all occurrences of "/" with "\\":
			s = stringWideReplace(s, "/", "\\"); // XXX: use FileSystem::otherPathSep() and FileSystem::pathSep()

			// string starts with:
			// "." or ".." -> relative (local)
			// "?:" -> local absolute (? is drive letter, windows)
			// "\\\\" -> remote absolute
			// "\\" -> relative (windows), absolute (unix)
			// else -> relative (local)
			std::string::size_type pos;
			if (stringStartsWith(s, ".") || stringStartsWith(s, ".."))
			{
				isRelative_ = true;
				isLocal_ = true;
				pos = 0;
			}
			else if (stringStartsWith(s, "?:"))
			{
				driveName_ = s.substr(0, 1);
				isRelative_ = false;
				isLocal_ = true;
				pos = 2;
				if (pos < s.size()) // for e.g. s = "C:" use 2, if longer use 3
					pos += 1;
			}
			else if (stringStartsWith(s, "\\\\"))
			{
				isRelative_ = false;
				isLocal_ = false;
				pos = 2;
			}
			else if (stringStartsWith(s, "\\"))
			{
				// windows:
				isRelative_ = true;
				isLocal_ = true;
				pos = 1;

				// unix:
//				isRelative_ = false;
//				isLocal_ = true; // ?
//				pos = 1;
			}
			else
			{
				isRelative_ = true;
				isLocal_ = true;
				pos = 0;
			}

			// tokenize using "\\" (or end) delimiter:
			while (1)
			{
				std::string::size_type pos2 = s.find("\\", pos);
				if (pos2 == std::string::npos)
				{
					if (pos != s.size())
					{
						std::string token = s.substr(pos, s.size() - pos);
						if (token != "")
							pathTokens_.push_back(token); // might happen if double slashes are used instead of single slashes (user error)
					}
					break;
				}
				else
				{
					std::string token = s.substr(pos, pos2 - pos);
					if (token != "") // might happen if double slashes are used instead of single slashes (user error)
						pathTokens_.push_back(token);
				}

				pos = pos2 + 1;
			}

			// resolve relative path (as much as is possible):
			resolveRelativePath();

			// check for invalid characters in path tokens:
			// ...
		}

		void resolveRelativePath()
		{
			// remove "." not at start:
			for (std::list<std::string>::iterator i = pathTokens_.begin(); i != pathTokens_.end(); ++i)
			{
				if (i == pathTokens_.begin())
					continue; // keep "." at start

				if ((*i) == ".")
				{
					std::list<std::string>::iterator toBeRemoved = i;
					--i; // set to previous, will be incremented in the for-loop (always valid as first is skipped)

					pathTokens_.erase(toBeRemoved);
				}
			}

			// go back one 'path token' (if possible) for each ".." encountered which isn't at start:
			for (std::list<std::string>::iterator i = pathTokens_.begin(); i != pathTokens_.end(); )
			{
				if (i == pathTokens_.begin())
				{
					++i;
					continue; // keep ".." at start
				}

				if ((*i) == "..")
				{
					std::list<std::string>::iterator toBeRemoved1 = i;
					--i; // set to previous, will be incremented in the for-loop (always valid as first is skipped)
					std::list<std::string>::iterator toBeRemoved2 = i;

					if ((*toBeRemoved2) == "." || (*toBeRemoved2) == "..") // can not go up one folder, keep as-is
					{
						++i;
						++i;
						continue;
					}

					pathTokens_.erase(toBeRemoved1);

					if ((*toBeRemoved2) != ".." && (*toBeRemoved2) != ".")
					{
						++i;
						pathTokens_.erase(toBeRemoved2);
						continue; // do not increment i (already incremented above, before erase)
					}
				}

				++i; // next element
			}
		}

	};
}

// ---------------------------------------------------------------------------------------

namespace concat
{
	class Filename
	{
	public:
		Filename()
		{
			filename_ = "";
			path_ = Path::NONE;
		}

		explicit Filename(const char *filename)
		{
			fromString(filename);
		}

		explicit Filename(const std::string &filename)
		{
			fromString(filename.c_str());
		}

		bool hasPath() const
		{
			return path_.isNullPath();
		}

		Path &getPath()
		{
			return path_;
		}
		
		const Path &getPath() const
		{
			return path_;
		}

		void setPath(const Path &path)
		{
			path_ = path;
		}

		std::string getFullFilenameAsString() const
		{
			return path_.getAsString(true) + filename_;
		}

		const std::string &getFilenameOnlyAsString() const
		{
			return filename_;
		}

		std::string getFullFilenameNoExtensionAsString() const
		{
			return path_.getAsString(true) + getFilenameOnlyNoExtensionAsString();
		}

		std::string getFilenameOnlyNoExtensionAsString() const
		{
			std::string result;
			std::string::size_type extPos = filename_.rfind(".");
			result = filename_.substr(0, extPos);
			return result;
		}

		std::string getExtensionOnlyAsString(bool includeDot = false) const
		{
			std::string result;
			std::string::size_type extPos = filename_.rfind(".");
			std::string::size_type pos = extPos;
			if (!includeDot)
				++pos;
			result = filename_.substr(pos, filename_.size() - pos);
			return result;
		}

		// .. get/set full/filename/extension/path/filenameNoExt as string, set functions return new Filename object ..

		bool getAsRelative(Path basePath, Filename &target) const // returns false if not possible
		{
			Path pathRelative;
			bool result = path_.getAsRelative(basePath, pathRelative);
			
			if (result == false)
				return false;

			target = pathRelative + Filename(filename_.c_str());

			return true;
		}

		Filename &toAbsolute()
		{
			path_.toAbsolute();
			return *this;
		}

		bool filenameNoExtensionsEndsWith(const char *str) const
		{
			std::string s(str);
			std::string filename = getFullFilenameNoExtensionAsString();
			std::string::size_type pos = filename.rfind(s);
			if (pos == filename.size() - s.size())
				return true;
			else
				return false;
		}

		Filename &replaceExtension(const char *newExtensionWithDot)
		{
			std::string::size_type extPos = filename_.rfind(".");
			if (extPos == std::string::npos)
			{
				filename_ += newExtensionWithDot;
				return *this;
			}
			else
			{
				filename_.replace(extPos, filename_.size() - extPos, newExtensionWithDot);
			}

			return *this;
		}

		bool exists() const
		{
			WIN32_FIND_DATA data;
			HANDLE h = ::FindFirstFile(getFullFilenameAsString().c_str(), &data);
			if (h != INVALID_HANDLE_VALUE)
			{
				::FindClose(h);
				return true;
			}
			else
			{
				return false;
			}
		}

		bool copyTo(const Filename &destinationFilename, bool failIfExists)
		{
			assert(exists());
			return (::CopyFile(getFullFilenameAsString().c_str(), destinationFilename.getFullFilenameAsString().c_str(), failIfExists) != 0);
		}

		// moveTo() can also be used to rename files if moved to the same path
		bool moveTo(const Filename &destinationFilename)
		{
			assert(exists());
			return (::MoveFile(getFullFilenameAsString().c_str(), destinationFilename.getFullFilenameAsString().c_str()) != 0);
//			bool succeeded = copyTo(destinationFilename, failIfExists);
//			if (succeeded)
//				succeeded = (::DeleteFile(getFullFilenameAsString().c_str()) != 0);
//
//			return succeeded;
		}

		bool deleteFile()
		{
			assert(exists());
			return (::DeleteFile(getFullFilenameAsString().c_str()) != 0);
		}

		// moves existing files out of the way, making backups
		// if file corresponding to filename already exists, 
		// backs up the existing file as filename.000 (moving existing 
		// filename.000 to filename.001, and so on), this is done by means of 
		// renaming files because source and destination will be on same drive (thus fast)
		//
		// maxNumBackups = 0  : no backups
		// maxNumBackups = -1 : 999 backups
		void makeBackup(int maxNumBackups = -1)
		{
			if (maxNumBackups > 999 || maxNumBackups < 0)
				maxNumBackups = 999;

			if (maxNumBackups > 0 && exists())
			{
				// copy all files except last one:
				for (int backupIdx = 999; backupIdx >= 0; --backupIdx)
				{
					Filename curFilename = getPath() + Filename(formatStr("%s.%03d", getFilenameOnlyAsString().c_str(), backupIdx).c_str());

					if (!curFilename.exists())
					{
						continue;
					}
					else
					{
						if (backupIdx >= maxNumBackups-1)
						{
							// delete .999 file
							curFilename.deleteFile();
						}
						else
						{
							// rename .001 to .002, etc.
							Filename nextFilename = getPath() + Filename(formatStr("%s.%03d", getFilenameOnlyAsString().c_str(), backupIdx+1).c_str());
							nextFilename.toAbsolute();
							curFilename.moveTo(nextFilename);
						}
					}
				}

				// copy last file to .000
				Filename lastFilename = getPath() + Filename(std::string(getFilenameOnlyAsString() + ".000").c_str());
				lastFilename.toAbsolute();
				moveTo(lastFilename);
			}
		}

	private:
		Path path_;
		std::string filename_;

	private:
		void fromString(const char *str)
		{
			// XXX:
			// "./foo/bar/test.txt" <- path + file
			// "foo" <- path only , not allowed because no way to tell if string is path or filename
			// "foo." <- file
			// XXX: dots allowed in paths, but code below doesn't work with them!

			std::string s(str);

			// replace all occurrences of "/" with "\\":
			s = stringWideReplace(s, "/", "\\"); // XXX: use FileSystem::otherPathSep() and FileSystem::pathSep()

			// find last \\:
			std::string::size_type pos = s.rfind("\\");
			if (pos == std::string::npos)
			{
				path_ = Path::NONE;
				filename_ = s;
			}
			else
			{
				path_ = Path(s.substr(0, pos).c_str());
				filename_ = s.substr(pos + 1, s.size() - (pos + 1));
			}
		}
	};

}

// ---------------------------------------------------------------------------------------

namespace concat
{
	inline Filename operator+(const Path &lhs, const Filename &rhs)
	{
		std::string tmp = lhs.getAsString(true) + rhs.getFullFilenameAsString();
		return Filename(tmp); // reparse

		//Filename result = rhs;
		//result.setPath(lhs);

		//return result;
	}
}

#endif

