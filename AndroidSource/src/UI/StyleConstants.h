#pragma once

namespace StyleConst
{
	inline const float LeftColumnWidth = 0.33f;
	inline const float ScrollbarSizeCM = 0.45f;
}

namespace MainScreenConst
{
	inline const float UserWinSize = 0.6f;
	inline const float TabWinSize = 0.05f;
	inline const float BotWinSize = 0.35f;
	inline const float UserWinCursor = 0.0f;
	inline const float TabWinCursor = UserWinCursor + UserWinSize;
	inline const float BotWinCursor = TabWinCursor + TabWinSize;
}

namespace TraversalWindowConst
{
	inline const uint16_t lobbies_per_line = 2u;
}

namespace ButtonColors
{
	inline const ImVec4 Unselected	= { 82 / 255.0f, 82 / 255.0f, 82 / 255.0f, 1.0f };
	inline const ImVec4 Pressed		= { 236 / 255.0f, 98 / 255.0f, 95 / 255.0f, 1.0f };
	inline const ImVec4 Marked		= { 49 / 255.0f, 49 / 255.0f, 49 / 255.0f, 1.0f };
	inline const ImVec4 Selected	= { 137 / 255.0f, 22 / 255.0f, 82 / 255.0f, 1.0f }; 
}

namespace FontSizes
{
	inline const float font_large_cm = 0.55f;
	inline const float font_med_large_cm = 0.34f;
	inline const float font_medium_cm = 0.29f;
	inline const float font_small_cm = 0.2f;
	inline const float font_very_small_cm = 0.1f;
}