#include "stdafx.h"

#include "utils.h"
#include "primitives.h"
#include "unprojectionchecks.h"
#include "bvtree.h"
#include "singleboxtree.h"
#include "geometry.h"
#include "cylindergeom.h"
#include "trimesh.h"
#include "boxgeom.h"

vector2df g_CylCont[64];
int g_CylContId[64];

CCylinderGeom* CCylinderGeom::CreateCylinder(cylinder *pcyl)
{
	m_cyl.center = pcyl->center;
	m_cyl.axis = pcyl->axis;
	m_cyl.hh = pcyl->hh;
	m_cyl.r = pcyl->r;

	box bbox;
	bbox.Basis.SetRow(2,m_cyl.axis);
	//bbox.Basis.SetRow(0,m_cyl.axis.orthogonal().normalized());
	bbox.Basis.SetRow(0,GetOrthogonal(m_cyl.axis).normalized());
	bbox.Basis.SetRow(1,m_cyl.axis ^ bbox.Basis.GetRow(0));
	bbox.bOriented = 1;
	bbox.center = m_cyl.center;
	bbox.size.z = m_cyl.hh;
	bbox.size.x = bbox.size.y = m_cyl.r;
	m_Tree.SetBox(&bbox);
	m_Tree.Build(this);
	m_minVtxDist = (m_cyl.r+m_cyl.hh)*1E-4f;
	return this;
}


int CCylinderGeom::CalcPhysicalProperties(phys_geometry *pgeom)
{ 
	pgeom->pGeom = this;
	pgeom->origin = m_cyl.center;
	//pgeom->q = quaternionf(vectorf(0,0,1),m_cyl.axis);
	pgeom->q.SetRotationV0V1(vectorf(0,0,1),m_cyl.axis);
	pgeom->V = sqr(m_cyl.r)*m_cyl.hh*(pi*2);
	float x2=pi*m_cyl.hh*sqr(sqr(m_cyl.r))*(1.0/2), z2=pi*sqr(m_cyl.r)*cube(m_cyl.hh)*(2.0/3);
	pgeom->Ibody.Set(x2+z2,x2+z2,x2*2);
	return 1;
}


int CCylinderGeom::PointInsideStatus(const vectorf &pt)
{
	vectorf ptr; float h;
	ptr = pt-m_cyl.center; h = m_cyl.axis*ptr; ptr -= m_cyl.axis*h;
	return isneg(ptr.len2()-sqr(m_cyl.r)) & isneg(fabs_tpl(h)-m_cyl.hh);
}


int CCylinderGeom::PrepareForIntersectionTest(geometry_under_test *pGTest, CGeometry *pCollider,geometry_under_test *pGTestColl, bool bKeepPrevContacts)
{
	static short g_CylIdBuf[1];
	static surface_desc g_CylSurfaceBuf[1];
	static edge_desc g_CylEdgeBuf[1];

	pGTest->pGeometry = this;
	pGTest->pBVtree = &m_Tree;
	m_Tree.PrepareForIntersectionTest(pGTest);

	pGTest->primbuf = pGTest->primbuf1 = g_CylBuf+g_CylBufPos++;
	pGTest->szprimbuf = 1;
	pGTest->typeprim = cylinder::type;
	pGTest->szprim = sizeof(cylinder);
	pGTest->idbuf = g_CylIdBuf;
	pGTest->surfaces = g_CylSurfaceBuf;
	pGTest->edges = g_CylEdgeBuf;

	pGTest->minAreaEdge = pi*m_cyl.r*2.0f/m_nTesselation;
	return 1;
}


void CCylinderGeom::PrepareCylinder(cylinder *pcyl, geometry_under_test *pGTest)
{
	pcyl->center = pGTest->R*m_cyl.center*pGTest->scale + pGTest->offset;
	pcyl->axis = pGTest->R*m_cyl.axis;
	pcyl->hh = m_cyl.hh*pGTest->scale;
	pcyl->r = m_cyl.r*pGTest->scale;
}

int CCylinderGeom::PreparePrimitive(geom_world_data *pgwd, primitive *&pprim)
{
	cylinder *pcyl = g_CylBuf+g_CylBufPos;
	g_CylBufPos = g_CylBufPos+1 & sizeof(g_CylBuf)/sizeof(g_CylBuf[0])-1;
	pcyl->center = pgwd->R*m_cyl.center*pgwd->scale + pgwd->offset;
	pcyl->axis = pgwd->R*m_cyl.axis;
	pcyl->hh = m_cyl.hh*pgwd->scale;
	pcyl->r = m_cyl.r*pgwd->scale;
	pprim = pcyl;
	return cylinder::type;
}


