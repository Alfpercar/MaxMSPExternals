#include "ScoreList.hxx"

#include <ostream>
#include <sstream>
#include <fstream>

//#include "juce.h" // disable if not using Juce

#include "concat/Utilities/StringHelpers.hxx"
#include "concat/Utilities/Filename.hxx"

#undef min
#undef max
#include <algorithm>

namespace concat
{
	int ScoreList::getMaxNumTakes()
	{
		// limit max number of takes so that big score lists load quicker
		// (checking which is the last take by seeing if files exist takes time)
		// filenames will be *-001.* .. *-030.*
		return 30;
	}

	// ---------------------------------------------------------------------------------------

	int ScoreList::getNumItems() const
	{
		return (int)items_.size();
	}

	// ---------------------------------------------------------------------------------------

	std::string ScoreList::getItemAsName(int index) const
	{
		if (index < 0 || index >= (int)items_.size())
			return "";

		return items_[index];
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	std::string ScoreList::getItemAsFilenameNoExt(int index) const
	{
		std::string result = getItemAsName(index);
		strReplaceAll(result, " ", "_");
		strToLower(result);
		return result;
	}

	// ---------------------------------------------------------------------------------------

	bool ScoreList::setItem(int index, const char *name, const char *outputPath)
	{
		if (index < 0 || index >= (int)items_.size())
			return false;

		if (name == "")
			return false;

		std::string validName;
	#ifdef __JUCE_JUCEHEADER__
		validName = (const char *)File::createLegalFileName(name);
	#else
		validName = name;
	#endif

		if (validName == "")
			return false;

		if (isItemDuplicate(validName))
			return false;

		items_[index] = validName;
		updateTakeIndexFromFiles(index, outputPath);

		return true;
	}

	// ---------------------------------------------------------------------------------------

	bool ScoreList::loadFromFile(const char *filename)
	{
		// note: doesn't clear items in case of failure

		std::fstream file;
		file.open(filename, std::ifstream::in);

		if (!file)
			return false;

		clearItems();

		int index = 0;

		const int maxNumItems = 512;	// just to avoid accidentally loading very big text 
										// files as score list and then checking if all 
										// takes for those lines exist
		while (1)
		{
			std::string line;
			if (std::getline(file, line).fail())
				break;

			if (index >= maxNumItems)
				break;

			if (line.empty())
				continue;

			std::string validName;
	#ifdef __JUCE_JUCEHEADER__
			validName = std::string((const char *)File::createLegalFileName(line.c_str()));
	#else
			validName = line;
	#endif

			if (validName == "")
				continue;

			if (!isItemDuplicate(validName))
			{
				items_.push_back(validName);
				takeIndexes_.push_back(0); // default value, need to call updateAllTakeIndexesFromFiles() after load!
				++index;
			}
		}

		file.close();

		return true;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	bool ScoreList::saveToFile(const char *filename, bool failIfExists)
	{
		Filename f(filename);
		if (failIfExists && f.exists())
			return false;

		std::fstream file;
		file.open(filename, std::ifstream::out);

		if (!file)
			return false;

		for (int i = 0; i < getNumItems(); ++i)
		{
			file << getItemAsName(i) << std::endl;
		}

		file.close();

		return true;
	}

	// ---------------------------------------------------------------------------------------

	void ScoreList::clearItems()
	{
		items_.clear();
		takeIndexes_.clear();
	}

	// ---------------------------------------------------------------------------------------

	bool ScoreList::isItemDuplicate(std::string name)
	{
		for (int i = 0; i < (int)items_.size(); ++i)
		{
			if (ciStrEqual(items_[i], name))
			{
				return true;
			}
		}

		return false;
	}

	// ---------------------------------------------------------------------------------------

	void ScoreList::updateAllTakeIndexesFromFiles(const char *outputPath)
	{
		for (int i = 0; i < (int)items_.size(); ++i)
		{
			updateTakeIndexFromFiles(i, outputPath);
		}
	}

	void ScoreList::updateTakeIndexFromFiles(int itemIndex, const char *outputPath)
	{
		if (itemIndex < 0 || itemIndex >= (int)items_.size())
			return;

		int nextTakeIndex = 0; // default (used in case of failure)

		// Try takes 29 down to 0 (filenames *-030.* to *-001.*), first take index for which any 
		// of the file exist is considered the last recorded take index, next take index will be 
		// last + 1.
		// This method can be a bit slow for big score lists, but allows lower take indexes 
		// (bad takes) to be deleted while keeping only higher take indexes (good takes) and 
		// having all the tools still work properly.
		for (int i = getMaxNumTakes()-1; i >= 0; --i)
		{
			// See if any of the files produced when recording exist (we don't want to overwrite 
			// any data never):
			// (it may happen that one of the streams doesn't start writing due to a negative rewind, but the 
			// other streams do; in this case we don't want to overwrite existing files)
			std::string filename1 = getFilename(itemIndex, i, outputPath, "ch1", ".wav");
			//std::string filename2 = getFilename(itemIndex, i, outputPath, "ch2", ".wav");
			std::string filename3 = getFilenameNoViolinIdx(itemIndex, i, outputPath, "header", ".dat");
			std::string filename4 = getFilenameNoViolinIdx(itemIndex, i, outputPath, "tracker", ".dat");
			//std::string filename5 = getFilename(itemIndex, i, outputPath, "arduino", ".dat");
			std::string filename6 = getFilenameNoViolinIdx(itemIndex, i, outputPath, "metronome", ".txt");
			// Note: Take index (i) 0 corresponds to filename *-001*.

			Filename f1(filename1);
			//Filename f2(filename2);
			Filename f3(filename3);
			Filename f4(filename4);
			//Filename f5(filename5);
			Filename f6(filename6);
			if (f1.exists() || 
				//f2.exists() || 
				f3.exists() || 
				f4.exists() || 
				//f5.exists() || 
				f6.exists())
			{
				nextTakeIndex = std::min(i+1, getMaxNumTakes()-1);
				// (if there are more than 30 takes, it will overwrite the *-030.* files)
				break;
			}
		}

		takeIndexes_[itemIndex] = nextTakeIndex;
	}

	bool ScoreList::increaseTakeIndex(int itemIndex)
	{
		if (itemIndex < 0 || itemIndex >= (int)items_.size())
			return false;

		if (takeIndexes_[itemIndex] < getMaxNumTakes()-1)
		{
			takeIndexes_[itemIndex] = takeIndexes_[itemIndex] + 1;
			return true;
		}
		else
		{
			// takeIndexes_[itemIndex] keeps at 29
			return false;
		}
	}

	int ScoreList::getTakeIndex(int itemIndex) const
	{
		return takeIndexes_[itemIndex];
	}

	std::string ScoreList::getFilename(int itemIdx, int takeIdx, const char *outputPath, const char *postfix, const char *extension)
	{
		std::string filename = formatStr("%s\\%s-%03d-%s%s",
									Path(outputPath).getAsString(false).c_str(),
									getItemAsFilenameNoExt(itemIdx).c_str(),
									takeIdx + 1,
									postfix,
									extension);
		return filename;
	}

	std::string ScoreList::getFilenameNoViolinIdx(int itemIdx, int takeIdx, const char *outputPath, const char *postfix, const char *extension)
	{
		int pos = getItemAsFilenameNoExt(itemIdx).find_last_of("_v");    // position of "_v" in str
		std::string nameNoViolinIdx = getItemAsFilenameNoExt(itemIdx).substr(0,pos-1);   
		std::string filename = formatStr("%s\\%s-%03d-%s%s",
			Path(outputPath).getAsString(false).c_str(),
			nameNoViolinIdx.c_str(),
			takeIdx + 1,
			postfix,
			extension);
		return filename;
	}
}






