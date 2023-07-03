// AnimSequences.cpp : implementation file
//

#include "stdafx.h"
#include "AnimSequences.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAnimSequences

CAnimSequences::CAnimSequences()
{
}

CAnimSequences::~CAnimSequences()
{
}

BEGIN_MESSAGE_MAP(CAnimSequences, CComboBox)
	//{{AFX_MSG_MAP(CAnimSequences)
	ON_WM_CTLCOLOR()
	ON_WM_CREATE()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAnimSequences message handlers

HBRUSH CAnimSequences::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	////////////////////////////////////////////////////////////////////////
	// The combox box should be drawn with a different font
	////////////////////////////////////////////////////////////////////////

	CFont cComboBoxFont;

	HBRUSH hbr = CComboBox::OnCtlColor(pDC, pWnd, nCtlColor);
		
	if (nCtlColor == CTLCOLOR_LISTBOX)
	{
		VERIFY(cComboBoxFont.CreatePointFont(60, "Terminal"));
		pDC->SelectObject(&cComboBoxFont);
	}
	
	// TODO: Return a different brush if the default is not desired
	return hbr;
}

int CAnimSequences::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	////////////////////////////////////////////////////////////////////////
	// Change some states of the controls
	////////////////////////////////////////////////////////////////////////

	static CFont cComboBoxFont;

	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// Set item size and list box heigt
	VERIFY(SetItemHeight(0, 11) != CB_ERR);
	VERIFY(SetItemHeight(-1, 150) != CB_ERR);

	// Change the font to a smaller one
	VERIFY(cComboBoxFont.CreatePointFont(60, "Terminal"));
	SetFont(&cComboBoxFont, TRUE);

	return 0;
}

void CAnimSequences::OnDropdown() 
{
	
}
