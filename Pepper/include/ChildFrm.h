#pragma once

class CChildFrame : public CMDIChildWndEx
{
public:
	DECLARE_DYNCREATE(CChildFrame)
	CChildFrame() {};
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual ~CChildFrame() {};
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	CSplitterWndEx m_MainSplitter, m_RightSplitter;
protected:
	bool m_fSpliterCreated = false;
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	UINT m_cx { }, m_cy { };
};