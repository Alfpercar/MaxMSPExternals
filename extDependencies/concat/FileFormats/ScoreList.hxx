#ifndef INCLUDED_CONCAT_SCORELIST_HXX
#define INCLUDED_CONCAT_SCORELIST_HXX

#include <string>
#include <vector>

namespace concat
{
	class ScoreList
	{
	public:
		static int getMaxNumTakes();

		int getNumItems() const;

		std::string getItemAsName(int index) const;
		std::string getItemAsFilenameNoExt(int index) const;

		bool setItem(int index, const char *name, const char *outputPath);
		// output path to check if resulting filename is valid or not

		bool loadFromFile(const char *filename);
		bool saveToFile(const char *filename, bool failIfExists = true);

		void clearItems();

		int getTakeIndex(int itemIndex) const;

		void updateAllTakeIndexesFromFiles(const char *outputPath);
		bool increaseTakeIndex(int itemIndex); // when done recording, increase take index

		// postfix without dash (-)
		// extension with dot (.); note: with so "" can be used for base filenames
		// outputPath with or without terminating (\\)
		std::string getFilename(int itemIdx, int takeIdx, const char *outputPath, const char *postfix, const char *extension);
		std::string getFilenameNoViolinIdx(int itemIdx, int takeIdx, const char *outputPath, const char *postfix, const char *extension);

		int findIdxFromName(const char *name)
		{
			for (int i = 0; i < getNumItems(); ++i)
			{
				if (getItemAsName(i) == std::string(name))
					return i;
			}

			return -1; // not found
		}

	private:
		std::vector<std::string> items_;
		std::vector<int> takeIndexes_; // used as a kind of cache to speed up finding indexes

		bool isItemDuplicate(std::string name);

		void updateTakeIndexFromFiles(int itemIndex, const char *outputPath);
	};
}

#endif