/****************************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/										*
* This software is available under the "MIT License".                                               *
* https://github.com/jovibor/Pepper/blob/master/LICENSE												*
* Pepper - PE (x86) and PE+ (x64) files viewer, based on libpe: https://github.com/jovibor/Pepper	*
* libpe - Windows library for reading PE (x86) and PE+ (x64) files inner structure information.		*
* https://github.com/jovibor/libpe																	*
****************************************************************************************************/
#pragma once
#include "constants.h"
#include "SplitterEx.h"

class CChildFrame : public CMDIChildWndEx
{
public:
	auto GetWndStatData()->std::vector<SWINDOWSTATUS>&;
	void SetWindowStatus(CWnd* pWnd, bool fVisible);
	DECLARE_DYNCREATE(CChildFrame)
	CSplitterEx m_stSplitterMain, m_stSplitterRight, m_stSplitterRightTop, m_stSplitterRightBottom;
protected:
	virtual ~CChildFrame() {};
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnClose();
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;
	DECLARE_MESSAGE_MAP()
private:
	bool m_fSplitterCreated { false };
	UINT m_cx { }, m_cy { };
	std::vector<SWINDOWSTATUS> m_vecWndStatus { };
};
