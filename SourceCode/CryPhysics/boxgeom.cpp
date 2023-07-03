#include "stdafx.h"

#include "utils.h"
#include "primitives.h"
#include "overlapchecks.h"
#include "unprojectionchecks.h"
#include "bvtree.h"
#include "singleboxtree.h"
#include "geometry.h"
#include "trimesh.h"
#include "boxgeom.h"

vector2df g_BoxCont[8];
int g_BoxVtxId[8],g_BoxEdgeId[8];

CBoxGeom* CBoxGeom::CreateBox(box *pbox)
{
	m_box.center = pbox->center;
	m_box.size = pbox->size;
	m_box.Basis = pbox->Basis;
	m_box.bOriented = m_box.Basis.IsIdentity()^1;
	m_Tree.SetBox(&m_box);
	m_Tree.Build(this);
	m_minVtxDist = (m_box.size.x+m_box.size.y+m_box.size.z)*1E-4f;
	return this;
}


int CBoxGeom::CalcPhysicalProperties(phys_geometry *pgeom)
{ 
	pgeom->pGeom = this;
	pgeom->origin = m_box.center;
	if (!m_box.bOriented)
		pgeom->q.SetIdentity();
	else
		pgeom->q = quaternionf(m_box.Basis.T());
	pgeom->V = m_box.size.volume()*8;
	pgeom->Ibody.Set(sqr(m_box.size.y)+sqr(m_box.size.z), sqr(m_box.size.x)+sqr(m_box.size.z),
		sqr(m_box.size.x)+sqr(m_box.size.y)) *= pgeom->V*(1.0/3);
	return 1;
}


int CBoxGeom::PointInsideStatus(const vectorf &pt)
{
	vectorf ptloc = pt-m_box.center;
	if (m_box.bOriented)
		ptloc = m_box.Basis*ptloc;
	return isneg(fabsf(ptloc.x)-m_box.size.x) & isneg(fabsf(ptloc.y)-m_box.size.y) & isneg(fabsf(ptloc.z)-m_box.size.z);
}


int CBoxGeom::PrepareForIntersectionTest(geometry_under_test *pGTest, CGeometry *pCollider,geometry_under_test *pGTestColl, bool bKeepPrevContacts)
{
	static short g_BoxIdBuf[3];
	static surface_desc g_BoxSurfaceBuf[3];
	static edge_desc g_BoxEdgeBuf[3];

	pGTest->pGeometry = this;
	pGTest->pBVtree = &m_Tree;
	m_Tree.PrepareForIntersectionTest(pGTest);

	pGTest->primbuf = pGTest->primbuf1 = g_BoxBuf+g_BoxBufPos++;
	pGTest->szprimbuf = 1;
	pGTest->typeprim = box::type;
	pGTest->szprim = sizeof(box);
	pGTest->idbuf = g_BoxIdBuf;
	pGTest->surfaces = g_BoxSurfaceBuf;
	pGTest->edges = g_BoxEdgeBuf;

	pGTest->minAreaEdge = 0;
	return 1;
}


void CBoxGeom::PrepareBox(box *pbox, geometry_under_test *pGTest)
{
	pbox->center = pGTest->R*m_box.center*pGTest->scale + pGTest->offset;
	if (!m_box.bOriented)	pbox->Basis = pGTest->R.T();
	else pbox->Basis = m_box.Basis*pGTest->R.T();
	pbox->bOriented = 1;
	pbox->size = m_box.size*pGTest->scale;
}

int CBoxGeom::PreparePrimitive(geom_world_data *pgwd, primitive *&pprim)
{
	box *pbox = g_BoxBuf+g_BoxBufPos;
	g_BoxBufPos = g_BoxBufPos+1 & sizeof(g_BoxBuf)/sizeof(g_BoxBuf[0])-1;
	pbox->center = pgwd->R*m_box.center*pgwd->scale + pgwd->offset;
	if (!m_box.bOriented) pbox->Basis = pgwd->R.T();
	else pbox->Basis = m_box.Basis*pgwd->R.T();
	pbox->bOriented = 1;
	pbox->size = m_box.size*pgwd->scale;
	pprim = pbox;
	return box::type;
}


