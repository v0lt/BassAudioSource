/*
* (C) 2020-2023 see Authors.txt
*
* This file is part of MPC-BE.
*
* MPC-BE is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* MPC-BE is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#ifndef FCC
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))
#endif

template <typename... Args>
inline void DebugLogFmt(std::wstring_view format, Args&& ...args)
{
	DbgLogInfo(LOG_TRACE, 3, std::vformat(format, std::make_wformat_args(args...)).c_str());
}

#ifdef _DEBUG
#define DLog(...) DebugLogFmt(__VA_ARGS__)
#define DLogIf(f,...) {if (f) DebugLogFmt(__VA_ARGS__);}
#define DLogError(...) DebugLogFmt(LOG_ERROR, 3, __VA_ARGS__)
#else
#define DLog(...) __noop
#define DLogIf(f,...) __noop
#define DLogError(...) __noop
#endif

#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#define SAFE_CLOSE_HANDLE(p) { if (p) { if ((p) != INVALID_HANDLE_VALUE) ASSERT(CloseHandle(p)); (p) = nullptr; } }
#define SAFE_DELETE(p)       { if (p) { delete (p); (p) = nullptr; } }

#define QI(i) (riid == __uuidof(i)) ? GetInterface((i*)this, ppv) :

#define ALIGN(x, a)           __ALIGN_MASK(x,(decltype(x))(a)-1)
#define __ALIGN_MASK(x, mask) (((x)+(mask))&~(mask))


// A byte that is not initialized to std::vector when using the resize method.
// Note: can be slow in debug mode.
struct NoInitByte
{
	uint8_t value;
#pragma warning(push)
#pragma warning(disable:26495)
	NoInitByte() {
		// do nothing
		static_assert(sizeof(*this) == sizeof(value), "invalid size");
	}
#pragma warning(pop)
};

template <typename T>
// If the specified value is out of range, set to default values.
inline T discard(T const& val, T const& def, T const& lo, T const& hi)
{
	return (val > hi || val < lo) ? def : val;
}

template <typename T>
inline T round_pow2(T number, T pow2)
{
	ASSERT(pow2 > 0);
	ASSERT(!(pow2 & (pow2 - 1)));
	--pow2;
	if (number < 0) {
		return (number - pow2) & ~pow2;
	} else {
		return (number + pow2) & ~pow2;
	}
}

inline std::wstring GUIDtoWString(const GUID& guid)
{
	std::wstring str(39, 0);
	int ret = StringFromGUID2(guid, &str[0], 39);
	if (ret) {
		str.resize(ret - 1);
	} else {
		str.clear();
	}
	return str;
}

std::wstring HR2Str(const HRESULT hr);
