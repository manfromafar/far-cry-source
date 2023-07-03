#include "stdafx.h"
#include "CryModEffMorph.h"

CryModEffMorph::CryModEffMorph(/*CryModelAnimationContainer* pAnimations*/)
//	:m_pAnimations(pAnimations)
{
}


// starts the morphing sequence
void CryModEffMorph::StartMorph (int nMorphTargetId, const CryCharMorphParams& rParams)
{
	m_Params = rParams;
	m_nMorphTargetId = nMorphTargetId;
	m_fTime = rParams.fStartTime;
	m_nFlags = rParams.nFlags;
}

void CryModEffMorph::stop()
{
	m_nMorphTargetId = -1;
}

// advances the current time of the played animation and returns the blending factor by which this animation affects the bone pose
void CryModEffMorph::Tick (float fDeltaTime)
{
	if (m_nMorphTargetId < 0)
		return;

	if (!(m_nFlags & m_Params.FLAGS_FREEZE))
		m_fTime += fDeltaTime * m_Params.fSpeed;
	
	if (!(m_nFlags & m_Params.FLAGS_NO_BLENDOUT) && m_fTime > m_Params.fBlendIn + m_Params.fBlendOut + m_Params.fLength)
		// we're finished
		m_nMorphTargetId = -1;
}

// returns the blending for the morph target
float CryModEffMorph::getBlending()const
{
	float fTimeStable = m_fTime - m_Params.fBlendIn;
	
	if (fTimeStable < 0) // blending in...
		return m_fTime*m_Params.fAmplitude/m_Params.fBlendIn;

	if (m_nFlags & m_Params.FLAGS_NO_BLENDOUT)
		return m_Params.fAmplitude; // never blending out - stable morph

	float fTimeBlendOut = fTimeStable - m_Params.fLength;
	
	if (fTimeBlendOut < 0)
		return m_Params.fAmplitude;

	return m_Params.fAmplitude * (1 - fTimeBlendOut/m_Params.fBlendOut);
}

// returns false when this morph target is inactive
bool CryModEffMorph::isActive()const
{
	return m_nMorphTargetId >= 0;
}

// returns the morph target, or -1 if none
int CryModEffMorph::getMorphTargetId () const
{
	return m_nMorphTargetId;
}