int CCylinderGeom::GetPrimitiveList(int iStart,int nPrims, int typeCollider,primitive *pCollider,int bColliderLocal,
																		geometry_under_test *pGTest,geometry_under_test *pGTestOp, primitive *pRes,short *pResId)
{
	PrepareCylinder((cylinder*)pRes, pGTest);
	pGTest->bTransformUpdated = 0; *pResId = -1;
	return 1;
}


int CCylinderGeom::GetUnprojectionCandidates(int iop,const contact *pcontact, primitive *&pprim,int *&piFeature, geometry_under_test *pGTest)
{
	if (pGTest->bTransformUpdated) {
		pprim = pGTest->primbuf1;
		PrepareCylinder((cylinder*)pprim, pGTest);
	}
	cylinder *pcyl = (cylinder*)pprim;
	pGTest->idbuf[0] = -1;

	int iFeature = pcontact->iFeature[iop],iCap;
	float r2,hh;
	r2 = (pcontact->pt-pcyl->center^pcyl->axis).len2();
	hh = (pcontact->pt-pcyl->center)*pcyl->axis;
	if ((iFeature&0xE0)!=0x20 && fabs_tpl(r2-sqr(pcyl->r))<sqr(m_minVtxDist) && fabs_tpl(fabs_tpl(hh)-pcyl->hh)<m_minVtxDist)
		iFeature = 0x20 | isnonneg(hh);

	if ((iFeature&0xE0)==0x20 || iFeature>0x40) {
		iCap = (iFeature&3)-(iFeature>>6);
		pGTest->surfaces[0].n = pcyl->axis*((iCap<<1)-1);
		pGTest->surfaces[0].idx = 0;
		pGTest->surfaces[0].iFeature = 0x40 | iCap+1;
		pGTest->nSurfaces = 1;
	} else
		pGTest->nSurfaces = 0;

	if ((iFeature&0xE0)==0x20 || iFeature==0x40) {
		pGTest->edges[0].dir = pcyl->axis;
		pGTest->edges[0].n[0] = pcyl->center-pcontact->pt^pcyl->axis;
		pGTest->edges[0].n[1] = -pGTest->edges[0].n[0];
		pGTest->edges[0].idx = 0;
		pGTest->edges[0].iFeature = iFeature;
		pGTest->nEdges = 1;
	} else
		pGTest->nEdges = 0;

	return 1;
}


int CCylinderGeom::PreparePolygon(coord_plane *psurface, int iPrim,int iFeature, geometry_under_test *pGTest, vector2df *&ptbuf,
																	int *&pVtxIdBuf,int *&pEdgeIdBuf)
{
	int i,icap;
	float ang,step;
	vectorf axes[3],pt,center;
	axes[2] = pGTest->R*m_cyl.axis;
	//axes[0] = axes[2].orthogonal().normalized();
	axes[0] = GetOrthogonal(axes[2]).normalized();
	axes[1] = axes[2]^axes[0];
	icap = (((iFeature&3)-(iFeature>>6)<<1)-1);
	center = pGTest->R*m_cyl.center*pGTest->scale + axes[2]*(m_cyl.hh*pGTest->scale*icap) + pGTest->offset;

	if (psurface->n.len2()==0) {
		psurface->n = axes[2]*icap;
		psurface->axes[0] = axes[0]; psurface->axes[1] = axes[1]*icap;
		psurface->origin = center;
	}
	center -= psurface->origin;
	step = (pi*2/m_nTesselation)*sgnnz(axes[2]*psurface->n);
	axes[0] *= (pGTest->scale*m_cyl.r); 
	axes[1] *= (pGTest->scale*m_cyl.r);

	for(i=0,ang=0;i<m_nTesselation;i++,ang+=step) {
		pt = center + axes[0]*cos_tpl(ang) + axes[1]*sin_tpl(ang);
		g_CylCont[i].set(pt*psurface->axes[0],pt*psurface->axes[1]);
		g_CylContId[i] = 0x20 | icap+1>>1;
	}
	g_CylCont[i] = g_CylCont[0]; g_CylContId[2] = g_CylContId[0];

	ptbuf = g_CylCont;
	pVtxIdBuf = pEdgeIdBuf = g_CylContId;
	return i;
}