int CBoxGeom::GetFeature(int iPrim,int iFeature, vectorf *pt)
{ 
	int i,sg,ix,iy,iz;
	switch (iFeature & 0x60) {
		case 0:    // vertex
			pt->x=m_box.size.x*((iFeature<<1&2)-1); pt->y=m_box.size.y*((iFeature&2)-1); 
			pt->z=m_box.size.z*((iFeature>>1&2)-1); pt[0] = pt[0]*m_box.Basis+m_box.center; return 1;
		case 0x20: // edge
			iz = iFeature>>2&3; ix = inc_mod3[iz]; iy = dec_mod3[iz];
			pt[0][ix]=pt[1][ix]=m_box.size[ix]*((iFeature<<1&2)-1); pt[0][iy]=pt[1][iy]=m_box.size[iy]*((iFeature&2)-1);
			pt[0][iz]=-m_box.size[iz]; pt[1][iz]=m_box.size[iz]; 
			pt[0] = pt[0]*m_box.Basis+m_box.center; pt[1] = pt[1]*m_box.Basis+m_box.center; return 2;
		case 0x40: // face
			iz = iFeature>>1&3; ix = inc_mod3[iz]; iy = dec_mod3[iz]; sg = (iFeature<<1&2)-1;
			for(i=0;i<4;i++) {
				pt[i^(sg&3)][ix] = m_box.size[ix]*(1-((i^i<<1)&2));
				pt[i^(sg&3)][iy] = m_box.size[iy]*(1-(i&2));
				pt[i^(sg&3)][iz] = m_box.size[iz]*sg;
				pt[i^(sg&3)] = pt[i^(sg&3)]*m_box.Basis+m_box.center;
			}
			return 4;
	}
	return 0;
}


int CBoxGeom::GetPrimitiveList(int iStart,int nPrims, int typeCollider,primitive *pCollider,int bColliderLocal,
															 geometry_under_test *pGTest,geometry_under_test *pGTestOp, primitive *pRes,short *pResId)
{
	PrepareBox((box*)pRes, pGTest);
	box *pbox = (box*)pRes;
	float szmax=max(max(pbox->size.x,pbox->size.y),pbox->size.z), szmin=min(min(pbox->size.x,pbox->size.y),pbox->size.z);
	if (pGTestOp->ptOutsidePivot.x<1E10 && szmin*8.0f<szmax) {
		int iz=idxmin3((float*)&pbox->size), ix=inc_mod3[iz], iy=dec_mod3[iz];
		vectorf ptloc = pbox->Basis*(pGTestOp->ptOutsidePivot-pbox->center);
		if (ptloc[ix]<pbox->size[ix] && ptloc[iy]<pbox->size[iy]) {
			pbox->center -= pbox->Basis.GetRow(iz)*(sgnnz(ptloc[iz])*szmax);
			pbox->size[iz] += szmax;
		}
	}
	pGTest->bTransformUpdated = 0; *pResId = -1;
	return 1;
}


