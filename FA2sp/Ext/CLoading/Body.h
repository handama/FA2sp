#pragma once

#include <CLoading.h>
#include "../FA2Expand.h"
#include "../../FA2sp/Helpers/FString.h"
#include <CShpFile.h>
#include <vector>
#include <array>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <CPalette.h>
#include <Drawing.h>
#include <fstream>
#include <set>

class ImageDataClass;
class Palette;

class NOVTABLE ImageDataClassSafe
{
public:

	std::unique_ptr<unsigned char[]> pImageBuffer; // draw from here, size = FullWidth*FullHeight

	struct ValidRangeData
	{
		short First;
		short Last;
	};
	std::unique_ptr<ValidRangeData[]> pPixelValidRanges;
	// size = FullHeight, stores the valid pixel from where to begin and end for each row
	// If it's an invalid row, then first = FullWidth - 1, second = 0 

	Palette* pPalette;
	short ValidX; // negative value for vxl, dunno why
	short ValidY; // negative value for vxl, dunno why
	short ValidWidth; // same as full for vxl, dunno why
	short ValidHeight; // same as full for vxl, dunno why
	short FullWidth;
	short FullHeight;
	ImageDataFlag Flag;
	BuildingImageFlag BuildingFlag; // see BuildingData
	BOOL IsOverlay; // Only OVRLXX_XX will set this true
};


class NOVTABLE ImageDataClassSurface
{
public:
	LPDIRECTDRAWSURFACE7 lpSurface; // Only available for flag = 0, if this is used, only ValidWidth and ValidHeight are set
	short FullWidth;
	short FullHeight;
	ImageDataFlag Flag;
};

struct ImageDataTransfer
{
	char imageID[0x100];
	ImageDataClassSafe pData;
};

class NOVTABLE CLoadingExt : public CLoading
{
public:

	//hook function to replace in virtual function map
	//BOOL PreTranslateMessageExt(MSG* pMsg);

	//static void ProgramStartupInit();

	static CLoadingExt* GetExtension()
	{
		return (CLoadingExt*)CLoading::Instance();
	}

	static bool HasFile_ReadyToReadFromFolder;

	bool InitMixFilesFix();
	static bool IsObjectLoaded(FString pRegName);
	static bool IsOverlayLoaded(FString pRegName);

	void LoadObjects(FString pRegName);
	void LoadOverlay(FString, int nIndex);
	
	// except buildings
	static FString GetImageName(FString ID, int nFacing, bool bShadow = false, bool bDeploy = false, bool bWater = false);
	static FString GetOverlayName(WORD ovr, BYTE ovrd, bool bShadow = false);
	// only buildings
	enum
	{
		GBIN_NORMAL,
		GBIN_RUBBLE,		
		GBIN_DAMAGED,
	};
	static FString GetBuildingImageName(FString ID, int nFacing, int state, bool bShadow = false);
	
	static void ClearItemTypes();
	void GetFullPaletteName(FString& PaletteName);
	static void LoadShp(FString ImageID, FString FileName, FString PalName, int nFrame, bool toServer = true);
	static void LoadShp(FString ImageID, FString FileName, Palette* pPal, int nFrame, bool toServer = true);
	static void LoadShpToSurface(FString ImageID, FString FileName, FString PalName, int nFrame);
	static void LoadShpToSurface(FString ImageID, unsigned char* pBuffer, int Width, int Height, Palette* pPal);
	static bool LoadShpToBitmap(ImageDataClassSafe* pData, CBitmap& outBitmap);
	static bool LoadShpToBitmap(ImageDataClass* pData, CBitmap& outBitmap);
	static void LoadSHPFrameSafe(int nFrame, int nFrameCount, unsigned char** ppBuffer, const ShapeHeader& header);
	static void LoadBitMap(FString ImageID, const CBitmap& cBitmap);
	void SetImageDataSafe(unsigned char* pBuffer, FString NameInDict, 
		int FullWidth, int FullHeight, Palette* pPal, bool toServer = true, bool clip = true);
	void SetImageData(unsigned char* pBuffer, FString NameInDict, int FullWidth, int FullHeight, Palette* pPal);
	// returns the mix index, -1 for in folder / pack, -2 for not found
	int HasFileMix(FString filename, int nMix = -114);