int CCylinderGeom::PreparePolyline(coord_plane *psurface, int iPrim,int iFeature, geometry_under_test *pGTest, vector2df *&ptbuf,
																	 int *&pVtxIdBuf,int *&pEdgeIdBuf)
{
	vectorf axis,center,pt;
	axis = pGTest->R*m_cyl.axis;
	center = pGTest->R*m_cyl.center*pGTest->scale + pGTest->offset;

	pt = center + axis*(m_cyl.hh*pGTest->scale) - psurface->origin;
	g_CylCont[0].set(pt*psurface->axes[0],pt*psurface->axes[1]);
	pt = center - axis*(m_cyl.hh*pGTest->scale) - psurface->origin;
	g_CylCont[1].set(pt*psurface->axes[0],pt*psurface->axes[1]);
	g_CylCont[2] = g_CylCont[0];
	g_CylContId[0] = g_CylContId[1] = g_CylContId[2] = 0x40;

	ptbuf = g_CylCont;
	pVtxIdBuf = pEdgeIdBuf = g_CylContId;
	return 2;
}


int CCylinderGeom::FindClosestPoint(geom_world_data *pgwd, int &iPrim,int &iFeature, const vectorf &ptdst0,const vectorf &ptdst1,
																		vectorf *ptres, int nMaxIters)
{
	vectorr axis,center,pt,l,ptdst[]={ptdst0,ptdst1},ptresi[2];
	real r,hh,r2;
	int i,icap,bLine;
	axis = pgwd->R*m_cyl.axis;
	center = pgwd->R*m_cyl.center*pgwd->scale + pgwd->offset;
	r = m_cyl.r*pgwd->scale; r2 = r*r; hh = m_cyl.hh*pgwd->scale;
	pt = ptdst0-center; ptres[1] = ptdst0;

	if (bLine = isneg(r2*1E-6f-(l=ptdst1-ptdst0).len2())) {
		vectorr n,la,pa;
		real n2,l2,t0,t1,kt,pl,pal,roots[4];
		polynomial_tpl<real,4> pn;

		n = l^axis;	n2 = n.len2();
		if (isneg(n2*r2-sqr(pt*n)) & inrange(t0=(-pt^axis)*n, (real)0,n2) & inrange(t1=(-pt^l)*n, (real)0,n2*hh))	{
			ptres[1] = ptdst0+l*(t0/n2);	// line-cylinder side distance
			ptres[0] = center+axis*(axis*(ptres[1]-center));
			ptres[0] = center+(ptres[1]-ptres[0]).normalized()*r;
			return 1;
		}

		icap = sgnnz(t1);
		la = l-axis*(l*axis);	l2 = l.len2(); kt = l2-sqr(axis*l);
		pt = ptdst0-(center+axis*(hh*icap));
		pa = pt-axis*(pt*axis); pl = pt*l; pal = pa*l; 
		pn = psqr(P1(l2)+pt*l)*(P2(la.len2())+P1((pa*la)*2)+pa.len2()) - psqr(P1(kt*r)+(pa*l)*r);
		if (pn.nroots(0,1)) for(i=pn.findroots(0,1,roots)-1;i>=0;i--) 
		if ((pl+l2*roots[i])*(pal+kt*roots[i])>0) { // skips phantom roots
			pa = pt+l*roots[i];
			if (isneg(r2-(pa^axis).len2()) & isneg((pa*axis)*-icap)) { // check if this is the global minimum
				center += axis*(hh*icap);
				ptres[1] = center + pa;
				ptres[0] = center + (pa-axis*(axis*pa)).normalized()*r;
				return 1;
			}
		}
	}

	ptresi[1].zero();
	for(i=0;i<=bLine;i++)	{
		pt = ptdst[i]-center;
		if (fabsf(pt*axis)<hh) { // the closest point lies on cylinder side
			pt -= axis*(axis*pt);
			ptresi[i] = ptdst[i]-pt+pt.normalized()*r;
			continue;
		}
		if ((pt^axis).len2()<r*r) { // .. cylinder cap
			ptresi[i] = ptdst[i]+axis*(sgn(pt*axis)*hh-pt*axis);
			continue;
		}
		// cylinder cap edge
		ptresi[i] = center+axis*(sgn(axis*pt)*hh) + (pt-axis*(axis*pt)).normalized()*r;
	}
	i = bLine & isneg((ptresi[1]-ptdst[1]).len2()-(ptresi[0]-ptdst[0]).len2());
	ptres[0] = ptresi[i];
	ptres[1] = ptdst[i];
	return 1;
}