int CBoxGeom::GetUnprojectionCandidates(int iop,const contact *pcontact, primitive *&pprim,int *&piFeature, geometry_under_test *pGTest)
{
	if (pGTest->bTransformUpdated) {
		pprim = pGTest->primbuf1;
		PrepareBox((box*)pprim, pGTest);
	}
	box *pbox = (box*)pprim;
	pGTest->idbuf[0] = -1;

	int iFeature = pcontact->iFeature[iop],ix,iy,iz;
	vectorf ptloc = pbox->Basis*(pcontact->pt-pbox->center);

	if ((iFeature&0xE0)==0x40) { // check if the point on the face is close to some edge
		iz=iFeature>>1 & 3; ix=inc_mod3[iz]; iy=dec_mod3[iz]; 
		if (fabsf(ptloc[ix]-pbox->size[ix]) < pbox->size[ix]*0.001f)
			iFeature = 0x20 | ix<<2 | (iFeature&1<<1) | sgnnz(ptloc[iy])+1>>1;
		else if (fabsf(ptloc[iy]-pbox->size[iy]) < pbox->size[iy]*0.001f)
			iFeature = 0x20 | iy<<2 | sgnnz(ptloc[ix])+1 | iFeature&1;
	}
	if ((iFeature&0xE0)==0x20) { // check if the point on the edge is close to one of its ends
		iz = iFeature>>2 & 3; ix=inc_mod3[iz]; iy=dec_mod3[iz]; 
		if (fabsf(ptloc[iz]-pbox->size[iz])<pbox->size[iz]*0.001f)
			iFeature = (iFeature&1)<<ix | (iFeature>>1&1)<<iy | (sgnnz(ptloc[iz])+1>>1)<<iz;
	}

	if ((iFeature&0xE0)==0x40) {
		iz = iFeature>>1 & 3;
		pGTest->surfaces[0].n = pbox->Basis.GetRow(iz)*((iFeature<<1&2)-1);
		pGTest->surfaces[0].idx = 0;
		pGTest->surfaces[0].iFeature = iFeature;
		pGTest->nSurfaces = 1;
		pGTest->nEdges = 0;
	} else if ((iFeature&0xE0)==0x20) {
		iz = iFeature>>2 & 3; ix=inc_mod3[iz]; iy=dec_mod3[iz]; 
		pGTest->surfaces[0].idx = pGTest->surfaces[1].idx = 0;
		pGTest->edges[0].n[0] = -(pGTest->surfaces[0].n = pbox->Basis.GetRow(ix)*((iFeature<<1&2)-1));
		pGTest->edges[0].n[1] = -(pGTest->surfaces[1].n = pbox->Basis.GetRow(iy)*((iFeature&2)-1));
		pGTest->surfaces[0].iFeature = 0x40 | ix<<1 | iFeature&1;
		pGTest->surfaces[1].iFeature = 0x40 | iy<<1 | iFeature>>1&1;
		pGTest->nSurfaces = 2;
		pGTest->edges[0].dir = pbox->Basis.GetRow(iz);
		pGTest->edges[0].idx = 0;
		pGTest->edges[0].iFeature = iFeature;
		pGTest->nEdges = 1;
	} else {
		for(iz=0;iz<3;iz++) {
			pGTest->surfaces[iz].idx = pGTest->edges[iz].idx = 0;
			pGTest->edges[iz].dir = pGTest->surfaces[iz].n = pbox->Basis.GetRow(iz)*(((iFeature>>iz)<<1&2)-1);
			pGTest->surfaces[iz].iFeature = 0x40 | iz<<1 | iFeature>>iz&1;
			ix = inc_mod3[iz]; iy = dec_mod3[iz];
			pGTest->edges[iz].n[0] = pbox->Basis.GetRow(ix)*(1-((iFeature>>ix)<<1&2));
			pGTest->edges[iz].n[1] = pbox->Basis.GetRow(iy)*(1-((iFeature>>iy)<<1&2));
			pGTest->edges[iz].iFeature = 0x20 | iz<<2 | (iFeature>>iy&1)<<1 | iFeature>>ix&1;
		}
		pGTest->nSurfaces = pGTest->nEdges = 3;
	}

	return 1;
}


int CBoxGeom::PreparePolygon(coord_plane *psurface, int iPrim,int iFeature, geometry_under_test *pGTest, vector2df *&ptbuf,
				 										 int *&pVtxIdBuf,int *&pEdgeIdBuf)
{
	int i,sg;
	vectorf size,center,pt;
	matrix3x3f R; 
	if (!m_box.bOriented) R = pGTest->R;
	else R = pGTest->R*m_box.Basis.T();
	Vec3_tpl<int> ic;
	size = m_box.size*pGTest->scale;
	ic.z = iFeature>>1&3; ic.x = inc_mod3[ic.z]; ic.y = dec_mod3[ic.z]; sg = (iFeature<<1&2)-1;
	center = pGTest->R*m_box.center*pGTest->scale + pGTest->offset + R.GetColumn(ic.z)*(size[ic.z]*sg);

	if (psurface->n.len2()==0) {
		psurface->n = R.GetColumn(ic.z)*sg;
		psurface->axes[0] = R.GetColumn(ic.x);
		psurface->axes[1] = R.GetColumn(ic.y)*sg;
		psurface->origin = center; sg &= 3;
	}	else
		sg = -isneg(psurface->n*R.GetColumn(ic.z))&3;
		//sg = sgnnz(psurface->n*R.GetColumn(ic.z))>>31 & 3;
	center -= psurface->origin;

	for(i=0;i<4;i++) {
		pt[ic.x] = size[ic.x]*(1-((i^i<<1)&2));
		pt[ic.y] = size[ic.y]*(1-(i&2));
		pt[ic.z] = 0;
		pt = center+R*pt;
		g_BoxCont[i^sg].set(pt*psurface->axes[0],pt*psurface->axes[1]);
		g_BoxEdgeId[(i^sg)-(sg&1) & 3] = 0x20 | ic[i&1]<<2 | (iFeature&1)<<(i&1) | (((i^i>>1)&1)^1)<<((i&1)^1);
		g_BoxVtxId[i^sg] = (iFeature&1)<<ic.z | ((i^i>>1)&1)<<ic.x | (i>>1&1)<<ic.y;
	}
	g_BoxCont[4] = g_BoxCont[0];

	ptbuf = g_BoxCont; pVtxIdBuf = g_BoxVtxId; pEdgeIdBuf = g_BoxEdgeId;
	return 4;
}