	static int GetITheaterIndex()
	{
		switch (CLoading::Instance->TheaterIdentifier)
		{
		case 'A':
			return 1;
		case 'U':
			return 2;
		case 'D':
			return 5;
		case 'L':
			return 4;
		case 'N':
			return 3;
		case 'T':
		default:
			return 0;
		}
	}
	// mode 0 = vanilla YR, mode 1 = Ares
	void SetTheaterLetter(FString& string, int mode = 1);
	void SetGenericTheaterLetter(FString& string);


private:
	static FString* __cdecl GetDictName(FString* ret, const char* ID, int nFacing) { JMP_STD(0x475450); }
	static FString GetDictName(FString ID, int nFacing)
	{
		FString buffer;
		GetDictName(&buffer, ID, nFacing);
		return buffer;
	}

	void LoadBuilding(FString ID);
	void LoadBuilding_Normal(FString ID);
	void LoadBuilding_Rubble(FString ID);
	void LoadBuilding_Damaged(FString ID, bool loadAsRubble = false);

	void LoadInfantry(FString ID);
	void LoadTerrainOrSmudge(FString ID, bool terrain);
	void LoadVehicleOrAircraft(FString ID);

	void SetImageDataSafe(unsigned char* pBuffer, ImageDataClassSafe* pData, int FullWidth, int FullHeight, Palette* pPal);
	void SetImageData(unsigned char* pBuffer, ImageDataClass* pData, int FullWidth, int FullHeight, Palette* pPal);
	void ShrinkSHP(unsigned char* pIn, int InWidth, int InHeight, unsigned char*& pOut, int* OutWidth, int* OutHeight);
	void UnionSHP_Add(unsigned char* pBuffer, int Width, int Height, int DeltaX = 0, int DeltaY = 0,
		bool UseTemp = false, bool bShadow = false, int ZAdjust = 0, int YSort = 0, bool MainBody = false);
	void UnionSHP_GetAndClear(unsigned char*& pOutBuffer, int* OutWidth, int* OutHeight,
		bool UseTemp = false, bool bShadow = false, bool bSort = false);
	void VXL_Add(unsigned char* pCache, int X, int Y, int Width, int Height, bool shadow = false);
	void VXL_GetAndClear(unsigned char*& pBuffer, int* OutWidth, int* OutHeight, bool shadow = false);
	
	void SetValidBuffer(ImageDataClass* pData, int Width, int Height);
	void SetValidBufferSafe(ImageDataClassSafe* pData, int Width, int Height);
	void TrimImageEdges(ImageDataClassSafe* pData);

	int ColorDistance(const ColorStruct& color1, const ColorStruct& color2); 
	std::vector<int> GeneratePalLookupTable(Palette* first, Palette* second);

public:
	enum class ObjectType{
		Unknown = -1,
		Infantry = 0,
		Vehicle = 1,
		Aircraft = 2,
		Building = 3,
		Terrain = 4,
		Smudge = 5
	};

	static FString GetArtID(FString ID);
	FString GetVehicleOrAircraftFileID(FString ID);
	FString GetTerrainOrSmudgeFileID(FString ID);
	FString GetBuildingFileID(FString ID);
	FString GetInfantryFileID(FString ID);
	static std::unordered_set<FString> LoadedOverlays;
	static Palette TempISOPalette;
	static bool IsLoadingObjectView;
	static std::unordered_set<FString> SwimableInfantries;
	ObjectType GetItemType(FString ID);
	static bool SaveCBitmapToFile(CBitmap* pBitmap, const FString& filePath, COLORREF bgColor);
	static bool LoadBMPToCBitmap(const FString& filePath, CBitmap& outBitmap);

	static std::unordered_map<FString, int> AvailableFacings;
	static std::unordered_set<FString> LoadedObjects;
	static std::unordered_set<int> Ra2dotMixes;
	static int TallestBuildingHeight;
private:

	void DumpFrameToFile(unsigned char* pBuffer, Palette* pPal, int Width, int Height, FString name);

	struct SHPUnionData
	{
		unsigned char* pBuffer;
		int Width;
		int Height;
		int DeltaX;
		int DeltaY;
		int ZAdjust = 0;
		int YSort = 0;
		bool MainBody = false;
	};

	static std::vector<SHPUnionData> UnionSHP_Data[2];
	static std::vector<SHPUnionData> UnionSHPShadow_Data[2];
	static std::unordered_map<FString, ObjectType> ObjectTypes;
	static unsigned char VXL_Data[0x10000];
	static unsigned char VXL_Shadow_Data[0x10000];


public:
	static std::unordered_map<FString, std::unique_ptr<ImageDataClassSafe>> CurrentFrameImageDataMap;
	static std::unordered_map<FString, std::unique_ptr<ImageDataClassSafe>> ImageDataMap;
	static std::unordered_map<FString, std::unique_ptr<ImageDataClassSurface>> SurfaceImageDataMap;