int CCylinderGeom::UnprojectSphere(vectorf center,float r,float rsep, contact *pcontact)
{
	vectorf dc = center-m_cyl.center;
	float axdist2,capdist,r2=sqr(m_cyl.r);
	axdist2 = max((dc^m_cyl.axis).len2(),r2);
	capdist = max(0.0f,fabsf(dc*m_cyl.axis)-m_cyl.hh);
	if (sqr_signed(axdist2+r2+sqr(capdist)-sqr(rsep)) > axdist2*r2*4)
		return 0;

	unprojection_mode umode;
	sphere sph;
	sph.center = center;
	sph.r = r;
	umode.dir.Set(0,0,0);
	cylinder_sphere_lin_unprojection(&umode,&m_cyl,-1,&sph,-1,pcontact,0);
	pcontact->pt -= umode.dir*pcontact->t;
	return 1;
}


void CCylinderGeom::CalcVolumetricPressure(geom_world_data *gwd, const vectorf &epicenter,float k,float rmin, 
																					 const vectorf &centerOfMass, vectorf &P,vectorf &L)
{
	vectorf c,dc;
	float blendf,area,r;
	c = gwd->R*m_cyl.center*gwd->scale + gwd->offset;
	dc = c-epicenter; r = dc.len(); 
	if (r>m_cyl.r*0.01f)
		dc/=r;
	else dc.Set(0,0,1);
	blendf = (gwd->R*m_cyl.axis^dc).len();
	area = (pi*sqr(m_cyl.r)*(1.0f-blendf) + m_cyl.hh*m_cyl.r*4*blendf)*sqr(gwd->scale);
	P += dc*(area*k/sqr(max(r,rmin)));
}


// debug testing code
/*float x,x0,dx=r*0.001f,Stest,Stesta,CxStest,CxStesta,Vtest,CxVtest,CzVtest,CxVtesta,CzVtesta;
for(x0=a,Vtest=CxVtest=CzVtest=0; x0<b; x0+=dx) {
	for(x=x0,Stest=CxStest=0; x<b; x+=dx) { 
		Stest += sqrt_tpl(r2-x*x)*2*dx;
		CxStest += x*sqrt_tpl(r2-x*x)*2*dx;
	}
	Stesta = b*sqrt_tpl(r2-b*b)+r2*asin_tpl(b*rinv)-x0*sqrt_tpl(r2-x0*x0)-r2*asin_tpl(x0*rinv);
	CxStesta = (-2.0f/3)*(cube(sqrt_tpl(r2-b*b))-cube(sqrt_tpl(r2-x0*x0)));
	Vtest += Stest*dx*k;
	CxVtest += CxStest*dx*k;
	CzVtest += ((x0-a)*k+z0)*Stest*dx*k;
}*/

/*CTriMesh mesh;
vectorf pt3d[48],normals[92];
int idx[92*3];float x,y;
axes.x=m_cyl.axis.orthogonal().normalized(); axes.y=m_cyl.axis^axes.x;
for(i=0,x=r,y=0; i<24; i++) {
	pt3d[i] = m_cyl.center+axes.x*x+axes.y*y-m_cyl.axis*m_cyl.hh;
	pt3d[i+24] = pt3d[i]+m_cyl.axis*(2*m_cyl.hh);
	k=x; x=x*cos_tpl(pi/12)-y*sin_tpl(pi/12); y=y*cos_tpl(pi/12)+k*sin_tpl(pi/12);
}
for(i=0;i<22;i++) { idx[i*3]=0; idx[i*3+1]=i+2;	idx[i*3+2]=i+1; normals[i]=-m_cyl.axis; }
for(;i<44;i++) { idx[i*3]=24; idx[i*3+1]=i+3; idx[i*3+2]=i+4; normals[i]=m_cyl.axis; }
for(;i<92;i++) { 
	idx[i*3]=i-44>>1; idx[i*3+1]=idx[i*3]+1; idx[i*3+2]=idx[i*3]+24; normals[i]=(pt3d[idx[i*3+1]]-pt3d[idx[i*3]]^pt3d[idx[i*3+2]]-pt3d[idx[i*3]]).normalized();	i++;
	idx[i*3]=(i-44>>1)+1; idx[i*3+1]=idx[i*3]+24; idx[i*3+2]=idx[i*3]+23; normals[i]=(pt3d[idx[i*3+1]]-pt3d[idx[i*3]]^pt3d[idx[i*3+2]]-pt3d[idx[i*3]]).normalized();
}
float V1; vectorf c1;
mesh.m_pVertices.data = pt3d;
mesh.m_pIndices = idx;
mesh.m_pNormals = normals;
mesh.m_nVertices=48; mesh.m_nTris=92;
V1 = mesh.CalculateBuoyancy(pplane,pgwd,c1);
if (fabs_tpl(V1-Vaccum)>V1*0.05f || (c1-massCenter).len2()>r2*sqr(0.05f))
i=0;
mesh.m_pNormals=0; mesh.m_pIndices=0; mesh.m_pVertices.data=0;*/

