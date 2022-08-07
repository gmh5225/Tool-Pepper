/****************************************************************************************************
* Copyright © 2018-2021 Jovibor https://github.com/jovibor/   										*
* This software is available under the "MIT License".                                               *
* https://github.com/jovibor/Pepper/blob/master/LICENSE												*
* Pepper - PE (x86) and PE+ (x64) files viewer, based on libpe: https://github.com/jovibor/Pepper	*
* libpe - Windows library for reading PE (x86) and PE+ (x64) files inner structure information.		*
* https://github.com/jovibor/libpe																	*
****************************************************************************************************/
#include "stdafx.h"
#include "CViewRightBR.h"
#include "MainFrm.h"
#include "Utility.h"
#include <format>
#pragma comment(lib, "Mincore.lib") //VerQueryValueW

BEGIN_MESSAGE_MAP(CWndSampleDlg, CWnd)
	ON_WM_PAINT()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

void CWndSampleDlg::Attach(CImageList* pImgList, CChildFrame* pChildFrame)
{
	m_pImgRes = pImgList;
	m_pChildFrame = pChildFrame;
}

void CWndSampleDlg::CreatedForMenu(bool fMenu)
{
	m_fMenu = fMenu;
}

void CWndSampleDlg::SetDlgVisible(bool fVisible)
{
	if (m_pChildFrame == nullptr)
		return;

	ShowWindow(fVisible ? SW_SHOW : SW_HIDE);
	m_pChildFrame->SetWindowStatus(m_hWnd, fVisible);
}

void CWndSampleDlg::OnPaint()
{
	CPaintDC dc(this);

	CRect rc;
	GetClientRect(rc);
	dc.FillSolidRect(rc, RGB(255, 255, 255));

	if (m_fMenu) {
		dc.SetTextColor(RGB(255, 0, 0));
		dc.TextOutW(5, 5, L"Sample dialog for showing menu resource");
	}
	else if (m_pImgRes != nullptr) {
		m_pImgRes->Draw(&dc, 0, POINT { 0, 0 }, ILD_NORMAL);
	}
}

void CWndSampleDlg::OnClose()
{
	SetDlgVisible(false);
	CWnd::OnClose();
}


IMPLEMENT_DYNCREATE(CViewRightBR, CScrollView)

BEGIN_MESSAGE_MAP(CViewRightBR, CScrollView)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CViewRightBR::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	SetScrollSizes(MM_TEXT, CSize(0, 0));

	m_pChildFrame = static_cast<CChildFrame*>(GetParentFrame());
	m_pMainDoc = static_cast<CPepperDoc*>(GetDocument());
	m_pLibpe = m_pMainDoc->m_pLibpe.get();

	m_EditBRB.Create(WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL
		| ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL, CRect(0, 0, 0, 0), this, 0x01);

	m_wndSampleDlg.Attach(&m_stImgRes, m_pChildFrame);

	LOGFONTW lf { };
	StringCchCopyW(lf.lfFaceName, 9, L"Consolas");
	lf.lfHeight = 18;
	if (!m_fontEditRes.CreateFontIndirectW(&lf))
	{
		NONCLIENTMETRICSW ncm { };
		ncm.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		m_fontEditRes.CreateFontIndirectW(&ncm.lfMessageFont);
	}
	m_EditBRB.SetFont(&m_fontEditRes);

	m_stlcs.stColor.clrTooltipText = RGB(255, 255, 255);
	m_stlcs.stColor.clrTooltipBk = RGB(0, 132, 132);
	m_stlcs.stColor.clrHdrText = RGB(255, 255, 255);
	m_stlcs.stColor.clrHdrBk = RGB(0, 132, 132);
	m_stlcs.stColor.clrHdrHglInact = RGB(0, 112, 112);
	m_stlcs.stColor.clrHdrHglAct = RGB(0, 92, 92);
	m_stlcs.pParent = this;
	m_stlcs.dwHdrHeight = 35;

	const auto pDC = GetDC();
	const auto iLOGPIXELSY = GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
	m_lf.lfHeight = m_hdrlf.lfHeight = -MulDiv(11, iLOGPIXELSY, 72);
	ReleaseDC(pDC);

	StringCchCopyW(m_lf.lfFaceName, 9, L"Consolas");
	m_stlcs.pListLogFont = &m_lf;
	m_hdrlf.lfWeight = FW_BOLD;
	StringCchCopyW(m_hdrlf.lfFaceName, 16, L"Times New Roman");
	m_stlcs.pHdrLogFont = &m_hdrlf;

	CreateListTLSCallbacks();
}

