//-------------------------------------------------------------------------------------------------
// Author: M�rcio Martins
//
// Purpose:
//  - Manage and cache glyphs, retrieving them from the renderer as needed
//
// History:
//  - [6/6/2003] created the file
//
//-------------------------------------------------------------------------------------------------
#include "StdAfx.h"
#include "GlyphCache.h"
#include "FontTexture.h"



//------------------------------------------------------------------------------------------------- 
CGlyphCache::CGlyphCache()
: m_dwUsage(1), m_iGlyphBitmapWidth(0), m_iGlyphBitmapHeight(0), m_fSizeRatio(0.8f), m_pScaleBitmap(0)
{
	m_pCacheTable.clear();
	m_pSlotList.clear();
}

//------------------------------------------------------------------------------------------------- 
CGlyphCache::~CGlyphCache()
{
}

//------------------------------------------------------------------------------------------------- 
int CGlyphCache::Create(int iChacheSize, int iGlyphBitmapWidth, int iGlyphBitmapHeight, int iSmoothMethod, int iSmoothAmount, float fSizeRatio)
{
	m_fSizeRatio = fSizeRatio;

	m_iSmoothMethod = iSmoothMethod;
	m_iSmoothAmount = iSmoothAmount;

	m_iGlyphBitmapWidth = iGlyphBitmapWidth;
	m_iGlyphBitmapHeight = iGlyphBitmapHeight;


	if (!CreateSlotList(iChacheSize))
	{
		ReleaseSlotList();

		return 0;
	}

	int iScaledGlyphWidth = 0;
	int iScaledGlyphHeight = 0;

	switch(m_iSmoothMethod)
	{
	case FONT_SMOOTH_SUPERSAMPLE:
		{
			switch (m_iSmoothAmount)
			{
			case FONT_SMOOTH_AMOUNT_2X:
				iScaledGlyphWidth = m_iGlyphBitmapWidth << 1;
				iScaledGlyphHeight = m_iGlyphBitmapHeight << 1;
				break;
			case FONT_SMOOTH_AMOUNT_4X:
				iScaledGlyphWidth = m_iGlyphBitmapWidth << 2;
				iScaledGlyphHeight = m_iGlyphBitmapHeight << 2;
				break;
			}
		}
		break;
	}

	if (iScaledGlyphWidth)
	{
		m_pScaleBitmap = new CGlyphBitmap;

		if (!m_pScaleBitmap)
		{
			Release();

			return 0;
		}

		if (!m_pScaleBitmap->Create(iScaledGlyphWidth, iScaledGlyphHeight))
		{
			Release();

			return 0;
		}

		m_pFontRenderer.SetGlyphBitmapSize(iScaledGlyphWidth, iScaledGlyphHeight);
	}
	else
	{
		m_pFontRenderer.SetGlyphBitmapSize(m_iGlyphBitmapWidth, m_iGlyphBitmapHeight);
	}

	return 1;
}

//------------------------------------------------------------------------------------------------- 
int CGlyphCache::Release()
{
	ReleaseSlotList();

	m_pCacheTable.clear();

	if (m_pScaleBitmap)
	{
		m_pScaleBitmap->Release();

		delete m_pScaleBitmap;

		m_pScaleBitmap = 0;
	}

	m_iGlyphBitmapWidth = 0;
	m_iGlyphBitmapHeight = 0;
	m_fSizeRatio = 0.8f;

	return 1;
}

//------------------------------------------------------------------------------------------------- 
int CGlyphCache::LoadFontFromFile(const string &szFileName)
{
	return m_pFontRenderer.LoadFromFile(szFileName);
}

//------------------------------------------------------------------------------------------------- 
int CGlyphCache::LoadFontFromMemory(unsigned char *pFileBuffer, int iDataSize)
{
	return m_pFontRenderer.LoadFromMemory(pFileBuffer, iDataSize);
}

//------------------------------------------------------------------------------------------------- 
int CGlyphCache::ReleaseFont()
{
	m_pFontRenderer.Release();

	return 1;
}

//------------------------------------------------------------------------------------------------- 
int CGlyphCache::GetGlyphBitmapSize(int *pWidth, int *pHeight)
{
	if (pWidth)
	{
		*pWidth = m_iGlyphBitmapWidth;
	}

	if (pHeight)
	{
		*pHeight = m_iGlyphBitmapWidth;
	}

	return 1;
}

