#pragma once

#include <CMapValidator.h>
#include <unordered_set>
#include "../FA2sp/Ext/CMapData/Body.h"

enum class ValidatorJumpType
{
	None = 0,
	Team,
	Trigger,
	Tag,
	Taskforce,
	Script,
	AITrigger,
};

struct ValidatorJumpTarget
{
	enum class Kind { MapCoord, Object };

	Kind kind = Kind::MapCoord;
	MapCoord coord{ 0, 0 };
	FString id;
	ValidatorJumpType type = ValidatorJumpType::None;
	int eventIndex = -1;
	int actionIndex = -1;
};

class CMapValidatorExt : public CMapValidator
{
public:
	int DoValidator();
	bool IsYRMap();

	void ValidateMapStructureChecks(BOOL& result);
	void ValidateOverlayLimit(BOOL& result);
	void ValidateStructureOverlapping(BOOL& result);
	void ValidateMissingParams(BOOL& result);
	void ValidateLoopTrigger(BOOL& result);
	void ValidateLoopTrigger_loop(ppmfc::CString attachedTrigger);
	void ValidateRepeatingTaskforce(BOOL& result);
	void ValidateBaseNode(BOOL& result);
	void ValidateValueLength(BOOL& result);
	void ValidateEmptyTeamTrigger(BOOL& result);
	void ValidateTubes(BOOL& result);

	LPARAM PushCoordTarget(const MapCoord& coord);
	LPARAM PushObjectTarget(ValidatorJumpType type, const FString& id, int eventIndex = -1, int actionIndex = -1);
	bool JumpToObject(const ValidatorJumpTarget& target);

	ppmfc::CString FetchLanguageString(const char* Key, const char* def);
	void InsertStringAsError(const char* String, LPARAM lParam = NULL);
	void InsertString(const char* String, bool IsWarning, LPARAM lParam = NULL);

	static std::unordered_set<std::string> StructureOverlappingIgnores;
	static std::vector<FString> AttachedTriggers;
	static std::vector<FString> LoopedTriggers;
	static std::list<ValidatorJumpTarget> JumpTargets;

	static void ProgramStartupInit();
	BOOL PreTranslateMessageExt(MSG* pMsg);
	void OnOKExt();
	void OnCancelExt();
};