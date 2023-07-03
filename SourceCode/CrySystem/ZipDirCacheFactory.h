//////////////////////////////////////////////////////////////////////////
// This is the class that can read the directory from Zip file,
// and store it into the directory cache
#ifndef _ZIP_DIR_READER_HDR_
#define _ZIP_DIR_READER_HDR_

namespace ZipDir
{

class CacheRW;
TYPEDEF_AUTOPTR(CacheRW);
typedef CacheRW_AutoPtr CacheRWPtr;


// an instance of this class is temporarily created on stack to initialize the CZipFile instance
class CacheFactory
{
public:
	enum 
	{
		// open RW cache in read-only mode
		FLAGS_READ_ONLY = 1,
		// do not compact RW-cached zip upon destruction
		FLAGS_DONT_COMPACT = 1 << 1,
		// if this is set, then the zip paths won't be memorized in the cache objects
		FLAGS_DONT_MEMORIZE_ZIP_PATH = 1 << 2,
		// if this is set, the archive will be created anew (the existing file will be overwritten)
		FLAGS_CREATE_NEW = 1 << 3
	};

	// initializes the internal structures
	// nFlags can have FLAGS_READ_ONLY flag, in this case the object will be opened only for reading
	CacheFactory (CMTSafeHeap* pHeap, InitMethodEnum nInitMethod, unsigned nFlags = 0);
	~CacheFactory();

	// the new function creates a new cache
	CachePtr New(const char* szFileName);// throw (ErrorEnum);

	CacheRWPtr NewRW(const char* szFileName);

protected:
	// reads the zip file into the file entry tree.
	void Read (CacheRW& rwCache);

	// creates from the m_f file
	// reserves the given number of bytes for future expansion of the object
	// upon return, pReserve contains the actual number of bytes that were allocated (more might have been allocated)
	CachePtr MakeCache (const char* szFile);

	// this sets the window size of the blocks of data read from the end of the file to find the Central Directory Record
	// since normally there are no 
	enum {g_nCDRSearchWindowSize = 0x100};

	void Clear();

	// reads everything and prepares the maps
	void Prepare();

	// searches for CDREnd record in the given file
	void FindCDREnd();// throw(ErrorEnum);

	// uses the found CDREnd to scan the CDR and probably the Zip file itself
	// builds up the m_mapFileEntries
	void BuildFileEntryMap();// throw (ErrorEnum);

	// give the CDR File Header entry, reads the local file header to validate and determine where
	// the actual file lies
	void AddFileEntry (const string& strFilePath, const ZipFile::CDRFileHeader* pFileHeader);// throw (ErrorEnum);

	// extracts the file path from the file header with subsequent information
	// may, or may not, put all letters to lower-case (depending on whether the system is to be case-sensitive or not)
	// it's the responsibility of the caller to ensure that the file name is in readable valid memory
	string GetFilePath (const ZipFile::CDRFileHeader* pFileHeader)
	{
		return GetFilePath((const char*)(pFileHeader + 1), pFileHeader->nFileNameLength);
	}
	// extracts the file path from the file header with subsequent information
	// may, or may not, put all letters to lower-case (depending on whether the system is to be case-sensitive or not)
	// it's the responsibility of the caller to ensure that the file name is in readable valid memory
	string GetFilePath (const ZipFile::LocalFileHeader* pFileHeader)
	{
		return GetFilePath((const char*)(pFileHeader + 1), pFileHeader->nFileNameLength);
	}
	// extracts the file path from the file header with subsequent information
	// may, or may not, put all letters to lower-case (depending on whether the system is to be case-sensitive or not)
	// it's the responsibility of the caller to ensure that the file name is in readable valid memory
	string GetFilePath (const char* pFileName, ZipFile::ushort nFileNameLength);

	// validates (if the init method has the corresponding value) the given file/header
	void Validate(const FileEntry& fileEntry);

	// initializes the actual data offset in the file in the fileEntry structure
	// searches to the local file header, reads it and calculates the actual offset in the file
	void InitDataOffset (FileEntry& fileEntry, const ZipFile::CDRFileHeader* pFileHeader);

	// seeks in the file relative to the starting position
	void Seek (ZipFile::ulong nPos, int nOrigin = SEEK_SET); // throw
	long Tell (); // throw
	void Read (void* pDest, unsigned nSize); // throw
protected:
	
	FILE* m_f;
	InitMethodEnum m_nInitMethod;
	unsigned m_nFlags;
	ZipFile::CDREnd m_CDREnd;

	// compatible with zlib memory-management functions used both
	// to allocate/free this instance and for decompression operations
	CMTSafeHeap* m_pHeap;

	unsigned m_nCDREndPos; // position of the CDR End in the file

	// Map: Relative file path => file entry info
	typedef std::map<string, ZipDir::FileEntry> FileEntryMap;
	FileEntryMap m_mapFileEntries;

	FileEntryTree m_treeFileEntries;

	bool m_bBuildFileEntryMap;
	bool m_bBuildFileEntryTree;
};
}

#endif