int CBoxGeom::PreparePolyline(coord_plane *psurface, int iPrim,int iFeature, geometry_under_test *pGTest, vector2df *&ptbuf,
														  int *&pVtxIdBuf,int *&pEdgeIdBuf)
{
	int ix,iy,iz;
	vectorf pt,offs;
	matrix3x3f R; 
	if (!m_box.bOriented) R = pGTest->R;
	else R = pGTest->R*m_box.Basis.T();
	iz=iFeature>>2&3; ix=inc_mod3[iz]; iy=dec_mod3[iz];
	offs[ix] = m_box.size[ix]*((iFeature<<1&2)-1);
	offs[iy] = m_box.size[iy]*((iFeature&2)-1);
	offs[iz] = -m_box.size[iz];

	pt = (pGTest->R*m_box.center+R*offs)*pGTest->scale + pGTest->offset - psurface->origin;
	g_BoxCont[0].set(pt*psurface->axes[0],pt*psurface->axes[1]);
	offs[ix]=offs[iy] = 0; offs[iz] = m_box.size[iz]*2;
	pt += R*offs*pGTest->scale;
	g_BoxCont[1].set(pt*psurface->axes[0],pt*psurface->axes[1]);
	g_BoxEdgeId[0] = g_BoxEdgeId[1] = iFeature;
	g_BoxVtxId[0] = (iFeature&1)<<ix | (iFeature>>1&1)<<iy;
	g_BoxVtxId[1] = g_BoxVtxId[0] | 1<<iz;
	g_BoxCont[2] = g_BoxCont[0];
	
	ptbuf = g_BoxCont; pVtxIdBuf = g_BoxVtxId; pEdgeIdBuf = g_BoxEdgeId;
	return 2;
}


