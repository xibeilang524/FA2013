
// FA2013Doc.h : CFA2013Doc 类的接口
//


#pragma once
#include "Comtrade.h"
#include "atltypes.h"
#include "uniclass.h"
#include "DlgHarmo.h"
#include "DlgPhase.h"
#include "DlgSeqComp.h"
#include "DlgForm.h"
#include "DlgImp.h"
#include "DlgBusDiff.h"

class CAView;
class CDView;
class CDlgHarmo;
class CDlgPhase;
class CDlgSeqComp;
class CDlgForm;
class CDlgImp;
class CDlgBusDiff;

class CFA2013Doc : public CDocument
{
protected: // 仅从序列化创建
	CFA2013Doc();
	DECLARE_DYNCREATE(CFA2013Doc)

// 特性
public:

// 操作
public:

// 重写
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// 实现
public:
	virtual ~CFA2013Doc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// 用于为搜索处理程序设置搜索内容的 Helper 函数
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS


public:
	CComtrade* pFile;
	afx_msg void OnOpenfile();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	CAView* pAView;
	CDView* pDView;
	CPoint chanPos;
	CSize viewSize;
	// 游标1
	CPoint pCursor1;
	CPoint pCursor2;
	int CursorNum1;
	int CursorNum2;
	afx_msg void OnCursor();
	afx_msg void OnVZoom();
	afx_msg void OnHZoom();
	CursorType m_Cursor;
public:
	afx_msg void OnUpdateCursor(CCmdUI *pCmdUI);
	afx_msg void OnUpdateVZoom(CCmdUI *pCmdUI);
	afx_msg void OnUpdateHZoom(CCmdUI *pCmdUI);
	afx_msg void OnFileOpen();
public:
	CDlgHarmo* pDlgHarmo;  //谐波窗口
	CDlgPhase* pDlgPhase;   //相量窗口
	CDlgForm* pDlgForm;     //公式编辑器
	CDlgSeqComp* pDlgSeqComp;	//序分量窗口
	CDlgImp* pDlgImp;           //阻抗分析窗口
	CDlgBusDiff* pDlgBusDiff;   //母差分析窗口
	afx_msg void OnHarmonics();
	int ChanNum1;
	int view_Scroll(int chanNum);
	afx_msg void OnPhase();

	afx_msg void OnSeqcomp();
	afx_msg void OnForm();

	afx_msg void OnImp();
	//Multi Thread
	CWinThread* pThread;
	double ThreadPro;

	afx_msg void OnUpdateIndicatorProgress(CCmdUI *pCmdUI);
	afx_msg void OnBusdiff();
	afx_msg void OnPower();
	int exit;
	afx_msg void OnUpdateIndicatorCursor1(CCmdUI *pCmdUI);
	afx_msg void OnUpdateIndicatorCursor2(CCmdUI *pCmdUI);
};
