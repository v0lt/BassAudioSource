/*
 *  Copyright (C) 2023-2024 v0lt
 */

#pragma once

#include "IBassSource.h"

// CBassMainPPage

class __declspec(uuid("D2E835E8-7673-4922-8CEA-5BF963FA56FD"))
	CBassMainPPage : public CBasePropertyPage, public CWindow
{
	CComQIPtr<IBassSource> m_pBassSource;
	Settings_t m_SetsPP;

	HFONT m_hMonoFont = nullptr;

public:
	CBassMainPPage(LPUNKNOWN lpunk, HRESULT* phr);
	~CBassMainPPage();

private:
	HRESULT OnConnect(IUnknown* pUnknown) override;
	HRESULT OnDisconnect() override;
	HRESULT OnActivate() override;
	void SetDirty()
	{
		m_bDirty = TRUE;
		if (m_pPageSite) {
			m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
		}
	}
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	HRESULT OnApplyChanges() override;
};