int CBoxGeom::FindClosestPoint(geom_world_data *pgwd, int &iPrim,int &iFeature, const vectorf &ptdst0,const vectorf &ptdst1,
															 vectorf *ptres, int nMaxIters)
{
	int i,iz,nFacesi,nFaces[2],bLine;
	vectori sg[2],idir[2],sgi,idiri;
	vectorf ptloc[2],size,diri,dir[2],ptdst[2]={ ptdst0,ptdst1 };
	ptres[0].Set(1E10f,1E10f,1E10f);
	ptres[1] = ptdst0;

	ptloc[0] = (ptdst0-pgwd->offset)*pgwd->R-m_box.center*pgwd->scale;
	if (m_box.bOriented)
		ptloc[0] = m_box.Basis*ptloc[0];
	size = m_box.size*pgwd->scale;
	iPrim = 0;

	if (bLine = isneg(size.x*1E-5f-(ptdst0-ptdst1).len2())) {
		int ix,iy,iedge,ivtx;
		vectorf ptbox,n;
		float n2,dir2,t0,t1;
		ptloc[1] = (ptdst1-pgwd->offset)*pgwd->R-m_box.center*pgwd->scale;
		if (m_box.bOriented)
			ptloc[1] = m_box.Basis*ptloc[1];
		diri = ptloc[1]-ptloc[0]; dir2 = diri.len2();

		for(iz=0;iz<3;iz++) {
			n = cross_with_ort(diri,iz); n2 = n.len2(); 
			ptbox[iz] = 0; ix = inc_mod3[iz]; iy = dec_mod3[iz];
			for(iedge=0;iedge<4;iedge++) {
				ptbox[ix] = size[ix]*((iedge&1)*2-1); ptbox[iy] = size[iy]*((iedge&2)-1);
				t0 = cross_with_ort(ptbox-ptloc[0],iz)*n;
				t1 = (ptbox-ptloc[0]^diri)*n;
				if (inrange(t0, (float)0,n2) & isneg(fabs_tpl(t1)-n2*size[iz])) {
					iFeature = 0x20|iz<<2|iedge; n2 = 1.0f/n2;
					ptbox[iz] = t1*n2;
					ptres[0] = pgwd->R*(ptbox*m_box.Basis+m_box.center*pgwd->scale)+pgwd->offset;
					ptres[1] = pgwd->R*((ptloc[0]+diri*(t0*n2))*m_box.Basis+m_box.center*pgwd->scale)+pgwd->offset;
					return 1;
				}
			}
		}
		
		for(ivtx=0;ivtx<8;ivtx++) {
			ptbox.x = size.x*((ivtx&1)*2-1); ptbox.y = size.y*((ivtx&2)-1); ptbox.z = size.z*((ivtx>>1&2)-1);;
			if (inrange(t0=(ptbox-ptloc[0])*diri,(float)0,dir2)) {
				iFeature = ivtx;
				ptres[0] = pgwd->R*(ptbox*m_box.Basis+m_box.center*pgwd->scale)+pgwd->offset;
				ptres[1] = pgwd->R*((ptloc[0]+diri*(t0/dir2))*m_box.Basis+m_box.center*pgwd->scale)+pgwd->offset;
				return 1;
			}
		}
	}

	dir[1].zero(); nFaces[1]=0;
	for(i=0;i<=bLine;i++) {
		for(iz=nFacesi=0;iz<3;iz++) {
			sgi[nFacesi] = sgnnz(ptloc[i][iz]);
			diri[iz] = max(0.0f,fabsf(ptloc[i][iz])-size[iz])*-sgi[nFacesi];
			idiri[nFacesi] = iz; nFacesi += isneg(1E-4f-fabsf(diri[iz]));
		}
		nFaces[i]=nFacesi; sg[i]=sgi;
		dir[i]=diri; idir[i]=idiri;
	}
	if (nFaces[0]+nFaces[1]==0)
		return -1;
	i = isneg(dir[1].len2()-dir[0].len2()) & isneg(-nFaces[1]);

	if (nFaces[i]==1)
		iFeature = 0x40 | idir[i][0]<<1 | sg[i][0]+1>>1;
	else if (nFaces[i]==2) {
		iz = 3-idir[i][0]-idir[i][1];
		iFeature = 0x20 | iz<<2 | (sg[i][0]+1>>1)<<(iz&1) | sg[i][1]+1>>(iz&1);
	} else
		iFeature = sg[i][2]+1<<1 | sg[i][1]+1 | sg[i][0]+1>>1;
	ptloc[i] += dir[i];
	if (m_box.bOriented)
		ptloc[i] = ptloc[i]*m_box.Basis;
	ptres[0] = pgwd->R*(ptloc[i]+m_box.center*pgwd->scale)+pgwd->offset;
	ptres[1] = ptdst[i];

	return 1;
}


void CBoxGeom::BuildTriMesh(CTriMesh &mesh)
{
	static vectorf normals[12],vtx[8];
	static int idx[36] = { 0,4,6, 6,2,0,  1,3,7, 7,5,1,  0,1,5, 5,4,0,  2,6,7, 7,3,2,  0,2,3, 3,1,0,  4,5,7, 7,6,4 };
	mesh.m_nTris = 12;
	mesh.m_nVertices = 8;
	mesh.m_pNormals = normals;
	mesh.m_pVertices = vtx;
	mesh.m_pIndices = idx;
	mesh.m_flags = 4;

	int i; for(i=0;i<8;i++)	{
		vtx[i].Set(m_box.size.x*((i<<1&2)-1),m_box.size.y*((i&2)-1),m_box.size.z*((i>>1&2)-1));
		if (m_box.bOriented) vtx[i] = vtx[i]*m_box.Basis;
		vtx[i] += m_box.center;
	}
	for(i=0;i<6;i++) {
		normals[i*2].Set(0,0,0); normals[i*2][i>>1] = ((i&1)<<1)-1;
		if (m_box.bOriented) normals[i*2] = normals[i*2]*m_box.Basis;
		normals[i*2+1] = normals[i*2];
	}
}


int CBoxGeom::UnprojectSphere(vectorf center,float r,float rsep, contact *pcontact)
{
	sphere sph;
	sph.center = center;
	sph.r = rsep;
	if (!box_sphere_overlap_check(&m_box,&sph))
		return 0;

	unprojection_mode umode;
	umode.dir.Set(0,0,0);
	box_sphere_lin_unprojection(&umode,&m_box,-1,&sph,-1,pcontact,0);
	pcontact->pt -= umode.dir*pcontact->t;
	return 1;
}