/*vectorf axisx,axisy,pt3d[16];
axisx = vectorf(1,0,0).rotated(rotax,n.z,-sina);
axisy = vectorf(0,1,0).rotated(rotax,n.z,-sina);
pt[0].set(r,0);	itype[0]=1;	pt3d[0] = center+axisx*pt[0].x+axisy*pt[0].y;
for(i=1;i<8;i++) { 
	itype[i]=1; pt[i].x=(pt[i-1].x-pt[i-1].y)*(sqrt2/2); pt[i].y=(pt[i-1].x+pt[i-1].y)*(sqrt2/2);
	pt3d[i] = center+axisx*pt[i].x+axisy*pt[i].y;
} npt=8; pt[8]=pt[0];
vectorf Ptest(zero),Ltest(zero);
CalcMediumResistance(pt3d,8,n,*pplane,pgwd->v,pgwd->w,pgwd->centerOfMass,Ptest,Ltest);*/

/*Ptest += dP.rotated(rotax,n.z,-sina);
Ltest += dL.rotated(rotax,n.z,-sina);
if (Ptest.len()+Ltest.len()>0.001)
i=0;*/


float CCylinderGeom::CalculateBuoyancy(const plane *pplane, const geom_world_data *pgwd, vectorf &massCenter)
{
	Vec3_tpl<vectorf> axes;
	float r=m_cyl.r*pgwd->scale, h=m_cyl.hh*pgwd->scale*2, r2=r*r, rinv=1.0f/r, 
		denom,k,a,b,z0,z1,ya,yb,anglea,angleb,V[4],Vaccum;
	vectorf n = pplane->n, center = pgwd->R*m_cyl.center+pgwd->offset, com[3],com_accum;
	int i,nPieces=0;
	axes.z = pgwd->R*m_cyl.axis; axes.z *= sgnnz(axes.z*n);
	center -= axes.z*(h*0.5f); axes.y = n^axes.z;
	massCenter = center+axes.z*(h*0.5f);

	if (axes.y.len2()<0.0001f) {
		// water plane is perpendicular to cylinder axis
		z0 = max(0.0f,min(h,(pplane->origin-center)*pplane->n));
		massCenter = center+axes.z*(z0*0.5f);
		return pi*r2*z0;
	}
	axes.y.normalize(); axes.x = axes.y^axes.z;

	denom = 1.0f/(axes.x*n);
	a = ((pplane->origin-center)*n)*denom;
	if (a>=r)
		return 0;
	else if (a<-r) {
		if (sqr(axes.z*n)<0.0001f)
			return pi*r2*h;
		a = -r; z0 = ((pplane->origin-(center-axes.x*r))*n)/ (axes.z*n);
		V[nPieces] = pi*r2*z0; com[nPieces++] = axes.z*(z0*0.5f*V[nPieces]); 
	} else z0 = 0;

	b = ((pplane->origin-(center+axes.z*h))*n)*denom;
	if (b<-r)
		return pi*r2*h;
	else if (b>r) {
		b = r; z1 = ((pplane->origin-(center+axes.x*r))*n)/ (axes.z*n);
	}	else {
		z1 = h; yb = sqrt_tpl(r2-b*b);
		V[nPieces] = (r2*(pi*0.5f-asin_tpl(b*rinv))-b*yb)*(h-z0);
		com[nPieces++] = axes.x*((2.0f/3)*cube(yb)*(h-z0)) + axes.z*((h+z0)*0.5f*V[nPieces]);
	}

	if (b>a+r*0.01f) {
		denom = 1.0f/(b-a); k = (z1-z0)*denom; 
		ya = sqrt_tpl(r2-a*a); anglea = asin_tpl(a*rinv);
		yb = sqrt_tpl(r2-b*b); angleb = asin_tpl(b*rinv);
		V[nPieces] = k*((b*yb+r2*angleb)*(b-a)+(1.0f/3)*(cube(yb)-cube(ya))-r2*(b*angleb-a*anglea+yb-ya));
		com[nPieces] = axes.z*(k*k*0.5f*((b*yb+r2*angleb)*(b*b-a*a)+0.5f*(b*cube(yb)-a*cube(ya))-r2*(0.75f*(b*yb-a*ya)-
			0.25f*r2*(angleb-anglea)+b*b*angleb-a*a*anglea)) + V[nPieces]*(z0-a*k));
		com[nPieces++] += axes.x*((2.0f/3)*k*(0.25f*(b*cube(yb)-a*cube(ya))+(3.0f/8)*r2*(b*yb-a*ya+r2*(angleb-anglea))-
			cube(yb)*(b-a)));
	}

	for(i=0,Vaccum=0,com_accum.Set(0,0,0);i<nPieces;i++) {
		Vaccum += V[i]; com_accum += com[i];//*V[i];
	}

	if (Vaccum>0)
		massCenter = center+com_accum/Vaccum;
	else Vaccum=0;
	return Vaccum;
}


