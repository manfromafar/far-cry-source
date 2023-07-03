#if !defined(AFX_NUMBERCTRL_H__F928C7EC_F2C9_4272_B538_C670C0B2EF9F__INCLUDED_)
#define AFX_NUMBERCTRL_H__F928C7EC_F2C9_4272_B538_C670C0B2EF9F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NumberCtrl.h : header file
//

#include "NumberCtrlEdit.h"

/////////////////////////////////////////////////////////////////////////////
// CNumberCtrl window

class CNumberCtrl : public CWnd
{
// Construction
public:
	typedef Functor1<CNumberCtrl*>	UpdateCallback;
	enum Flags {
		LEFTARROW = 0x01,	//!< Place arrows at left side of edit control.
		NOBORDER	= 0x02,	//!< Not draw border arroud edit control.
		LEFTALIGN = 0x04,	//!< Align text to left side.
	};

	CNumberCtrl();

// Attributes
public:

// Operations
public:
	void Create( CWnd* parentWnd,UINT ctrlID,int nFlags=0 );
	void Create( CWnd* parentWnd,CRect &rc,UINT nID,int nFlags=0 );

	//! If called will enable undo with givven text when control is modified.
	void EnableUndo( const CString& undoText );

	void SetUpdateCallback( const UpdateCallback &cb ) { m_updateCallback = cb; };
	//void SetBeginUpdateCallback( const UpdateCallback &cb ) { m_beginUpdateCallback = cb; };
	//void SetEndUpdateCallback( const UpdateCallback &cb ) { m_endUpdateCallback = cb; };

	void SetMultiplier( float fMultiplier );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNumberCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNumberCtrl();

	//! Set current value and change step.
	void SetValue( float val,float step=0.01f );
	//! Get current value.
	float GetValue() const;

	//! Get increment step.
	float GetStep() const;
	CString GetValueAsString() const;

	//! Set min/max values.
	void SetRange( float min,float max );

	//! Value in control will be integer.
	void SetInteger( bool enable );
	//! If left is true align text in edit control to left, overwise to right.
	void SetLeftAlign( bool left );

	//! set the internal precision for floats m.m. (default is 2)
	void SetInternalPrecision( int iniDigits );

	//! Ovveridable from MFC.
	void SetFont( CFont* pFont,BOOL bRedraw = TRUE);

	// Generated message map functions
protected:
	void DrawButtons( CDC &dc );
	//{{AFX_MSG(CNumberCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnEditSetFocus();
	afx_msg void OnEditKillFocus();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void GetBtnRect( int btn,CRect &rc );
	int GetBtn( CPoint point );
	void SetBtnStatus( int s );
	void NotifyUpdate( bool tracking );
	void OnEditChanged();

	void SetInternalValue( float val );
	//! Get current value.
	float GetInternalValue() const;

	CNumberCtrlEdit m_edit;

	int m_nFlags;
	int m_iInternalPrecision;									//!< m.m. internal precisicion used for floats default is 2
	float m_step;
	float m_min,m_max;
	mutable float m_value;
	float m_lastUpdateValue;
	float m_multiplier;
	// 0 if no buttons pressed.
	// 1 if up button pressed.
	// 2 if down button pressed.
	int m_btnStatus; 
	int m_btnWidth;
	CPoint m_mousePos;
	bool m_draggin;
	HICON m_upArrow,m_downArrow;
	HCURSOR m_upDownCursor;
	bool m_enabled;
	bool m_noNotify;
	bool m_integer;

	bool m_bUndoEnabled;
	bool m_bDragged;

	CString m_undoText;

	UpdateCallback m_updateCallback;
	//UpdateCallback m_beginUpdateCallback;
	//UpdateCallback m_endUpdateCallback;

	//! calculate the digits right from the comma 
	//! \param infNumber source float number
	//! \param iniMaxPlaces maximum number of places (used for numeric rounding problems)
	//! \return number of places 0..iniMaxPlaces
	static int CalculateDecimalPlaces( float infNumber, int iniMaxPlaces );
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NUMBERCTRL_H__F928C7EC_F2C9_4272_B538_C670C0B2EF9F__INCLUDED_)