void CBoxGeom::CalcVolumetricPressure(geom_world_data *gwd, const vectorf &epicenter,float k,float rmin, 
																			const vectorf &centerOfMass, vectorf &P,vectorf &L)
{
	CTriMesh mesh; BuildTriMesh(mesh);
	mesh.CalcVolumetricPressure(gwd,epicenter,k,rmin,centerOfMass,P,L);
}


float CBoxGeom::CalculateBuoyancy(const plane *pplane, const geom_world_data *pgwd, vectorf &massCenter)
{
	CTriMesh mesh; BuildTriMesh(mesh);
	return mesh.CalculateBuoyancy(pplane,pgwd,massCenter);
}

void CBoxGeom::CalculateMediumResistance(const plane *pplane, const geom_world_data *pgwd, vectorf &dPres,vectorf &dLres)
{
	int i,j,ix,iy,iz;
	vectorf pt[4],n, size=m_box.size*pgwd->scale, offset=pgwd->R*m_box.center*pgwd->scale+pgwd->offset;
	matrix3x3f R; 
	if (!m_box.bOriented) R = pgwd->R;
	else R = pgwd->R*m_box.Basis.T();

	dPres.Set(0,0,0); dLres.zero();
	for(i=0;i<6;i++) {
		iz=i>>1; ix=inc_mod3[iz]; iy=dec_mod3[iz];
		n.Set(0,0,0); n[iz] = ((i<<1&2)-1);
		for(j=0;j<4;j++) {
			pt[j][ix] = size[ix]*(1-((j^j<<1)&2));
			pt[j][iy] = size[iy]*((j&2^i<<1&2)-1);
			pt[j][iz] = size[iz]*n[iz];
			pt[j] = R*pt[j]+offset;
		}
		CalcMediumResistance(pt,4,R*n, *pplane, pgwd->v,pgwd->w,pgwd->centerOfMass, dPres,dLres);
	}
}


int CBoxGeom::DrawToOcclusionCubemap(const geom_world_data *pgwd, int iStartPrim,int nPrims, int iPass, int *pGrid[6],int nRes, 
																		 float rmin,float rmax,float zscale)
{
	int i,j,ix,iy,iz;
	vectorf pt[4],n, size=m_box.size*pgwd->scale, offset=pgwd->R*m_box.center*pgwd->scale+pgwd->offset;;
	matrix3x3f R; 
	if (!m_box.bOriented) R = pgwd->R;
	else R = pgwd->R*m_box.Basis.T();

	for(i=0;i<6;i++) {
		iz=i>>1; ix=inc_mod3[iz]; iy=dec_mod3[iz];
		n.Set(0,0,0); n[iz] = (((i&1)<<1)-1);
		for(j=0;j<4;j++) {
			pt[j][ix] = size[ix]*(1-((j^j<<1)&2));
			pt[j][iy] = size[iy]*((j&2^i<<1&2)-1);
			pt[j][iz] = size[iz]*n[iz];
			pt[j] = R*pt[j]+offset;
		}
		RasterizePolygonIntoCubemap(pt,4, iPass, pGrid,nRes, rmin,rmax,zscale);
	}
	return 1;
}


void CBoxGeom::DrawWireframe(void (*DrawLineFunc)(float*,float*), geom_world_data *pgwd, int iLevel)
{
	int i,j;
	vectorf pt[8], size=m_box.size*pgwd->scale, offset=pgwd->R*m_box.center*pgwd->scale+pgwd->offset;
	matrix3x3f R; 
	if (!m_box.bOriented) R = pgwd->R;
	else R = pgwd->R*m_box.Basis.T();
	for(i=0;i<8;i++)
		pt[i] = R*vectorf(size.x*((i<<1&2)-1),size.y*((i&2)-1),size.z*((i>>1&2)-1))+offset;
	for(i=0;i<8;i++) for(j=0;j<3;j++) if (i&1<<j)
		DrawLineFunc(pt[i],pt[i^1<<j]);
}


void CBoxGeom::GetMemoryStatistics(ICrySizer *pSizer)
{
	pSizer->AddObject(this, sizeof(CBoxGeom));
}


void CBoxGeom::Save(CMemStream &stm)
{
	stm.Write(m_box);
	m_Tree.Save(stm);
}

void CBoxGeom::Load(CMemStream &stm)
{
	stm.Read(m_box);
	m_Tree.Load(stm,this);
}