void CViewRightBR::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* pHint)
{
	//If it's UpdateAllViews call for top right Hex (IDC_HEX_RIGHT_TR), (from top left Resource Tree) we do nothing.
	if (!m_pChildFrame || LOWORD(lHint) == IDC_HEX_RIGHT_TR || LOWORD(lHint) == ID_DOC_EDITMODE)
		return;

	//If any but Resources Update we destroy m_DlgSampleWnd, if it's currently created.
	if (LOWORD(lHint) != IDC_SHOW_RESOURCE_RBR) {
		if (m_wndSampleDlg.m_hWnd)
			m_wndSampleDlg.DestroyWindow();
	}

	if (m_hwndActive)
		::ShowWindow(m_hwndActive, SW_HIDE);

	CRect rcParent;
	GetParent()->GetWindowRect(&rcParent);
	CRect rcClient;
	GetClientRect(&rcClient);
	m_fDrawRes = false;

	switch (LOWORD(lHint))
	{
	case IDC_LIST_TLS:
		m_stListTLSCallbacks->SetWindowPos(this, 0, 0, rcClient.Width(), rcClient.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_hwndActive = m_stListTLSCallbacks->m_hWnd;
		m_pChildFrame->m_stSplitterRightBottom.ShowCol(1);
		m_pChildFrame->m_stSplitterRightBottom.SetColumnInfo(0, rcParent.Width() / 2, 0);
		break;
	case IDC_TREE_RESOURCE:
		m_pChildFrame->m_stSplitterRightBottom.ShowCol(1);
		m_pChildFrame->m_stSplitterRightBottom.SetColumnInfo(0, rcParent.Width() / 3, 0);
		break;
	case IDC_SHOW_RESOURCE_RBR:
		ShowResource(reinterpret_cast<SRESDATA*>(pHint));
		m_pChildFrame->m_stSplitterRightBottom.ShowCol(1);
		m_pChildFrame->m_stSplitterRightBottom.SetColumnInfo(0, rcParent.Width() / 3, 0);
		break;
	case IDC_LIST_DEBUG_ENTRY:
		CreateDebugEntry(HIWORD(lHint));
		m_pChildFrame->m_stSplitterRightBottom.ShowCol(1);
		m_pChildFrame->m_stSplitterRightBottom.SetColumnInfo(0, rcParent.Width() / 2, 0);
		break;
	default:
		m_pChildFrame->m_stSplitterRightBottom.HideCol(1);
	}

	m_pChildFrame->m_stSplitterRightBottom.RecalcLayout();
}

void CViewRightBR::OnDraw(CDC* pDC)
{
	CRect rcClient;
	GetClientRect(rcClient);

	if (!m_fDrawRes) {
		pDC->FillSolidRect(rcClient, RGB(255, 255, 255));
		return;
	}

	const auto sizeScroll = GetTotalSize();
	CPoint ptDrawAt;
	int x, y;

	switch (m_iResTypeToDraw)
	{
	case 1: //RT_CURSOR
	case 2: //RT_BITMAP
	case 3: //RT_ICON
	{
		//Drawing in the center, independently from scroll pos.
		pDC->FillSolidRect(rcClient, m_clrBkIcons);

		if (sizeScroll.cx > rcClient.Width())
			x = sizeScroll.cx / 2 - (m_stBmp.bmWidth / 2);
		else
			x = rcClient.Width() / 2 - (m_stBmp.bmWidth / 2);

		if (sizeScroll.cy > rcClient.Height())
			y = sizeScroll.cy / 2 - (m_stBmp.bmHeight / 2);
		else
			y = rcClient.Height() / 2 - (m_stBmp.bmHeight / 2);

		ptDrawAt.SetPoint(x, y);
		m_stImgRes.Draw(pDC, 0, ptDrawAt, ILD_NORMAL);
		break;
	}
	case 5: //RT_DIALOG
		break;
	case 12: //RT_GROUP_CURSOR
	case 14: //RT_GROUP_ICON
	{
		pDC->FillSolidRect(rcClient, m_clrBkIcons);

		if (sizeScroll.cx > rcClient.Width())
			x = sizeScroll.cx / 2 - (m_iImgResWidth / 2);
		else
			x = rcClient.Width() / 2 - (m_iImgResWidth / 2);

		for (const auto& iter : m_vecImgRes)
		{
			IMAGEINFO imginfo;
			iter->GetImageInfo(0, &imginfo);
			int iImgHeight = imginfo.rcImage.bottom - imginfo.rcImage.top;
			if (sizeScroll.cy > rcClient.Height())
				y = sizeScroll.cy / 2 - (iImgHeight / 2);
			else
				y = rcClient.Height() / 2 - (iImgHeight / 2);

			ptDrawAt.SetPoint(x, y);
			iter->Draw(pDC, 0, ptDrawAt, ILD_NORMAL);
			x += imginfo.rcImage.right - imginfo.rcImage.left;
		}
		break;
	}
	case 0xFF:
		pDC->FillSolidRect(rcClient, RGB(255, 255, 255));
		pDC->SetTextColor(RGB(255, 0, 0));
		pDC->TextOutW(0, 0, L"Unable to load resource! It's either damaged, packed or zero-length.");
		break;
	default:
		pDC->FillSolidRect(rcClient, RGB(255, 255, 255));
		pDC->TextOutW(0, 0, L"This Resource type is not supported.");
	}
}

BOOL CViewRightBR::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CViewRightBR::OnSize(UINT nType, int cx, int cy)
{
	CScrollView::OnSize(nType, cx, cy);

	if (m_hwndActive)
		::SetWindowPos(m_hwndActive, m_hWnd, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CViewRightBR::CreateIconCursor(const SRESDATA& stResData)
{
	const auto hIcon = CreateIconFromResourceEx(const_cast<PBYTE>(reinterpret_cast<const BYTE*>(stResData.pData->data())),
		static_cast<DWORD>(stResData.pData->size()),
		(stResData.wIdType == 3) ? TRUE : FALSE, 0x00030000, 0, 0, LR_DEFAULTCOLOR);
	if (!hIcon)
		return ResLoadError();

	ICONINFO iconInfo;
	if (!GetIconInfo(hIcon, &iconInfo))
		return ResLoadError();
	if (!GetObjectW(iconInfo.hbmMask, sizeof(BITMAP), &m_stBmp))
		return ResLoadError();
	DeleteObject(iconInfo.hbmColor);
	DeleteObject(iconInfo.hbmMask);

	m_stImgRes.Create(m_stBmp.bmWidth,
		(stResData.wIdType == 3) ? m_stBmp.bmHeight : m_stBmp.bmWidth, ILC_COLORDDB, 0, 1);
	m_stImgRes.SetBkColor(m_clrBkImgList);
	if (m_stImgRes.Add(hIcon) == -1)
		return ResLoadError();

	DestroyIcon(hIcon);
	SetScrollSizes(MM_TEXT, CSize(m_stBmp.bmWidth,
		(stResData.wIdType == 3) ? m_stBmp.bmHeight : m_stBmp.bmWidth));

	m_iResTypeToDraw = stResData.wIdType;
	m_fDrawRes = true;
}

void CViewRightBR::CreateBitmap(const SRESDATA& stResData)
{
	const auto pBMPInfo = reinterpret_cast<const BITMAPINFO*>(stResData.pData->data());
	const auto pBMPHdr = &pBMPInfo->bmiHeader;
	const auto nNumColors = pBMPHdr->biClrUsed > 0 ? pBMPHdr->biClrUsed : 1 << pBMPHdr->biBitCount;

	LPCVOID pDIBBits;
	if (pBMPHdr->biBitCount > 8) {
		pDIBBits = static_cast<LPCVOID>(reinterpret_cast<const DWORD*>(pBMPInfo->bmiColors + pBMPHdr->biClrUsed) +
			((pBMPHdr->biCompression == BI_BITFIELDS) ? 3 : 0));
	}
	else
		pDIBBits = static_cast<LPCVOID>(pBMPInfo->bmiColors + nNumColors);

	const auto hDC = ::GetDC(m_hWnd);
	const auto hBitmap = CreateDIBitmap(hDC, pBMPHdr, CBM_INIT, pDIBBits, pBMPInfo, DIB_RGB_COLORS);
	::ReleaseDC(m_hWnd, hDC);
	if (!hBitmap)
		return ResLoadError();

	if (!GetObjectW(hBitmap, sizeof(BITMAP), &m_stBmp))
		return ResLoadError();

	CBitmap bmp;
	if (!bmp.Attach(hBitmap))
		return ResLoadError();

	m_stImgRes.Create(m_stBmp.bmWidth, m_stBmp.bmHeight, ILC_COLORDDB, 0, 1);
	if (m_stImgRes.Add(&bmp, nullptr) == -1)
		return ResLoadError();

	m_iResTypeToDraw = 2;
	m_fDrawRes = true;
	SetScrollSizes(MM_TEXT, CSize(m_stBmp.bmWidth, m_stBmp.bmHeight));
	bmp.DeleteObject();
}

void CViewRightBR::CreateMenu(const SRESDATA& stResData)
{
	if (stResData.pData == nullptr || stResData.pData->empty())
		return;

	m_menuSample.DestroyMenu();
	if (m_menuSample.LoadMenuIndirectW(stResData.pData->data())) {
		for (auto iter = 0; iter < m_menuSample.GetMenuItemCount(); ++iter) {
			CString strName;
			m_menuSample.GetMenuStringW(iter, strName, MF_BYPOSITION);
			if (strName.GetLength() == 0) { //Setting name for all empty menus to "<Empty>", to be clickable.
				strName = L"<Empty>";
				MENUITEMINFOW mii { };
				mii.cbSize = sizeof(MENUITEMINFOW);
				mii.fMask = MIIM_STRING;
				mii.dwTypeData = const_cast<LPWSTR>(strName.GetString());
				m_menuSample.SetMenuItemInfoW(iter, &mii, TRUE);
				m_menuSample.EnableMenuItem(iter, MF_ENABLED | MF_BYPOSITION);
			}
		}

		int iPosX = 0, iPosY = 0;
		constexpr auto iWidth = 700;
		constexpr auto iHeight = 100;
		UINT uFlags = SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER;
		constexpr DWORD dwStyles { WS_POPUP | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX };
		if (!m_wndSampleDlg.m_hWnd)
		{
			if (!m_wndSampleDlg.CreateEx(WS_EX_APPWINDOW, AfxRegisterWndClass(0),
				L"Sample Dialog...", dwStyles, 0, 0, 0, 0, m_hWnd, nullptr)) {
				MessageBoxW(L"Sample Dialog window CreateEx failed.", L"Error");
				return;
			}
			iPosX = (GetSystemMetrics(SM_CXSCREEN) / 2) - iWidth / 2;
			iPosY = (GetSystemMetrics(SM_CYSCREEN) / 2) - iHeight / 2;
			uFlags &= ~SWP_NOMOVE;
		}
		m_wndSampleDlg.CreatedForMenu(true);
		m_wndSampleDlg.SetMenu(&m_menuSample);
		m_wndSampleDlg.SetWindowPos(this, iPosX, iPosY, iWidth, iHeight, uFlags);
		m_wndSampleDlg.SetDlgVisible(true);
		m_wndSampleDlg.RedrawWindow();
	}
}

void CViewRightBR::CreateDlg(const SRESDATA& stResData)
{
#pragma pack(push, 4)
	struct DLGTEMPLATEEX //Helper struct. Not completed.
	{
		WORD      dlgVer;
		WORD      signature;
		DWORD     helpID;
		DWORD     exStyle;
		DWORD     style;
		WORD      cDlgItems;
		short     x;
		short     y;
		short     cx;
		short     cy;
		WORD      menu;
	};
#pragma pack(pop)

	const auto optData = ParceDlgTemplate({ const_cast<std::byte*>(stResData.pData->data()), stResData.pData->size() });
	if (!optData)
		return ResLoadError();

	const bool fDlgEx = *((reinterpret_cast<const WORD*>(stResData.pData->data())) + 1) == 0xFFFF;

	//Pointer to 'Style' dword.
	PDWORD pStyle { fDlgEx ? &(const_cast<DLGTEMPLATEEX*>(reinterpret_cast<const DLGTEMPLATEEX*>(stResData.pData->data())))->style :
		&(const_cast<DLGTEMPLATE*>(reinterpret_cast<const DLGTEMPLATE*>(stResData.pData->data())))->style };

	bool fWS_VISIBLE { false };
	if (*pStyle & WS_VISIBLE) { //Remove WS_VISIBLE flag if exists, so that dialog not steal the focus on creation.
		*pStyle &= ~WS_VISIBLE;
		fWS_VISIBLE = true;
	}

	auto hwndResDlg = CreateDialogIndirectParamW(nullptr, reinterpret_cast<LPCDLGTEMPLATEW>(stResData.pData->data()), m_hWnd, nullptr, 0);

	if (!hwndResDlg)
	{
		//Trying to set dialog's Items count to 0, to avoid failing on CreateWindowEx 
		//within CreateDialogIndirectParamW, for Dialog items (controls) with CustomClassName.
		//It can be overcome though, by creating a WindowClass with that custom name, 
		//but it's a lot of overhead for every such control: Fetch CustomClassName within ParceDlgTemplate
		//then register it with AfxRegisterClass.
		//Instead, just showing empty dialog without controls.
		PWORD pWordDlgItems;
		WORD wOld;
		if (fDlgEx) { //DLGTEMPLATEEX
			pWordDlgItems = &(const_cast<DLGTEMPLATEEX*>(reinterpret_cast<const DLGTEMPLATEEX*>(stResData.pData->data())))->cDlgItems;
			wOld = *pWordDlgItems;
		}
		else { //DLGTEMPLATE

			pWordDlgItems = &(const_cast<DLGTEMPLATE*>(reinterpret_cast<const DLGTEMPLATE*>(stResData.pData->data())))->cdit;
			wOld = *pWordDlgItems;
		}
		*pWordDlgItems = 0;
		hwndResDlg = CreateDialogIndirectParamW(nullptr, reinterpret_cast<LPCDLGTEMPLATEW>(stResData.pData->data()), m_hWnd, nullptr, 0);
		*pWordDlgItems = wOld;
	}

	if (fWS_VISIBLE) //Revert style back.
		*pStyle |= WS_VISIBLE;
	if (!hwndResDlg)
		return ResLoadError();

	CRect rcResDlg;
	::GetWindowRect(hwndResDlg, &rcResDlg);
	int iPosX = 0, iPosY = 0;
	UINT uFlags = SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER;
	constexpr DWORD dwStyles { WS_POPUP | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX };
	constexpr DWORD dwExStyles { WS_EX_APPWINDOW };
	if (!m_wndSampleDlg.m_hWnd)
	{
		if (!m_wndSampleDlg.CreateEx(dwExStyles, AfxRegisterWndClass(0),
			L"Sample Dialog...", dwStyles, 0, 0, 0, 0, m_hWnd, nullptr)) {
			MessageBoxW(L"Sample Dialog window CreateEx failed.", L"Error");
			return;
		}
		iPosX = (GetSystemMetrics(SM_CXSCREEN) / 2) - rcResDlg.Width() / 2;
		iPosY = (GetSystemMetrics(SM_CYSCREEN) / 2) - rcResDlg.Height() / 2;
		uFlags &= ~SWP_NOMOVE;
	}

	const auto hDC = ::GetDC(m_wndSampleDlg.m_hWnd);
	const auto hDCMemory = CreateCompatibleDC(hDC);
	const auto hBitmap = CreateCompatibleBitmap(hDC, rcResDlg.Width(), rcResDlg.Height());
	::SelectObject(hDCMemory, hBitmap);

	//To avoid window pop-up removing Windows animation temporarily, then restore back.
	ANIMATIONINFO aninfo { .cbSize { sizeof(ANIMATIONINFO) } };
	SystemParametersInfoW(SPI_GETANIMATION, aninfo.cbSize, &aninfo, 0);
	const auto iMinAnimate = aninfo.iMinAnimate;
	if (iMinAnimate) {
		aninfo.iMinAnimate = 0;
		SystemParametersInfoW(SPI_SETANIMATION, aninfo.cbSize, &aninfo, SPIF_SENDCHANGE);
	}
	const auto lLong = GetWindowLongPtrW(hwndResDlg, GWL_EXSTYLE);
	SetWindowLongPtrW(hwndResDlg, GWL_EXSTYLE, lLong | WS_EX_LAYERED);
	::SetLayeredWindowAttributes(hwndResDlg, 0, 1, LWA_ALPHA);

	::ShowWindow(hwndResDlg, SW_SHOWNOACTIVATE);
	::PrintWindow(hwndResDlg, hDCMemory, 0);
	::DestroyWindow(hwndResDlg);

	if (iMinAnimate) {
		aninfo.iMinAnimate = iMinAnimate;
		SystemParametersInfoW(SPI_SETANIMATION, aninfo.cbSize, &aninfo, SPIF_SENDCHANGE);
	}

	DeleteDC(hDCMemory);
	::ReleaseDC(m_wndSampleDlg.m_hWnd, hDC);

	CBitmap bmp;
	if (!bmp.Attach(hBitmap))
		return ResLoadError();

	m_stImgRes.Create(rcResDlg.Width(), rcResDlg.Height(), ILC_COLORDDB, 0, 1);
	if (m_stImgRes.Add(&bmp, nullptr) == -1)
		return ResLoadError();

	bmp.DeleteObject();

	AdjustWindowRectEx(rcResDlg, dwStyles, FALSE, dwExStyles); //Get window size with desirable client rect.
	m_wndSampleDlg.CreatedForMenu(false);
	m_wndSampleDlg.SetMenu(nullptr); //Removing any menu that could be set previously in RT_MENU handler.
	m_wndSampleDlg.SetWindowPos(this, iPosX, iPosY, rcResDlg.Width(), rcResDlg.Height(), uFlags);
	m_wndSampleDlg.SetDlgVisible(true);
	m_wndSampleDlg.RedrawWindow();  //Draw dialog bitmap.

	m_EditBRB.SetWindowTextW(optData->data()); //Set Dialog resources info to Editbox.
	CRect rcClient;
	GetClientRect(&rcClient);
	m_EditBRB.SetWindowPos(this, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	m_hwndActive = m_EditBRB.m_hWnd;
}

void CViewRightBR::CreateListTLSCallbacks()
{
	const auto pTLS = m_pLibpe->GetTLS();
	if (pTLS == nullptr)
		return;

	m_stlcs.dwStyle = 0;
	m_stlcs.uID = IDC_LIST_TLS_CALLBACKS;
	m_stListTLSCallbacks->Create(m_stlcs);
	m_stListTLSCallbacks->InsertColumn(0, L"TLS Callbacks", LVCFMT_CENTER | LVCFMT_FIXED_WIDTH, 300);
	LVCOLUMNW stCol { LVCF_FMT, LVCFMT_CENTER };
	m_stListTLSCallbacks->SetColumn(0, &stCol);

	int listindex { };
	for (const auto& iterCallbacks : pTLS->vecTLSCallbacks) {
		m_stListTLSCallbacks->InsertItem(listindex, std::format(L"{:08X}", iterCallbacks).data());
		++listindex;
	}
}

void CViewRightBR::CreateDebugEntry(DWORD dwEntry)
{
	const auto pDebug = m_pLibpe->GetDebug();
	if (pDebug == nullptr)
		return;

	//At the moment only IMAGE_DEBUG_TYPE_CODEVIEW info is supported.
	if (pDebug->at(dwEntry).stDebugDir.Type != IMAGE_DEBUG_TYPE_CODEVIEW)
		return;

	std::wstring wstrEdit;
	const auto& refDebug = pDebug->at(dwEntry);
	if (refDebug.stDebugHdrInfo.dwHdr[0] == 0x53445352) { //"RSDS"
		wstrEdit = L"Signature: RSDS\r\nGUID: ";
		const auto guid = *(reinterpret_cast<const GUID*>(&refDebug.stDebugHdrInfo.dwHdr[1]));
		LPWSTR lpwstr;
		StringFromIID(guid, &lpwstr);
		wstrEdit += lpwstr;
		wstrEdit += std::format(L"\r\nCounter/Age: {}\r\n", refDebug.stDebugHdrInfo.dwHdr[5]);
	}
	else if (refDebug.stDebugHdrInfo.dwHdr[0] == 0x3031424E) { //"NB10"
		wstrEdit += std::format(L"Signature: NB10\r\nOffset: {}\r\nTime/Signature: {}\r\nCounter/Age: {}\r\n",
			refDebug.stDebugHdrInfo.dwHdr[1], refDebug.stDebugHdrInfo.dwHdr[2], refDebug.stDebugHdrInfo.dwHdr[3]);
	}

	if (!refDebug.stDebugHdrInfo.strPDBName.empty()) {
		wstrEdit += L"PDB File: " + StrToWstr(refDebug.stDebugHdrInfo.strPDBName);
	}
	m_EditBRB.SetWindowTextW(wstrEdit.data());

	CRect rc;
	GetClientRect(&rc);
	m_EditBRB.SetWindowPos(this, rc.left, rc.top, rc.right, rc.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	m_hwndActive = m_EditBRB.m_hWnd;
}

void CViewRightBR::CreateStrings(const SRESDATA& stResData)
{
	auto pwszResString = reinterpret_cast<LPCWSTR>(stResData.pData->data());
	std::wstring wstrEdit;
	for (int i = 0; i < 16; ++i)
	{
		std::wstring wstrTmp;
		wstrEdit += wstrTmp.assign(pwszResString + 1, static_cast<UINT>(*pwszResString));
		if (i != 15)
			wstrEdit += L"\r\n";
		pwszResString += 1 + static_cast<UINT>(*pwszResString);
	}

	m_EditBRB.SetWindowTextW(wstrEdit.data());
	CRect rcClient;
	GetClientRect(&rcClient);
	m_EditBRB.SetWindowPos(this, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	m_hwndActive = m_EditBRB.m_hWnd;
}

void CViewRightBR::CreateAccel(const SRESDATA& stResData)
{
	if (stResData.pData == nullptr || stResData.pData->empty())
		return;

#pragma pack(push, 1) //No padding.
	struct ACCEL_MEM {
		BYTE bVirt;
		BYTE bReserved;
		WORD wKey;
		WORD wCmd;
		WORD wReserved;
	};
#pragma pack(pop)

	if (stResData.pData->size() % sizeof(ACCEL_MEM) != 0) { //Wrong data padding.
		return ResLoadError();
	}

	static const std::unordered_map<DWORD, std::wstring_view> mapAccVirt {
		{ FVIRTKEY, L"VIRTKEY" },
		{ FNOINVERT, L"NOINVERT" },
		{ FSHIFT, L"Shift" },
		{ FCONTROL, L"Control" },
		{ FALT, L"Alt" }
	};
	static const std::unordered_map<DWORD, std::wstring_view> mapAccKey {
		{ VK_LBUTTON, L"VK_LBUTTON" },
		{ VK_RBUTTON, L"VK_RBUTTON" },
		{ VK_CANCEL, L"VK_CANCEL" },
		{ VK_MBUTTON, L"VK_MBUTTON" },
		{ VK_XBUTTON1, L"VK_XBUTTON1" },
		{ VK_XBUTTON2, L"VK_XBUTTON2" },
		{ VK_BACK, L"VK_BACK" },
		{ VK_TAB, L"VK_TAB" },
		{ VK_CLEAR, L"VK_CLEAR" },
		{ VK_RETURN, L"VK_RETURN" },
		{ VK_SHIFT, L"VK_SHIFT" },
		{ VK_CONTROL, L"VK_CONTROL" },
		{ VK_MENU, L"VK_MENU" },
		{ VK_PAUSE, L"VK_PAUSE" },
		{ VK_CAPITAL, L"VK_CAPITAL" },
		{ VK_KANA, L"VK_KANA" },
		{ VK_IME_ON, L"VK_IME_ON" },
		{ VK_JUNJA, L"VK_JUNJA" },
		{ VK_FINAL, L"VK_FINAL" },
		{ VK_KANJI, L"VK_KANJI" },
		{ VK_IME_OFF, L"VK_IME_OFF" },
		{ VK_ESCAPE, L"VK_ESCAPE" },
		{ VK_CONVERT, L"VK_CONVERT" },
		{ VK_NONCONVERT, L"VK_NONCONVERT" },
		{ VK_ACCEPT, L"VK_ACCEPT" },
		{ VK_MODECHANGE, L"VK_MODECHANGE" },
		{ VK_SPACE, L"VK_SPACE" },
		{ VK_PRIOR, L"VK_PRIOR" },
		{ VK_NEXT, L"VK_NEXT" },
		{ VK_END, L"VK_END" },
		{ VK_HOME, L"VK_HOME" },
		{ VK_LEFT, L"VK_LEFT" },
		{ VK_UP, L"VK_UP" },
		{ VK_RIGHT, L"VK_RIGHT" },
		{ VK_DOWN, L"VK_DOWN" },
		{ VK_SELECT, L"VK_SELECT" },
		{ VK_PRINT, L"VK_PRINT" },
		{ VK_EXECUTE, L"VK_EXECUTE" },
		{ VK_SNAPSHOT, L"VK_SNAPSHOT" },
		{ VK_INSERT, L"VK_INSERT" },
		{ VK_DELETE, L"VK_DELETE" },
		{ VK_HELP, L"VK_HELP" },
		{ VK_LWIN, L"VK_LWIN" },
		{ VK_RWIN, L"VK_RWIN" },
		{ VK_APPS, L"VK_APPS" },
		{ VK_SLEEP, L"VK_SLEEP" },
		{ VK_NUMPAD0, L"VK_NUMPAD0" },
		{ VK_NUMPAD1, L"VK_NUMPAD1" },
		{ VK_NUMPAD2, L"VK_NUMPAD2" },
		{ VK_NUMPAD3, L"VK_NUMPAD3" },
		{ VK_NUMPAD4, L"VK_NUMPAD4" },
		{ VK_NUMPAD5, L"VK_NUMPAD5" },
		{ VK_NUMPAD6, L"VK_NUMPAD6" },
		{ VK_NUMPAD7, L"VK_NUMPAD7" },
		{ VK_NUMPAD8, L"VK_NUMPAD8" },
		{ VK_NUMPAD9, L"VK_NUMPAD9" },
		{ VK_MULTIPLY, L"VK_MULTIPLY" },
		{ VK_ADD, L"VK_ADD" },
		{ VK_SEPARATOR, L"VK_SEPARATOR" },
		{ VK_SUBTRACT, L"VK_SUBTRACT" },
		{ VK_DECIMAL, L"VK_DECIMAL" },
		{ VK_DIVIDE, L"VK_DIVIDE" },
		{ VK_F1, L"VK_F1" },
		{ VK_F2, L"VK_F2" },
		{ VK_F3, L"VK_F3" },
		{ VK_F4, L"VK_F4" },
		{ VK_F5, L"VK_F5" },
		{ VK_F6, L"VK_F6" },
		{ VK_F7, L"VK_F7" },
		{ VK_F8, L"VK_F8" },
		{ VK_F9, L"VK_F9" },
		{ VK_F10, L"VK_F10" },
		{ VK_F11, L"VK_F11" },
		{ VK_F12, L"VK_F12" },
		{ VK_F13, L"VK_F13" },
		{ VK_F14, L"VK_F14" },
		{ VK_F15, L"VK_F15" },
		{ VK_F16, L"VK_F16" },
		{ VK_F17, L"VK_F17" },
		{ VK_F18, L"VK_F18" },
		{ VK_F19, L"VK_F19" },
		{ VK_F20, L"VK_F20" },
		{ VK_F21, L"VK_F21" },
		{ VK_F22, L"VK_F22" },
		{ VK_F23, L"VK_F23" },
		{ VK_F24, L"VK_F24" },
		{ VK_NUMLOCK, L"VK_NUMLOCK" },
		{ VK_SCROLL, L"VK_SCROLL" },
		{ VK_LSHIFT, L"VK_LSHIFT" },
		{ VK_RSHIFT, L"VK_RSHIFT" },
		{ VK_LCONTROL, L"VK_LCONTROL" },
		{ VK_RCONTROL, L"VK_RCONTROL" },
		{ VK_LMENU, L"VK_LMENU" },
		{ VK_RMENU, L"VK_RMENU" },
		{ VK_BROWSER_BACK, L"VK_BROWSER_BACK" },
		{ VK_BROWSER_FORWARD, L"VK_BROWSER_FORWARD" },
		{ VK_BROWSER_REFRESH, L"VK_BROWSER_REFRESH" },
		{ VK_BROWSER_STOP, L"VK_BROWSER_STOP" },
		{ VK_BROWSER_SEARCH, L"VK_BROWSER_SEARCH" },
		{ VK_BROWSER_FAVORITES, L"VK_BROWSER_FAVORITES" },
		{ VK_BROWSER_HOME, L"VK_BROWSER_HOME" },
		{ VK_VOLUME_MUTE, L"VK_VOLUME_MUTE" },
		{ VK_VOLUME_DOWN, L"VK_VOLUME_DOWN" },
		{ VK_VOLUME_UP, L"VK_VOLUME_UP" },
		{ VK_MEDIA_NEXT_TRACK, L"VK_MEDIA_NEXT_TRACK" },
		{ VK_MEDIA_PREV_TRACK, L"VK_MEDIA_PREV_TRACK" },
		{ VK_MEDIA_STOP, L"VK_MEDIA_STOP" },
		{ VK_MEDIA_PLAY_PAUSE, L"VK_MEDIA_PLAY_PAUSE" },
		{ VK_LAUNCH_MAIL, L"VK_LAUNCH_MAIL" },
		{ VK_LAUNCH_MEDIA_SELECT, L"VK_LAUNCH_MEDIA_SELECT" },
		{ VK_LAUNCH_APP1, L"VK_LAUNCH_APP1" },
		{ VK_LAUNCH_APP2, L"VK_LAUNCH_APP2" },
		{ VK_OEM_1, L"VK_OEM_1" },
		{ VK_OEM_PLUS, L"VK_OEM_PLUS" },
		{ VK_OEM_COMMA, L"VK_OEM_COMMA" },
		{ VK_OEM_MINUS, L"VK_OEM_MINUS" },
		{ VK_OEM_PERIOD, L"VK_OEM_PERIOD" },
		{ VK_OEM_2, L"VK_OEM_2" },
		{ VK_OEM_3, L"VK_OEM_3" },
		{ VK_OEM_4, L"VK_OEM_4" },
		{ VK_OEM_5, L"VK_OEM_5" },
		{ VK_OEM_6, L"VK_OEM_6" },
		{ VK_OEM_7, L"VK_OEM_7" },
		{ VK_OEM_8, L"VK_OEM_8" },
		{ VK_OEM_102, L"VK_OEM_102" },
		{ VK_PROCESSKEY, L"VK_PROCESSKEY" },
		{ VK_PACKET, L"VK_PACKET" },
		{ VK_ATTN, L"VK_ATTN" },
		{ VK_CRSEL, L"VK_CRSEL" },
		{ VK_EXSEL, L"VK_EXSEL" },
		{ VK_EREOF, L"VK_EREOF" },
		{ VK_PLAY, L"VK_PLAY" },
		{ VK_ZOOM, L"VK_ZOOM" },
		{ VK_NONAME, L"VK_NONAME" },
		{ VK_PA1, L"VK_PA1" },
		{ VK_OEM_CLEAR, L"VK_OEM_CLEAR" }
	};

	std::wstring wstrEdit;
	const auto iAccels = stResData.pData->size() / sizeof(ACCEL_MEM); //How many accel entrys.
	for (auto iter { 0 }; iter < iAccels; ++iter) {
		const auto& refAccel = reinterpret_cast<const ACCEL_MEM&>(stResData.pData->data()[iter * sizeof(ACCEL_MEM)]);

		wstrEdit += L"Behavior: ";
		bool fHit { false }; //First occurense in map.Find().
		for (const auto& it : mapAccVirt) { //Accel bVirt.
			if (refAccel.bVirt & it.first) {
				if (fHit) {
					wstrEdit += L"+";
				}
				wstrEdit += it.second;
				fHit = true;
			}
		}

		wstrEdit += L"; Key: ";
		if (const auto it = mapAccKey.find(refAccel.wKey); it != mapAccKey.end()) { //Accel wKey.
			wstrEdit += it->second;
		}
		else {
			wstrEdit += std::format(L"{}", static_cast<char>(refAccel.wKey));
		}

		wstrEdit += std::format(L"; Identifier: {};\r\n", refAccel.wCmd); //Accel wCmd.
	}

	m_EditBRB.SetWindowTextW(wstrEdit.data());
	CRect rcClient;
	GetClientRect(&rcClient);
	m_EditBRB.SetWindowPos(this, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	m_hwndActive = m_EditBRB.m_hWnd;
}

void CViewRightBR::CreateGroupIconCursor(const SRESDATA& stResData)
{
	const auto pstResRoot = m_pLibpe->GetResources();
	if (pstResRoot == nullptr)
		return;

#pragma pack(push, 2)
	struct GRPICONDIRENTRY
	{
		BYTE  bWidth;       // Width, in pixels, of the image
		BYTE  bHeight;      // Height, in pixels, of the image
		BYTE  bColorCount;  // Number of colors in image (0 if >=8bpp)
		BYTE  bReserved;    // Reserved
		WORD  wPlanes;      // Color Planes
		WORD  wBitCount;    // Bits per pixel
		DWORD dwBytesInRes; // how many bytes in this resource?
		WORD  nID;          // the ID
	};
	struct GRPICONDIR
	{
		WORD			idReserved;   // Reserved (must be 0)
		WORD			idType;       // Resource type (1 for icons)
		WORD			idCount;      // How many images?
		GRPICONDIRENTRY idEntries[1]; // The entries for each image
	};
	using LPGRPICONDIR = const GRPICONDIR*;
#pragma pack(pop)

	const auto pGRPIDir = reinterpret_cast<LPGRPICONDIR>(stResData.pData->data());

	for (int i = 0; i < pGRPIDir->idCount; ++i)
	{
		const auto& rootvec = pstResRoot->vecResData;
		for (const auto& iterRoot : rootvec)
		{
			if (iterRoot.stResDirEntry.Id == 3 || //RT_ICON
				iterRoot.stResDirEntry.Id == 1)   //RT_CURSOR
			{
				const auto& lvl2tup = iterRoot.stResLvL2;
				const auto& lvl2vec = lvl2tup.vecResData;
				for (const auto& iterlvl2 : lvl2vec)
				{
					if (iterlvl2.stResDirEntry.Id == pGRPIDir->idEntries[i].nID)
					{
						const auto& lvl3tup = iterlvl2.stResLvL3;
						const auto& lvl3vec = lvl3tup.vecResData;
						if (!lvl3vec.empty())
						{
							const auto& data = lvl3vec.at(0).vecRawResData;
							if (!data.empty())
							{
								const auto hIcon = CreateIconFromResourceEx(const_cast<PBYTE>(reinterpret_cast<const BYTE*>(data.data())),
									static_cast<DWORD>(data.size()),
									(iterRoot.stResDirEntry.Id == 3) ? TRUE : FALSE, 0x00030000, 0, 0, LR_DEFAULTCOLOR);
								if (!hIcon)
									return ResLoadError();

								ICONINFO iconInfo;
								if (!GetIconInfo(hIcon, &iconInfo))
									return ResLoadError();
								if (!GetObjectW(iconInfo.hbmMask, sizeof(BITMAP), &m_stBmp))
									return ResLoadError();

								DeleteObject(iconInfo.hbmColor);
								DeleteObject(iconInfo.hbmMask);

								m_vecImgRes.emplace_back(std::make_unique<CImageList>());
								const auto& vecBack = m_vecImgRes.back();
								vecBack->Create(m_stBmp.bmWidth,
									(iterRoot.stResDirEntry.Id == 3) ? m_stBmp.bmHeight : m_stBmp.bmWidth, ILC_COLORDDB, 0, 1);
								vecBack->SetBkColor(m_clrBkImgList);
								m_iImgResWidth += m_stBmp.bmWidth;
								m_iImgResHeight = max(m_stBmp.bmHeight, m_iImgResHeight);

								if (vecBack->Add(hIcon) == -1)
									return ResLoadError();

								DestroyIcon(hIcon);
								break;
							}
						}
					}
				}
			}
		}
	}
	SetScrollSizes(MM_TEXT, CSize(m_iImgResWidth, m_iImgResHeight));

	m_iResTypeToDraw = stResData.wIdType;
	m_fDrawRes = true;
}

void CViewRightBR::CreateVersion(const SRESDATA& stResData)
{
#pragma pack(push, 4)
	struct LANGANDCODEPAGE
	{
		WORD wLanguage;
		WORD wCodePage;
	};
#pragma pack(pop)

	static const std::unordered_map<int, std::wstring_view> mapVerInfoStrings {
		{ 0, L"FileDescription" },
		{ 1, L"FileVersion" },
		{ 2, L"InternalName" },
		{ 3, L"CompanyName" },
		{ 4, L"LegalCopyright" },
		{ 5, L"OriginalFilename" },
		{ 6, L"ProductName" },
		{ 7, L"ProductVersion" }
	};

	LANGANDCODEPAGE* pLangAndCP { };
	UINT dwBytesOut;

	//Read the list of languages and code pages.
	VerQueryValueW(stResData.pData->data(), L"\\VarFileInfo\\Translation", reinterpret_cast<LPVOID*>(&pLangAndCP), &dwBytesOut);

	std::wstring wstrEdit;
	DWORD dwLangCount = dwBytesOut / sizeof(LANGANDCODEPAGE);
	for (size_t iterCodePage = 0; iterCodePage < dwLangCount; ++iterCodePage) //Read the file description for each language and code page.
	{
		for (unsigned iterMap = 0; iterMap < mapVerInfoStrings.size(); ++iterMap) //sizeof pstrVerInfoStrings [];
		{
			wstrEdit += mapVerInfoStrings.at(iterMap);
			wstrEdit += L" - ";
			if (wchar_t* pszBufferOut { }; VerQueryValueW(stResData.pData->data(), std::format(L"\\StringFileInfo\\{:04x}{:04x}\\{}",
				pLangAndCP[iterCodePage].wLanguage, pLangAndCP[iterCodePage].wCodePage, mapVerInfoStrings.at(iterMap)).data(),
				reinterpret_cast<LPVOID*>(&pszBufferOut), &dwBytesOut))
			{
				if (dwBytesOut > 0)
					wstrEdit += pszBufferOut;
			}
			wstrEdit += L"\r\n";
		}
	}
	m_EditBRB.SetWindowTextW(wstrEdit.data());
	CRect rcClient;
	GetClientRect(&rcClient);
	m_EditBRB.SetWindowPos(this, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	m_hwndActive = m_EditBRB.m_hWnd;
}

void CViewRightBR::CreateManifest(const SRESDATA& stResData)
{
	m_EditBRB.SetWindowTextW(StrToWstr({ reinterpret_cast<const char*>(stResData.pData->data()), stResData.pData->size() }).data());
	CRect rcClient;
	GetClientRect(&rcClient);
	m_EditBRB.SetWindowPos(this, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
	m_hwndActive = m_EditBRB.m_hWnd;
}

void CViewRightBR::CreateToolbar(const SRESDATA& stResData)
{
	const auto pstResRoot = m_pLibpe->GetResources();
	if (pstResRoot == nullptr)
		return;

	const auto& rootvec = pstResRoot->vecResData;
	for (const auto& iterRoot : rootvec)
	{
		if (iterRoot.stResDirEntry.Id == 2) //RT_BITMAP
		{
			const auto& lvl2tup = iterRoot.stResLvL2;
			const auto& lvl2vec = lvl2tup.vecResData;
			for (const auto& iterlvl2 : lvl2vec)
			{
				if (iterlvl2.stResDirEntry.Id == stResData.wIdName)
				{
					const auto& lvl3tup = iterlvl2.stResLvL3;
					const auto& lvl3vec = lvl3tup.vecResData;
					if (!lvl3vec.empty())
					{
						const auto pData = &lvl3vec.at(0).vecRawResData;
						if (!pData->empty()) {
							SRESDATA stData { .wIdType { 2 }, .wIdName { stResData.wIdName }, .pData { pData } };
							CreateBitmap(stData);
						}
					}
				}
			}
		}
	}
}

void CViewRightBR::ShowResource(const SRESDATA* pResData)
{
	m_stImgRes.DeleteImageList();
	m_iResTypeToDraw = -1;
	m_iImgResWidth = 0;
	m_iImgResHeight = 0;
	m_vecImgRes.clear();

	if (pResData != nullptr)
	{
		//Destroy Dialog Sample window if it's any other resource but RT_MENU or RT_DIALOG.
		if (pResData->wIdType != 4 && pResData->wIdType != 5 && m_wndSampleDlg.m_hWnd) {
			m_wndSampleDlg.SetDlgVisible(false);
			m_wndSampleDlg.DestroyWindow();
		}

		if (pResData->pData->empty()) {
			return ResLoadError();
		}

		switch (pResData->wIdType)
		{
		case 1: //RT_CURSOR
		case 3: //RT_ICON
			CreateIconCursor(*pResData);
			break;
		case 2: //RT_BITMAP
			CreateBitmap(*pResData);
			break;
		case 4: //RT_MENU
			CreateMenu(*pResData);
			break;
		case 5: //RT_DIALOG
			CreateDlg(*pResData);
			break;
		case 6: //RT_STRING
			CreateStrings(*pResData);
			break;
		case 9: //RT_ACCELERATOR
			CreateAccel(*pResData);
			break;
		case 12: //RT_GROUP_CURSOR
		case 14: //RT_GROUP_ICON
			CreateGroupIconCursor(*pResData);
			break;
		case 16: //RT_VERSION
			CreateVersion(*pResData);
			break;
		case 24: //RT_MANIFEST
			CreateManifest(*pResData);
			break;
		case 241: //RT_TOOLBAR
			CreateToolbar(*pResData);
			break;
		default:
			m_iResTypeToDraw = pResData->wIdType;
			m_fDrawRes = true;
		}
	}
	else
	{
		//Destroy Dialog Sample window if it's just Resource window Update.
		if (m_wndSampleDlg.m_hWnd) {
			m_wndSampleDlg.SetDlgVisible(false);
			m_wndSampleDlg.DestroyWindow();
		}
	}

	RedrawWindow();
}

void CViewRightBR::ResLoadError()
{
	m_iResTypeToDraw = 0xFF;
	m_fDrawRes = true;
	RedrawWindow();
}

auto CViewRightBR::ParceDlgTemplate(std::span<std::byte> spnData)->std::optional<std::wstring>
{
#pragma pack(push, 4) //When pData comes from PE resources it is packed.

	//Helper struct as described in https://docs.microsoft.com/en-us/windows/win32/dlgbox/dlgtemplateex
	//It can not be completely defined because of its variadic nature.
	struct DLGTEMPLATEEX {
		WORD  dlgVer;
		WORD  signature;
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		WORD  cDlgItems;
		short x;
		short y;
		short cx;
		short cy;
		WORD  menu;
	};

	//Helper struct as described in https://docs.microsoft.com/en-us/windows/win32/dlgbox/dlgitemtemplateex
	//It can not be completely defined because of its variadic nature.
	struct DLGITEMTEMPLATEEX {
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		short x;
		short y;
		short cx;
		short cy;
		DWORD id;
	};
#pragma pack(pop)

	const auto pDataDlg = spnData.data();
	const auto nSizeDataDlg = spnData.size();
	auto pDataDlgHdr = pDataDlg;
	const bool fDlgEx { *((reinterpret_cast<PWORD>(pDataDlgHdr)) + 1) == 0xFFFF }; //DLGTEMPLATEEX ?.
	DWORD dwDlgStyles;
	DWORD dwDlgStylesEx;
	WORD wCountDlgItems;
	WCHAR* pwszDlgMenu { };
	size_t nSizeDlgMenu { };
	WORD wDlgMenuOrdinal { };
	std::wstring wstrRet; //String to return.

	if (fDlgEx) //DLGTEMPLATEEX.
	{
		const auto pDlgEx = reinterpret_cast<DLGTEMPLATEEX*>(pDataDlgHdr);
		wstrRet = std::format(L"DIALOGEX {}, {}, {}, {}\r\n", pDlgEx->x, pDlgEx->y, pDlgEx->cx, pDlgEx->cy);
		dwDlgStyles = pDlgEx->style;
		dwDlgStylesEx = pDlgEx->exStyle;
		wCountDlgItems = pDlgEx->cDlgItems;

		//Menu.
		if (pDlgEx->menu == 0) //No menu.
			pDataDlgHdr += sizeof(DLGTEMPLATEEX);
		else if (pDlgEx->menu == 0xFFFF) //Menu ordinal.
		{
			wDlgMenuOrdinal = *reinterpret_cast<PWORD>(pDataDlgHdr + sizeof(WORD));
			pDataDlgHdr += sizeof(WORD) * 2; //Ordinal's WORD follows ((DLGTEMPLATEEX*)pDataDlgHdr)->menu.
		}
		else //Menu wstring.
		{
			pDataDlgHdr += sizeof(DLGTEMPLATEEX);
			pwszDlgMenu = reinterpret_cast<WCHAR*>(pDataDlgHdr);
			if (StringCbLengthW(pwszDlgMenu, nSizeDataDlg - (reinterpret_cast<DWORD_PTR>(pDataDlgHdr) -
				reinterpret_cast<DWORD_PTR>(pDataDlg)), &nSizeDlgMenu) != S_OK)
				return std::nullopt;

			pDataDlgHdr += nSizeDlgMenu + sizeof(WCHAR); //Plus null terminating.
		}
	}
	else //DLGTEMPLATE.
	{
		const auto pDlg = reinterpret_cast<DLGTEMPLATE*>(pDataDlgHdr);
		wstrRet = std::format(L"DIALOG {}, {}, {}, {}\r\n", pDlg->x, pDlg->y, pDlg->cx, pDlg->cy);
		dwDlgStyles = pDlg->style;
		dwDlgStylesEx = pDlg->dwExtendedStyle;
		wCountDlgItems = pDlg->cdit;
		pDataDlgHdr += sizeof(DLGTEMPLATE);

		//Menu.
		if (*reinterpret_cast<PWORD>(pDataDlgHdr) == 0) //No menu.
			pDataDlgHdr += sizeof(WORD);
		else if (*reinterpret_cast<PWORD>(pDataDlgHdr) == 0xFFFF) //Menu ordinal.
		{
			wDlgMenuOrdinal = *reinterpret_cast<PWORD>(pDataDlgHdr + sizeof(WORD));
			pDataDlgHdr += sizeof(WORD) * 2; //Ordinal's WORD follows menu WORD.
		}
		else //Menu wstring.
		{
			pwszDlgMenu = reinterpret_cast<WCHAR*>(&pDataDlgHdr);
			if (StringCbLengthW(pwszDlgMenu, nSizeDataDlg - (reinterpret_cast<DWORD_PTR>(pDataDlgHdr) -
				reinterpret_cast<DWORD_PTR>(pDataDlg)), &nSizeDlgMenu) != S_OK)
				return std::nullopt;

			pDataDlgHdr += nSizeDlgMenu + sizeof(WCHAR); //Plus null terminating.
		}
	}

	//Dialog styles.
	std::wstring wstrStyles;
	const std::unordered_map<DWORD, std::wstring> mapDlgStyles { //All Window and Dialog styles except default (0).
		{ DS_3DLOOK, L"DS_3DLOOK" },
		{ DS_ABSALIGN, L"DS_ABSALIGN" },
		{ DS_CENTER, L"DS_CENTER" },
		{ DS_CENTERMOUSE, L"DS_CENTERMOUSE" },
		{ DS_CONTEXTHELP, L"DS_CONTEXTHELP" },
		{ DS_CONTROL, L"DS_CONTROL" },
		{ DS_FIXEDSYS, L"DS_FIXEDSYS" },
		{ DS_LOCALEDIT, L"DS_LOCALEDIT" },
		{ DS_MODALFRAME, L"DS_MODALFRAME" },
		{ DS_NOFAILCREATE, L"DS_NOFAILCREATE" },
		{ DS_NOIDLEMSG, L"DS_NOIDLEMSG" },
		{ DS_SETFONT, L"DS_SETFONT" },
		{ DS_SETFOREGROUND, L"DS_SETFOREGROUND" },
		{ DS_SHELLFONT, L"DS_SHELLFONT" },
		{ DS_SYSMODAL, L"DS_SYSMODAL" },
		{ WS_BORDER, L"WS_BORDER" },
		{ WS_CAPTION, L"WS_CAPTION" },
		{ WS_CHILD, L"WS_CHILD" },
		{ WS_CHILDWINDOW, L"WS_CHILDWINDOW" },
		{ WS_CLIPCHILDREN, L"WS_CLIPCHILDREN" },
		{ WS_CLIPSIBLINGS, L"WS_CLIPSIBLINGS" },
		{ WS_DISABLED, L"WS_DISABLED" },
		{ WS_DLGFRAME, L"WS_DLGFRAME" },
		{ WS_HSCROLL, L"WS_HSCROLL" },
		{ WS_MAXIMIZE, L"WS_MAXIMIZE" },
		{ WS_MAXIMIZEBOX, L"WS_MAXIMIZEBOX" },
		{ WS_MINIMIZE, L"WS_MINIMIZE" },
		{ WS_MINIMIZEBOX, L"WS_MINIMIZEBOX" },
		{ WS_POPUP, L"WS_POPUP" },
		{ WS_SYSMENU, L"WS_SYSMENU" },
		{ WS_THICKFRAME, L"WS_THICKFRAME" },
		{ WS_VISIBLE, L"WS_VISIBLE" },
		{ WS_VSCROLL, L"WS_VSCROLL" }
	};

	for (const auto& iter : mapDlgStyles)
	{
		if (iter.first & dwDlgStyles)
		{
			if (!wstrStyles.empty())
				wstrStyles += L" | " + iter.second;
			else
				wstrStyles += iter.second;
		}
	}
	wstrRet += L"DIALOG STYLES: " + wstrStyles + L"\r\n";
	wstrStyles.clear();

	if (dwDlgStylesEx) //ExStyle stringify.
	{
		const std::unordered_map<DWORD, std::wstring> mapDlgExStyles { //All Extended Window and Dialog styles except default (0).
			{ WS_EX_ACCEPTFILES, L"WS_EX_ACCEPTFILES" },
			{ WS_EX_APPWINDOW, L"WS_EX_APPWINDOW" },
			{ WS_EX_CLIENTEDGE, L"WS_EX_CLIENTEDGE" },
			{ WS_EX_COMPOSITED, L"WS_EX_COMPOSITED" },
			{ WS_EX_CONTEXTHELP, L"WS_EX_CONTEXTHELP" },
			{ WS_EX_CONTROLPARENT, L"WS_EX_CONTROLPARENT" },
			{ WS_EX_DLGMODALFRAME, L"WS_EX_DLGMODALFRAME" },
			{ WS_EX_LAYERED, L"WS_EX_LAYERED" },
			{ WS_EX_LAYOUTRTL, L"WS_EX_LAYOUTRTL" },
			{ WS_EX_LEFTSCROLLBAR, L"WS_EX_LEFTSCROLLBAR" },
			{ WS_EX_MDICHILD, L"WS_EX_MDICHILD" },
			{ WS_EX_NOACTIVATE, L"WS_EX_NOACTIVATE" },
			{ WS_EX_NOINHERITLAYOUT, L"WS_EX_NOINHERITLAYOUT" },
			{ WS_EX_NOPARENTNOTIFY, L"WS_EX_NOPARENTNOTIFY" },
			{ WS_EX_RIGHT, L"WS_EX_RIGHT" },
			{ WS_EX_RTLREADING, L"WS_EX_RTLREADING" },
			{ WS_EX_STATICEDGE, L"WS_EX_STATICEDGE" },
			{ WS_EX_TOOLWINDOW, L"WS_EX_TOOLWINDOW" },
			{ WS_EX_TOPMOST, L"WS_EX_TOPMOST" },
			{ WS_EX_TRANSPARENT, L"WS_EX_TRANSPARENT" },
			{ WS_EX_WINDOWEDGE, L"WS_EX_WINDOWEDGE" }
		};

		for (const auto& iter : mapDlgExStyles)
		{
			if (iter.first & dwDlgStylesEx)
			{
				if (!wstrStyles.empty())
					wstrStyles += L" | " + iter.second;
				else
					wstrStyles += iter.second;
			}
		}
		wstrRet += L"DIALOG EXTENDED STYLES: " + wstrStyles + L"\r\n";
	}

	//Next go "Class Name" and "Title" that common for both templates.

	//Class name.
	if (*reinterpret_cast<PWORD>(pDataDlgHdr) == 0xFFFF) //Class name is ordinal.
	{
		const auto wClassOrdinal = *reinterpret_cast<PWORD>(pDataDlgHdr + sizeof(WORD));
		pDataDlgHdr += sizeof(WORD) * 2; //Class name WORD plus Ordinal WORD.
		wstrRet += std::format(L"DIALOG CLASS ORDINAL: 0x{:X}\r\n", wClassOrdinal);
	}
	else //Class Name is wstring.
	{
		if (*reinterpret_cast<PWORD>(pDataDlgHdr) != 0x0000) //If not NULL then there is a need to align.
		{
			pDataDlgHdr += (sizeof(WORD) - ((reinterpret_cast<DWORD_PTR>(pDataDlgHdr) -
				reinterpret_cast<DWORD_PTR>(pDataDlg)) & 1)) & 1; //WORD Aligning.
		}

		size_t lengthClassName { };
		const auto* pwstrClassName = reinterpret_cast<WCHAR*>(pDataDlgHdr);
		if (StringCbLengthW(pwstrClassName, nSizeDataDlg - (reinterpret_cast<DWORD_PTR>(pDataDlgHdr) -
			reinterpret_cast<DWORD_PTR>(pDataDlg)), &lengthClassName) != S_OK)
			return std::nullopt;

		pDataDlgHdr += lengthClassName + sizeof(WCHAR); //Plus null terminating.
		wstrRet += L"DIALOG CLASS NAME: \"" + std::wstring { pwstrClassName } + L"\"\r\n";
	}

	//Title (Caption).
	if (*reinterpret_cast<PWORD>(pDataDlgHdr) != 0x0000) //If not NULL then there is a need to align.
	{
		pDataDlgHdr += (sizeof(WORD) - ((reinterpret_cast<DWORD_PTR>(pDataDlgHdr) -
			reinterpret_cast<DWORD_PTR>(pDataDlg)) & 1)) & 1; //WORD Aligning.
	}

	size_t lengthTitle { };
	const auto* pwstrTitle = reinterpret_cast<WCHAR*>(pDataDlgHdr);
	if (StringCbLengthW(pwstrTitle, nSizeDataDlg - (reinterpret_cast<DWORD_PTR>(pDataDlgHdr) -
		reinterpret_cast<DWORD_PTR>(pDataDlg)), &lengthTitle) != S_OK)
		return std::nullopt;

	pDataDlgHdr += lengthTitle + sizeof(WCHAR); //Plus null terminating.
	wstrRet += L"DIALOG CAPTION: \"" + std::wstring { pwstrTitle } + L"\"\r\n";

	//Menu.
	if (pwszDlgMenu != nullptr && nSizeDlgMenu > 0) {
		wstrRet += L"DIALOG MENU RESOURCE NAME: \"" + std::wstring { pwszDlgMenu } + L"\"\r\n";
	}
	else if (wDlgMenuOrdinal > 0) {
		wstrRet += std::format(L"DIALOG MENU RESOURCE ORDINAL: 0x{:X}\r\n", wDlgMenuOrdinal);
	}

	//Font related stuff, has small differences between templates.
	wstrRet += L"DIALOG FONT: ";

	//DLGTEMPLATEEX font related, only if DS_SETFONT or DS_SHELLFONT styles present.
	if (fDlgEx && ((dwDlgStyles & DS_SETFONT) || (dwDlgStyles & DS_SHELLFONT)))
	{
		const auto wFontPointSize = *reinterpret_cast<PWORD>(pDataDlgHdr);
		pDataDlgHdr += sizeof(wFontPointSize);
		const auto wFontWeight = *reinterpret_cast<PWORD>(pDataDlgHdr);
		pDataDlgHdr += sizeof(wFontWeight);
		const auto bItalic = static_cast<BYTE>(*pDataDlgHdr);
		pDataDlgHdr += sizeof(bItalic);
		const auto bCharset = static_cast<BYTE>(*pDataDlgHdr);
		pDataDlgHdr += sizeof(bCharset);

		if (*reinterpret_cast<PWORD>(pDataDlgHdr) != 0x0000) //If not NULL then there is a need to align.
		{
			pDataDlgHdr += (sizeof(WORD) - ((reinterpret_cast<DWORD_PTR>(pDataDlgHdr) -
				reinterpret_cast<DWORD_PTR>(pDataDlg)) & 1)) & 1; //WORD Aligning.
		}

		size_t lengthTypeFace { };
		const auto pwstrTypeFace = reinterpret_cast<WCHAR*>(pDataDlgHdr);
		if (StringCbLengthW(pwstrTypeFace, nSizeDataDlg - (reinterpret_cast<DWORD_PTR>(pDataDlgHdr) -
			reinterpret_cast<DWORD_PTR>(pDataDlg)), &lengthTypeFace) != S_OK)
			return std::nullopt;

		pDataDlgHdr += lengthTypeFace + sizeof(WCHAR); //Plus null terminating.
		wstrRet += L"NAME: \"" + std::wstring { pwstrTypeFace } + L"\"";

		//These font params are only in DLGTEMPLATEEX.
		wstrRet += std::format(L", SIZE: {}, WEIGHT: {}, IS ITALIC: {}, CHARSET: {}\r\n", wFontPointSize, wFontWeight, bItalic, bCharset);
	}
	else if (!fDlgEx && (dwDlgStyles & DS_SETFONT)) //DLGTEMPLATE font related, only if DS_SETFONT style is present.
	{
		const auto wFontPointSize = *reinterpret_cast<PWORD>(pDataDlgHdr);
		pDataDlgHdr += sizeof(wFontPointSize);

		if (*reinterpret_cast<PWORD>(pDataDlgHdr) != 0x0000)
		{
			pDataDlgHdr += (sizeof(WORD) - ((reinterpret_cast<DWORD_PTR>(pDataDlgHdr) -
				reinterpret_cast<DWORD_PTR>(pDataDlg)) & 1)) & 1; //WORD Aligning.
		}

		size_t lengthTypeFace { };
		const auto pwstrTypeFace = reinterpret_cast<WCHAR*>(pDataDlgHdr);
		if (StringCbLengthW(pwstrTypeFace, nSizeDataDlg - (reinterpret_cast<DWORD_PTR>(pDataDlgHdr) -
			reinterpret_cast<DWORD_PTR>(pDataDlg)), &lengthTypeFace) != S_OK)
			return std::nullopt;

		pDataDlgHdr += lengthTypeFace + sizeof(WCHAR); //Plus null terminating.
		wstrRet += L"NAME \"" + std::wstring { pwstrTypeFace } + L"\"";
		wstrRet += std::format(L", SIZE: {}\r\n", wFontPointSize); //Only SIZE in DLGTEMPLATE.
	}

	wstrRet += std::format(L"DIALOG ITEMS: {}\r\n", wCountDlgItems);
	//DLGTEMPLATE(EX) end. ///////////////////////////////////////////

	//Now go DLGITEMTEMPLATE(EX) pData structures.
	auto pDataItems = pDataDlgHdr; //Just to differentiate.

	wstrRet += L"{\r\n";
	for (WORD items = 0; items < wCountDlgItems; ++items)
	{
		pDataItems += (sizeof(DWORD) - ((reinterpret_cast<DWORD_PTR>(pDataItems) -
			reinterpret_cast<DWORD_PTR>(pDataDlg)) & 3)) & 3; //DWORD Aligning.
		//Out of bounds checking.
		if (reinterpret_cast<DWORD_PTR>(pDataItems) >= reinterpret_cast<DWORD_PTR>(pDataDlg) + nSizeDataDlg)
			break;

		std::wstring wstrItemStyles; //Styles, ExStyles, ItemID and Coords.
		if (fDlgEx) //DLGITEMTEMPLATEEX.
		{
			const auto pItemEx = reinterpret_cast<DLGITEMTEMPLATEEX*>(pDataItems);
			//Styles, ExStyles, ItemID and Coords. ItemID is DWORD;
			wstrItemStyles = std::format(L"Styles: 0x{:08X}, ExStyles: 0x{:08X}, ItemID: 0x{:08X}, Coords: x={}, y={}, cx={}, cy={}",
				pItemEx->style, pItemEx->exStyle, pItemEx->id, pItemEx->x, pItemEx->y, pItemEx->cx, pItemEx->cy);
			pDataItems += sizeof(DLGITEMTEMPLATEEX);
		}
		else //DLGITEMTEMPLATE.
		{
			const auto pItem = reinterpret_cast<DLGITEMTEMPLATE*>(pDataItems);
			//Styles, ExStyles, ItemID and Coords. ItemID is WORD;
			wstrItemStyles = std::format(L"Styles: 0x{:08X}, ExStyles: 0x{:08X}, ItemID: 0x{:04X}, Coords: x={}, y={}, cx={}, cy={}",
				pItem->style, pItem->dwExtendedStyle, pItem->id, pItem->x, pItem->y, pItem->cx, pItem->cy);
			pDataItems += sizeof(DLGITEMTEMPLATE);
		}

		std::wstring wstrItem = L"    ";

		//Item Class name.
		if (*reinterpret_cast<PWORD>(pDataItems) == 0xFFFF) //Class is ordinal.
		{
			const auto wClassOrdinalItem = *reinterpret_cast<PWORD>(pDataItems + sizeof(WORD));
			pDataItems += sizeof(WORD) * 2;
			wstrItem += std::format(L"Class ordinal: 0x{:04X}", wClassOrdinalItem);

			const std::unordered_map<DWORD, std::wstring> mapItemClassOrd {
				{ 0x0080, L"Button" }, { 0x0081, L"Edit" }, { 0x0082, L"Static" },
				{ 0x0083, L"List box" }, { 0x0084, L"Scroll bar" }, { 0x0085, L"Combo box" },
			};
			if (const auto iter = mapItemClassOrd.find(wClassOrdinalItem); iter != mapItemClassOrd.end())
				wstrItem += L" (\"" + iter->second + L"\")";
			wstrItem += L", ";
		}
		else //Class Name is wstring.
		{
			pDataItems += (sizeof(WORD) - ((reinterpret_cast<DWORD_PTR>(pDataItems) -
				reinterpret_cast<DWORD_PTR>(pDataDlg)) & 1)) & 1; //WORD Aligning.
			size_t lengthClassNameItem { };
			const auto* pwstrClassNameItem = reinterpret_cast<WCHAR*>(pDataItems);
			if (StringCbLengthW(pwstrClassNameItem, nSizeDataDlg - (reinterpret_cast<DWORD_PTR>(pDataItems) -
				reinterpret_cast<DWORD_PTR>(pDataDlg)), &lengthClassNameItem) != S_OK)
				return std::nullopt;

			pDataItems += lengthClassNameItem + sizeof(WCHAR); //Plus null terminating.
			wstrItem += L"Class name: \"" + std::wstring { pwstrClassNameItem } + L"\", ";
		}

		//Item Title (Caption).
		if (*reinterpret_cast<PWORD>(pDataItems) == 0xFFFF) //Item Title is ordinal
		{
			const auto wTitleOrdinalItem = *reinterpret_cast<PWORD>(pDataItems + sizeof(WORD));
			pDataItems += sizeof(WORD) * 2;
			wstrItem += std::format(L"Caption ordinal: 0x{:04X}, ", wTitleOrdinalItem);
		}
		else //Title is wstring.
		{
			pDataItems += (sizeof(WORD) - ((reinterpret_cast<DWORD_PTR>(pDataItems) -
				reinterpret_cast<DWORD_PTR>(pDataDlg)) & 1)) & 1; //WORD Aligning.
			size_t lengthTitleItem { };
			const auto* pwstrTitleItem = reinterpret_cast<WCHAR*>(pDataItems);
			if (StringCbLengthW(pwstrTitleItem, nSizeDataDlg - (reinterpret_cast<DWORD_PTR>(pDataItems) -
				reinterpret_cast<DWORD_PTR>(pDataDlg)), &lengthTitleItem) != S_OK)
				return std::nullopt;

			pDataItems += lengthTitleItem + sizeof(WCHAR); //Plus null terminating.
			wstrItem += L"Caption: \"" + std::wstring { pwstrTitleItem } + L"\", ";
		}

		//Extra count Item.
		const auto wExtraCountItem = *reinterpret_cast<PWORD>(pDataItems);
		pDataItems += sizeof(WORD);
		if (wExtraCountItem)
		{
			pDataItems += (sizeof(WORD) - ((reinterpret_cast<DWORD_PTR>(pDataItems) -
				reinterpret_cast<DWORD_PTR>(pDataDlg)) & 1)) & 1; //WORD Aligning.
			pDataItems += wExtraCountItem;
		}

		wstrItem += wstrItemStyles;
		wstrRet += wstrItem + L"\r\n";
	}
	wstrRet += L"}";

	return { std::move(wstrRet) };
}