int crop_polyarc2d_with_line(const vector2df *pt,const int *itype,int npt, float r, vector2df *ptout,int *itypeout, 
														 const vector2df &p0, const vector2df &l)
{
	if (npt==0) return 0;
	vector2df d0,c,ptt;
	quotientf at[2],a[2],t[3];
	float ka,kb,kc,kd;
	int state,i,nout=0,i0,i1,j;

	ka=l.len2(); kb=p0*l; kc=p0.len2()-r*r;
	if ((kd=kb*kb-ka*kc)>=0) {
		t[0].y=t[1].y=ka; kd=sqrt_tpl(kd); 
		t[0].x=-kb-kd; ptt = p0*t[0].y+l*t[0].x; at[0] = fake_atan2(ptt.y,ptt.x);
		t[1].x=-kb+kd; ptt = p0*t[1].y+l*t[1].x; at[1] = fake_atan2(ptt.y,ptt.x);
	}

	state = isnonneg(l ^ pt[0]-p0);
	for(i=0; i<npt; i++) {
		i0=i1 = 0;
		if (itype[i]) {
			d0=pt[i+1]-pt[i]; c=pt[i]-p0;
			t[2].y=d0^l; t[2].x=l^c;
			if (inrange(t[2].x, 0.0f,t[2].y)) {
				i0=2;i1=3; t[2].x=d0^c;
			}
		} else if (kd>=0) {
			a[0] = fake_atan2(pt[i].y,pt[i].x);
			a[1] = fake_atan2(pt[i+1].y,pt[i+1].x);
			i0 = isneg(isneg(a[0]-at[0])+isneg(at[0]-a[1])+isneg(a[1]-a[0])-2);
			i1 = isneg(1-isneg(a[0]-at[1])-isneg(at[1]-a[1])-isneg(a[1]-a[0]))+1;
		}
		if (state) { 
			ptout[nout]=pt[i]; itypeout[nout++]=itype[i]; 
		}
		for(j=i0; j<i1; j++,state^=1) {
			ptout[nout]=p0+l*t[j].x/t[j].y;
			itypeout[nout++] = itype[i]|state;
		}
	}
	return nout;
}


