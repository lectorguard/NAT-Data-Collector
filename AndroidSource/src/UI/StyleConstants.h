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