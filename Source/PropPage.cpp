/*
 *  Copyright (C) 2023-2024 v0lt
 */

#include "stdafx.h"
#include "resource.h"
#include "Helper.h"
#include "Utils/Util.h"
#include "Utils/StringUtil.h"
#include "PropPage.h"

void SetCursor(HWND hWnd, LPCWSTR lpCursorName)
{
	SetClassLongPtrW(hWnd, GCLP_HCURSOR, (LONG_PTR)::LoadCursorW(nullptr, lpCursorName));
}

void SetCursor(HWND hWnd, UINT nID, LPCWSTR lpCursorName)
{
	SetCursor(::GetDlgItem(hWnd, nID), lpCursorName);
}

inline void ComboBox_AddStringData(HWND hWnd, int nIDComboBox, LPCWSTR str, LONG_PTR data)
{
	LRESULT lValue = SendDlgItemMessageW(hWnd, nIDComboBox, CB_ADDSTRING, 0, (LPARAM)str);
	if (lValue != CB_ERR) {
		SendDlgItemMessageW(hWnd, nIDComboBox, CB_SETITEMDATA, lValue, data);
	}
}

inline LONG_PTR ComboBox_GetCurItemData(HWND hWnd, int nIDComboBox)
{
	LRESULT lValue = SendDlgItemMessageW(hWnd, nIDComboBox, CB_GETCURSEL, 0, 0);
	if (lValue != CB_ERR) {
		lValue = SendDlgItemMessageW(hWnd, nIDComboBox, CB_GETITEMDATA, lValue, 0);
	}
	return lValue;
}

inline std::wstring ComboBox_GetCurItemText(HWND hWnd, int nIDComboBox)
{
	std::wstring str;

	LRESULT idx = SendDlgItemMessageW(hWnd, nIDComboBox, CB_GETCURSEL, 0, 0);
	if (idx != CB_ERR) {
		LRESULT lValue = SendDlgItemMessageW(hWnd, nIDComboBox, CB_GETLBTEXTLEN, idx, 0);
		if (lValue != CB_ERR) {
			str.assign(lValue, L'\0');
			lValue = SendDlgItemMessageW(hWnd, nIDComboBox, CB_GETLBTEXT, idx, (LPARAM)str.data());
		}
	}
	str_truncate_after_null(str);
	return str;
}

void ComboBox_SelectByItemData(HWND hWnd, int nIDComboBox, LONG_PTR data)
{
	LRESULT lCount = SendDlgItemMessageW(hWnd, nIDComboBox, CB_GETCOUNT, 0, 0);
	if (lCount != CB_ERR) {
		for (int idx = 0; idx < lCount; idx++) {
			const LRESULT lValue = SendDlgItemMessageW(hWnd, nIDComboBox, CB_GETITEMDATA, idx, 0);
			if (data == lValue) {
				SendDlgItemMessageW(hWnd, nIDComboBox, CB_SETCURSEL, idx, 0);
				break;
			}
		}
	}
}


// CBassMainPPage

// https://msdn.microsoft.com/ru-ru/library/windows/desktop/dd375010(v=vs.85).aspx

CBassMainPPage::CBassMainPPage(LPUNKNOWN lpunk, HRESULT* phr) :
	CBasePropertyPage(L"MainProp", lpunk, IDD_MAINPROPPAGE, IDS_MAINPROPPAGE_TITLE)
{
	DLog(L"CBassMainPPage()");
}

CBassMainPPage::~CBassMainPPage()
{
	DLog(L"~CBassMainPPage()");
}

HRESULT CBassMainPPage::OnConnect(IUnknown *pUnk)
{
	if (pUnk == nullptr) return E_POINTER;

	m_pBassSource = pUnk;
	if (!m_pBassSource) {
		return E_NOINTERFACE;
	}

	return S_OK;
}