void CCylinderGeom::CalculateMediumResistance(const plane *pplane, const geom_world_data *pgwd, vectorf &dPres,vectorf &dLres)
{
	vectorf n,rotax,v,w,center,dir,org,pt0,dP(zero),dL(zero),ptside[4];
	vector2df ptbuf0[16],ptbuf1[16],*pt=ptbuf0,*pt1=ptbuf1,*ptt;
	float sina,r,rr,r2,hh,x0,y0,x1,y1,dx,dy,Fx,Fxy,Fxx,Fxxy,Fxyy,Fxxx,alpha,square;
	int i,icap,npt,sgx,itypebuf0[16],itypebuf1[16],*itype=itypebuf0,*itype1=itypebuf1,*itypet;
	dPres.zero(); dLres.zero();

	r2 = sqr(r=m_cyl.r*pgwd->scale);
	hh = m_cyl.hh*pgwd->scale;
	n = pgwd->R*-m_cyl.axis;
	rotax = n^vectorf(0,0,1);
	sina = rotax.len();
	if (sina>0.001f)
		rotax/=sina;
	else 
		rotax.Set(1,0,0);
	center = pgwd->R*m_cyl.center*pgwd->scale+pgwd->offset+n*hh;
	org = center + pplane->n*(pplane->n*(pplane->origin-center));
	rr = 1/r;

	x1 = 0.965925826f; y1 = 0.258819045f;	// 15 degrees sin/cos
	ptside[0] = vectorf(x0=r,y0=0,0).rotated(rotax,n.z,-sina)+center;
	for(i=0; i<12; i++) {
		ptside[1] = ptside[0];
		dx = x0; x0 = (x0*sqrt3-y0)*0.5f; y0 = (y0*sqrt3+dx)*0.5f;
		ptside[0] = vectorf(x0,y0,0).rotated(rotax,n.z,-sina)+center;
		ptside[2] = ptside[1]-n*(hh*2); ptside[3] = ptside[0]-n*(hh*2);
		CalcMediumResistance(ptside,4, vectorf(x1,y1,0).rotated(rotax,n.z,-sina),
			*pplane,pgwd->v,pgwd->w,pgwd->centerOfMass,dPres,dLres);
		dx = x1; x1 = (x1*sqrt3-y1)*0.5f; y1 = (y1*sqrt3+dx)*0.5f;
	}

	for(icap=-1; icap<=1; icap+=2,n.flip(),center+=n*(hh*2),rotax.flip())	{
		v = (pgwd->v+(pgwd->w^center-pgwd->centerOfMass)).rotated(rotax,n.z,sina);
		w = pgwd->w.rotated(rotax,n.z,sina);
		pt[0].set(0,r);	pt[1].set(0,-r); pt[2]=pt[0];
		itype[0]=itype[1] = 0; npt = 2;

		dir = pplane->n*(pplane->n*n)-n;
		if (dir.len2() > 0.001f)
			npt = crop_polyarc2d_with_line(pt,itype,npt, r, pt1,itype1, 
				vector2df((org+dir*(((center-org)*n)/(dir*n))-center).rotated(rotax,n.z,sina)), 
				-vector2df(cross_with_ort(pplane->n.rotated(rotax,n.z,sina),2)));
		else {
			ptt=pt;pt=pt1;pt1=ptt; itypet=itype;itype=itype1;itype1=itypet;
			if (center*pplane->n > org*pplane->n)
				npt = 0;
		} pt1[npt] = pt1[0]; 
		dir = pgwd->w^n; 
		if (dir.len2() > w.len2()*0.001f)
			npt = crop_polyarc2d_with_line(pt1,itype1,npt, r, pt,itype, 
				vector2df((dir*((pgwd->v*n-center*dir)/dir.len2())).rotated(rotax,n.z,sina)), vector2df(w));
		else {
			ptt=pt;pt=pt1;pt1=ptt; itypet=itype;itype=itype1;itype1=itypet;
			if (center*(pgwd->w^n) > pgwd->v*n)
				npt = 0;
		}	pt[npt] = pt[0];

		for(i=0,square=0,dP.zero(),dL.zero(); i<npt; i++) {
			x0=pt[i].x; y0=pt[i].y;	x1=pt[i+1].x; y1=pt[i+1].y;
			if (itype[i]) {
				dx = x1-x0; dy = y1-y0;
				Fx = (x0*y1-x1*y0)*0.5f;
				Fxy = dy*(x0*y0 + (dx*y0+dy*x0)*0.5f + dx*dy*(1.0f/3));
				Fxx = dy*(x0*x0 + dx*x0 + dx*dx*(1.0f/3));
				Fxxy = dy*(dx*dx*dy*0.25f + (dx*dx*y0+dx*dy*x0*2)*(1.0f/3) + (x0*y0*dx*2+x0*x0*dy)*0.5f + x0*x0*y0);
				Fxyy = dy*(dy*dy*dx*0.25f + (dy*dy*x0+dy*dx*y0*2)*(1.0f/3) + (y0*x0*dy*2+y0*y0*dx)*0.5f + y0*y0*x0);
				Fxxx = dy*(dx*dx*dx*0.25f + dx*dx*x0 + dx*x0*x0*1.5f + x0*x0*x0);
			}	else {
				alpha = asin_tpl(max(-0.9999f,min(0.9999f,y1*rr)))-asin_tpl(max(-0.9999f,min(0.9999f,y0*rr)));	
				sgx = sgnnz(y1-y0);
				Fx = r2*alpha*0.5f*sgx;
				Fxy = (cube(x1)-cube(x0))*(-1.0f/3);
				Fxx = r2*(y1-y0)-(cube(y1)-cube(y0))*(1.0f/3);
				Fxxy = r2*(sqr(y1)-sqr(y0))*0.5f-(cube(y1)-cube(y0))*(1.0f/3);
				Fxyy = (y1*cube(x1)-y0*cube(x0))*(-1.0f/4) + r2*(1.0f/8)*(x1*y1-x0*y0+r2*alpha*sgx);
				Fxxx = 0.25f*(y1*cube(x1)-y0*cube(x0)+1.5f*r2*(y1*x1-y0*x0+r2*alpha*sgx));
			}
			square += Fx;
			dP.z += w.x*Fxy-w.y*0.5f*Fxx;
			dL.x += v.z*Fxy;
			dL.y -= v.z*0.5f*Fxx;
			dL.x += w.x*Fxyy-w.y*0.5f*Fxxy;
			dL.y -= w.x*0.5*Fxxy-w.y*(1.0f/3)*Fxxx;
		}
		dP.z += v.z*square;
		dPres -= dP.rotated(rotax,n.z,-sina);
		dLres -= dL.rotated(rotax,n.z,-sina);
	}
}