//------------------------------------------------------------------------------------------------- 
int CGlyphCache::PreCacheGlyph(wchar_t cChar)
{
	CCacheTableItor pItor = m_pCacheTable.find(cChar);

	if (pItor != m_pCacheTable.end())
	{
		pItor->second->dwUsage = m_dwUsage;

		return 1;
	}

	CCacheSlot *pSlot = GetLRUSlot();

	if (!pSlot)
	{
		return 0;
	}

	if (pSlot->dwUsage > 0)
	{
		UnCacheGlyph(pSlot->cCurrentChar);
	}

	if (m_pScaleBitmap)
	{
		int iOffsetMult = 1;

		switch (m_iSmoothAmount)
		{
		case FONT_SMOOTH_AMOUNT_2X:
			iOffsetMult = 2;
			break;
		case FONT_SMOOTH_AMOUNT_4X:
			iOffsetMult = 4;
			break;
		}

		m_pScaleBitmap->Clear();

		if (!m_pFontRenderer.GetGlyph(m_pScaleBitmap, &pSlot->iCharWidth, &pSlot->iCharHeight, FONT_GLYPH_OFFSET * iOffsetMult, 0, cChar))
		{
			return 0;
		}

		pSlot->iCharWidth >>= iOffsetMult >> 1;
		pSlot->iCharHeight >>= iOffsetMult >> 1;

		m_pScaleBitmap->BlitScaledTo8(pSlot->pGlyphBitmap.GetBuffer(), 0, 0, m_pScaleBitmap->GetWidth(), m_pScaleBitmap->GetHeight(), 0, 0, pSlot->pGlyphBitmap.GetWidth(), pSlot->pGlyphBitmap.GetHeight(), pSlot->pGlyphBitmap.GetWidth());
	}
	else
	{
		if (!m_pFontRenderer.GetGlyph(&pSlot->pGlyphBitmap, &pSlot->iCharWidth, &pSlot->iCharHeight, FONT_GLYPH_OFFSET, 0, cChar))
		{
			return 0;
		}
	}

	if (m_iSmoothMethod == FONT_SMOOTH_BLUR)
	{
		pSlot->pGlyphBitmap.Blur(m_iSmoothAmount);
	}

	pSlot->dwUsage = m_dwUsage;
	pSlot->cCurrentChar = cChar;

	m_pCacheTable.insert(std::pair<wchar_t, CCacheSlot *>(cChar, pSlot));

	return 1;
}

int CGlyphCache::UnCacheGlyph(wchar_t cChar)
{
	CCacheTableItor pItor = m_pCacheTable.find(cChar);

	if (pItor != m_pCacheTable.end())
	{
		CCacheSlot *pSlot = pItor->second;

		pSlot->Reset();

		m_pCacheTable.erase(pItor);

		return 1;
	}

	return 0;
}

//------------------------------------------------------------------------------------------------- 
int CGlyphCache::GlyphCached(wchar_t cChar)
{
	return (m_pCacheTable.find(cChar) != m_pCacheTable.end());
}

//------------------------------------------------------------------------------------------------- 
CCacheSlot *CGlyphCache::GetLRUSlot()
{
	unsigned int	dwMinUsage = 0xffffffff;
	CCacheSlot		*pLRUSlot = 0;
	CCacheSlot		*pSlot;

	CCacheSlotListItor pItor = m_pSlotList.begin();

	while (pItor != m_pSlotList.end())
	{
		pSlot = *pItor;

		if (pSlot->dwUsage == 0)
		{
			return pSlot;
		}
		else
		{
			if (pSlot->dwUsage < dwMinUsage)
			{
				pLRUSlot = pSlot;
				dwMinUsage = pSlot->dwUsage;
			}
		}

		pItor++;
	}

	return pLRUSlot;
}

//------------------------------------------------------------------------------------------------- 
CCacheSlot *CGlyphCache::GetMRUSlot()
{
	unsigned int	dwMaxUsage = 0;
	CCacheSlot		*pMRUSlot = 0;
	CCacheSlot		*pSlot;

	CCacheSlotListItor pItor = m_pSlotList.begin();

	while (pItor != m_pSlotList.end())
	{
		pSlot = *pItor;

		if (pSlot->dwUsage != 0)
		{
			if (pSlot->dwUsage > dwMaxUsage)
			{
				pMRUSlot = pSlot;
				dwMaxUsage = pSlot->dwUsage;
			}
		}

		pItor++;
	}

	return pMRUSlot;
}

//------------------------------------------------------------------------------------------------- 
int CGlyphCache::GetGlyph(CGlyphBitmap **pGlyph, int *piWidth, int *piHeight, wchar_t cChar)
{
	CCacheTableItor pItor = m_pCacheTable.find(cChar);

	if (pItor == m_pCacheTable.end())
	{
		if (!PreCacheGlyph(cChar))
		{
			return 0;
		}
	}

	pItor = m_pCacheTable.find(cChar);

	pItor->second->dwUsage = m_dwUsage++;
	(*pGlyph) = &pItor->second->pGlyphBitmap;

	if (piWidth)
	{
		*piWidth = pItor->second->iCharWidth;
	}

	if (piHeight)
	{
		*piHeight = pItor->second->iCharHeight;
	}


	return 1;
}

//------------------------------------------------------------------------------------------------- 
//------------------------------------------------------------------------------------------------- 
int	CGlyphCache::CreateSlotList(int iListSize)
{
	for (int i = 0; i < iListSize; i++)
	{
		CCacheSlot *pCacheSlot = new CCacheSlot;

		if (!pCacheSlot)
		{
			return 0;
		}

		if (!pCacheSlot->pGlyphBitmap.Create(m_iGlyphBitmapWidth, m_iGlyphBitmapHeight))
		{
			delete pCacheSlot;

			return 0;
		}

		pCacheSlot->Reset();
		
		pCacheSlot->iCacheSlot = i;

		m_pSlotList.push_back(pCacheSlot);
	}

	return 1;
}

//------------------------------------------------------------------------------------------------- 
int	CGlyphCache::ReleaseSlotList()
{
	CCacheSlotListItor pItor = m_pSlotList.begin();

	while (pItor != m_pSlotList.end())
	{
		(*pItor)->pGlyphBitmap.Release();

		delete (*pItor);

		pItor = m_pSlotList.erase(pItor);
	}
	
	return 1;
}

//------------------------------------------------------------------------------------------------- 