HRESULT CBassMainPPage::OnDisconnect()
{
	if (m_pBassSource == nullptr) {
		return E_UNEXPECTED;
	}

	m_pBassSource.Release();

	return S_OK;
}

HRESULT CBassMainPPage::OnActivate()
{
	// set m_hWnd for CWindow
	m_hWnd = m_hwnd;

	m_pBassSource->GetSettings(m_SetsPP);

	CheckDlgButton(IDC_CHECK1, m_SetsPP.bMidiEnable ? BST_CHECKED : BST_UNCHECKED);
	GetDlgItem(IDC_STATIC1).EnableWindow(m_SetsPP.bMidiEnable);
	GetDlgItem(IDC_COMBO1).EnableWindow(m_SetsPP.bMidiEnable);

	const std::wstring filterDir = GetFilterDirectory();
	std::vector<std::wstring> soundFontFiles;
	int listPos = -1;
	for (auto& p : std::filesystem::recursive_directory_iterator(filterDir)) {
		if (std::filesystem::is_regular_file(p)) {
			std::wstring ext = p.path().extension();
			str_tolower(ext);
			if (ext == L".sf2" || ext == L".sfz") {
				std::wstring filename = p.path().filename();
				if (listPos < 0
					&& filename.length() == m_SetsPP.sMidiSoundFontDefault.length()
					&& _wcsnicmp(filename.c_str(), m_SetsPP.sMidiSoundFontDefault.c_str(), filename.length()) == 0) {
					listPos = (int)soundFontFiles.size();
				}
				soundFontFiles.emplace_back(filename);
			}
		}
	}
	if (listPos < 0) {
		soundFontFiles.insert(soundFontFiles.cbegin(), m_SetsPP.sMidiSoundFontDefault);
		listPos = 0;
	}
	for (const auto& sff : soundFontFiles) {
		SendDlgItemMessageW(IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)sff.c_str());
	}
	SendDlgItemMessageW(IDC_COMBO1, CB_SETCURSEL, listPos, 0);

	// init monospace font
	LOGFONTW lf = {};
	HDC hdc = GetWindowDC();
	lf.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(hdc);
	lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	wcscpy_s(lf.lfFaceName, L"Consolas");
	m_hMonoFont = CreateFontIndirectW(&lf);

	GetDlgItem(IDC_EDIT1).SetFont(m_hMonoFont);

	std::wstring strInfo;
	m_pBassSource->GetInfo(strInfo);
	str_replace(strInfo, L"\n", L"\r\n");
	SetDlgItemTextW(IDC_EDIT1, strInfo.c_str());

	SetDlgItemTextW(IDC_EDIT3, GetNameAndVersion());

	SetCursor(m_hWnd, IDC_ARROW);

	return S_OK;
}

INT_PTR CBassMainPPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND) {
		const int nID = LOWORD(wParam);
		int action = HIWORD(wParam);

		if (action == BN_CLICKED) {
			if (nID == IDC_CHECK1) {
				m_SetsPP.bMidiEnable = IsDlgButtonChecked(IDC_CHECK1) == BST_CHECKED;
				GetDlgItem(IDC_STATIC1).EnableWindow(m_SetsPP.bMidiEnable);
				GetDlgItem(IDC_COMBO1).EnableWindow(m_SetsPP.bMidiEnable);
				SetDirty();
				return (LRESULT)1;
			}
		}
		else if (action == CBN_SELCHANGE) {
			if (nID == IDC_COMBO1) {
				std::wstring str = ComboBox_GetCurItemText(m_hWnd, IDC_COMBO1);
				if (str.length()) {
					m_SetsPP.sMidiSoundFontDefault = str;
					SetDirty();
					return (LRESULT)1;
				}
			}
		}
	}

	// Let the parent class handle the message.
	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CBassMainPPage::OnApplyChanges()
{
	m_pBassSource->SetSettings(m_SetsPP);
	m_pBassSource->SaveSettings();

	return S_OK;
}