int CCylinderGeom::DrawToOcclusionCubemap(const geom_world_data *pgwd, int iStartPrim,int nPrims, int iPass, int *pGrid[6],int nRes, 
																					float rmin,float rmax,float zscale)
{
	CBoxGeom geombox;
	geombox.m_box = m_Tree.m_Box;
	return geombox.DrawToOcclusionCubemap(pgwd,iStartPrim,nPrims,iPass,pGrid,nRes,rmin,rmax,zscale);
}


void CCylinderGeom::DrawWireframe(void (*DrawLineFunc)(float*,float*), geom_world_data *gwd, int iLevel)
{
	if (iLevel==0) {
		int i; float step,ang;
		vectorf axes[3],center,pt0,pt1;
		step = (pi*2/m_nTesselation);
		axes[2] = gwd->R*m_cyl.axis;
	//	axes[0] = axes[2].orthogonal().normalized();
		axes[0] = GetOrthogonal(axes[2]).normalized();
		axes[1] = axes[2]^axes[0];
		axes[0] *= m_cyl.r*gwd->scale; axes[1] *= m_cyl.r*gwd->scale;
		axes[2] *= m_cyl.hh*gwd->scale;
		center = gwd->R*m_cyl.center*gwd->scale + gwd->offset;
		pt0 = center+axes[0];
		for(i=0,ang=step;i<m_nTesselation;i++,ang+=step,pt0=pt1) {
			pt1 = center + axes[0]*cos_tpl(ang) + axes[1]*sin_tpl(ang);
			DrawLineFunc(pt0-axes[2], pt1-axes[2]);
			DrawLineFunc(pt0+axes[2],	pt1+axes[2]);
			DrawLineFunc(pt1-axes[2], pt1+axes[2]);
		}
	}	else {
		BV *pbbox;
		ResetGlobalPrimsBuffers();
		m_Tree.GetNodeBV(gwd->R,gwd->offset,gwd->scale, pbbox);
		DrawBBox(DrawLineFunc,gwd, &m_Tree,(BBox*)pbbox,iLevel-1);
	}
}


void CCylinderGeom::GetMemoryStatistics(ICrySizer *pSizer)
{
	pSizer->AddObject(this, sizeof(CCylinderGeom));
}


void CCylinderGeom::Save(CMemStream &stm)
{
	stm.Write(m_cyl);
	stm.Write(m_nTesselation);
	m_Tree.Save(stm);
}

void CCylinderGeom::Load(CMemStream &stm)
{
	stm.Read(m_cyl);
	stm.Read(m_nTesselation);
	m_Tree.Load(stm,this);
}