	static bool IsImageLoaded(const FString& name);
	static ImageDataClassSafe* GetImageDataFromMap(const FString& name);
	static ImageDataClassSafe* GetImageDataFromServer(const FString& name);
	static bool IsSurfaceImageLoaded(const FString& name);
	static ImageDataClassSurface* GetSurfaceImageDataFromMap(const FString& name);
	static int GetAvailableFacing(const FString& ID);

	static HANDLE hPipeData;
	static std::atomic<bool> PingServerRunning;
	static FString PipeNameData;
	static FString PipeNamePing;
	static FString PipeName;

	static bool StartImageServerProcess();
	static bool ConnectToImageServer();
	static void StartPingThread();

	static bool WriteImageData(HANDLE hPipe, const FString& imageID, const ImageDataClassSafe* data);
	static bool ReadImageData(HANDLE hPipe, ImageDataClassSafe& data);
	static bool SendImageToServer(const FString& imageID, const ImageDataClassSafe* imageData);
	static bool RequestImageFromServer(const FString& imageID, ImageDataClassSafe& outImageData);
	static void SendRequestText(const char* text);
};

#pragma pack(push, 1)
struct FileEntry
{
	uint32_t offset;
	uint32_t enc_size;
	uint32_t original_size;
};
#pragma pack(pop)
static_assert(sizeof(FileEntry) == 12, "FileEntry must be 12 bytes");

struct CaseInsensitiveHash 
{
	size_t operator()(const FString& key) const {
		FString lower;
		lower.reserve(key.size());
		for (char ch : key) lower.push_back(std::tolower(static_cast<unsigned char>(ch)));
		return std::hash<FString>()(lower);
	}
};

struct CaseInsensitiveEqual 
{
	bool operator()(const FString& lhs, const FString& rhs) const {
		if (lhs.size() != rhs.size()) return false;
		for (size_t i = 0; i < lhs.size(); ++i) {
			if (std::tolower(static_cast<unsigned char>(lhs[i])) != std::tolower(static_cast<unsigned char>(rhs[i])))
				return false;
		}
		return true;
	}
};

class ResourcePack 
{
public:
	bool load(const FString& filename);
	std::unique_ptr<uint8_t[]> getFileData(const FString& filename, size_t* out_size = nullptr);

private:
	std::unordered_map<FString, FileEntry, CaseInsensitiveHash, CaseInsensitiveEqual> index_map;
	uint32_t index_size = 0; 
	FString file_path;
	std::ifstream file_stream; 

	bool aesDecryptBlockwise(const uint8_t* input, size_t len, std::vector<uint8_t>& output);
	std::array<uint8_t, 32> get_aes_key();
	FString toHex(const unsigned char* data, size_t len);
	FString encrypt_filename(const FString& filename, const unsigned char* key);
};

class ResourcePackManager 
{
public:

	static ResourcePackManager& instance();
	bool loadPack(const FString& packPath);
	std::unique_ptr<uint8_t[]> getFileData(const FString& filename, size_t* out_size = nullptr);
	void clear();

private:
	std::vector<std::unique_ptr<ResourcePack>> packs;
};

struct MixEntry {
	uint32_t id;
	uint32_t offset;
	uint32_t size;
};

struct MixFile {
	std::string path;
	std::ifstream stream;
	std::vector<MixEntry> entries;
	bool isNested = false; 
	uint32_t baseOffset = 0;
};

struct FindFileHelper {
	uint32_t mixIndex;
	const MixEntry* entry;
};

class MixLoader {
public:
	static MixLoader& Instance();

	MixLoader(const MixLoader&) = delete;
	MixLoader& operator=(const MixLoader&) = delete;

	bool LoadTopMix(const std::string& path);
	bool LoadNestedMix(MixFile& parent, const MixEntry& entry);
	bool LoadMixFile(const std::string& path, int* parentIndex = nullptr);
	int QueryFileIndex(const std::string& fileName, int mixIdx = -1);
	std::unique_ptr<uint8_t[]> LoadFile(const std::string& fileName, size_t* outSize, int mixIdx = -1);
	bool ExtractFile(const std::string& fileName, const std::string& outPath, int mixIdx = -1);
	void Clear();

	static uint32_t GetFileID(const std::string& fileName);

private:
	MixLoader() = default;
	~MixLoader() = default;

	std::vector<MixFile> mixFiles;
	std::unordered_map<uint32_t, FindFileHelper> fileMap;
};