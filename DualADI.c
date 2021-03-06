#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "ham2dtypes.h"
#include "ham2dFunctionDefs.h"
#define NQ 4

void DualADI(GRID *g,SOLN *s,double cflnum,double dt)
{
  //
  int i,j,k,l,m,f,n;
  int mm1,f1,f2,iface,idv,chainSize;
  double ds[2];
  double lmat[NQ][NQ];
  double rmat[NQ][NQ];
  double dsnorm,nynx,nx2ny;
  int node1,node2,leftCell,rightCell,icell;
  double x1,y1,x2,y2,r01,r02,a1,a2,b1,b2,pp,dtfac;
  double linearl2rho;
  double fff,dual_res,alp,dtau;
  int isweep;
  dtau = 100.;
  alp = 1.0;
  //
  // one loop per chain to evaluate fluxes
  // on all the faces in the chain
  //
  for(i=0;i<g->ncells;i++)
  {
    dtfac=cflnum/s->sigma[i];
    //dtfac = dtfac/(alp+dtfac/dtau); //dual
    for(m=0;m<NVAR;m++)
	 {
	   s->r[NVAR*i+m]*=dtfac;
	   s->r0[NVAR*i+m]=s->r[NVAR*i+m];
	   s->dq[NVAR*i+m]=0;
	 }
  }
  //
  for(isweep=0;isweep<g->msweep;isweep++)
  {
    for(i=0;i<g->nchains;i++)
	 {
	   //
	   // zero out block matrices
	   //
	   for(m=0;m<g->nmaxchain+5;m++)
	   for(k=0;k<NQ;k++)
	   {
		  g->F[m][k]=0;
		  for(j=0;j<NQ;j++)
		  g->A[m][k][j]=g->B[m][k][j]=g->C[m][k][j]=0;
	   }
	   //
	   // collect cells on the loop
	   // 
	   f1=g->faceStartPerChain[i];
	   f2=g->faceStartPerChain[i+1];
	   idv=(g->chainConn[f1]==g->chainConn[f2-1]);
	   m=0;
	   chainSize=(f2-idv-f1);
	   //
	   // make matrices A,B,C and vector F for
	   // inversion.
	   //
	   for(f=f1;f<f2-idv;f++)
	   {
	     iface=g->chainConn[f];
	     leftCell=g->faces[6*iface+2];
	     rightCell=g->faces[6*iface+4];
	     // 
	     // construct left and right matrices for this face
	     //
	     for(j=0;j<NQ;j++)
		  for(k=0;k<NQ;k++)
		  {
		    lmat[j][k]=(g->ff[iface].lmat[j][k]);
		    rmat[j][k]=(g->ff[iface].rmat[j][k]);
		  }
	     if (idv==1) 
	     {
		    mm1=(m==0)?chainSize-1:m-1;
		    for(j=0;j<NQ;j++)
		    {
		      for(k=0;k<NQ;k++)
		      {
			     g->B[m][j][k]+=(lmat[j][k]);
			     g->B[mm1][j][k]-=(rmat[j][k]);
			     g->A[m][j][k]+=(rmat[j][k]);
			     g->C[mm1][j][k]-=(lmat[j][k]);
		      }
		      g->F[m][j]=s->r[NVAR*leftCell+j];
		    }
	     }
	     else
		  {
		    mm1=(m-1);
		    if (rightCell==-2)
		    {
		      node1=g->faces[6*iface];
		      node2=g->faces[6*iface+1];
		      x1=g->x[2*node1];
		      y1=g->x[2*node1+1];
		      x2=g->x[2*node2];
		      y2=g->x[2*node2+1];
		      ds[0]=(y2-y1);
		      ds[1]=-(x2-x1);
		      dsnorm=ds[0]*ds[0]+ds[1]*ds[1];
		      nynx=ds[0]*ds[1]/dsnorm;
		      nx2ny=(ds[0]*ds[0]-ds[1]*ds[1])/dsnorm;
		      a1=-nx2ny;
		      b1=-2*nynx;
		      a2=-2*nynx;
		      b2=nx2ny;

		      for (j=0;j<NQ;j++)
			   {
			     r01=rmat[j][1];
			     r02=rmat[j][2];
			     rmat[j][1]=a1*r01+b1*r02;
			     rmat[j][2]=a2*r01+b2*r02;
			   }
		    }
		    //
		    if (rightCell < 0 && idv==0 && f==f2-1) m--;
		    //
		    for(j=0;j<NQ;j++)
		    {
		      for(k=0;k<NQ;k++)
			   {
			     g->B[m][j][k]+=(lmat[j][k]);
			     if (mm1 > -1 && rightCell > -1)
			     {
			       g->A[m][j][k]+=(rmat[j][k]);
			       g->B[mm1][j][k]-=(rmat[j][k]);
			       g->C[mm1][j][k]-=(lmat[j][k]);
			     }
			     else
			     {
			       //invicid wall condition
			       if (rightCell==-2) g->B[m][j][k]+=(rmat[j][k]);
			     } 
			   }
		      g->F[m][j]=s->r[NVAR*leftCell+j];
		    }
		  } //idv 1 or not	  
	     m++;
	   } //f1-f2     

      // for residual
      //if(i==0) fff=g->F[0][0];

	   m=0;
	   chainSize=chainSize-(idv==0);
	   for(f=f1;f<f2-1;f++)
	   {
	     iface=g->chainConn[f];	  
	     leftCell=g->faces[6*iface+2];
	     dtfac=cflnum/s->sigma[leftCell];

        //dtfac = alp*dtfac/(alp+dtfac/dtau); //dual

	     for(j=0;j<NQ;j++)
		    for(k=0;k<NQ;k++)
		    {
		      g->A[m][j][k]*=dtfac;
		      g->B[m][j][k]*=dtfac;
		      g->C[m][j][k]*=dtfac;
		    }
	     for(j=0;j<NQ;j++)
		  g->B[m][j][j]+=1.0+dtfac/dtau;
	     m++;
	   }
	   //
	   // invert using appropriate banded block solver
	   //
	   if (idv==1) blockTridagPeriodic4(g->A,g->B,g->C,g->F,chainSize,NQ);
	   if (idv==0) blockTridag4(g->A,g->B,g->C,g->F,chainSize,NQ);
 
	   //
	   // reassign values back at the unknown locations
	   //
	   m=0;
	   for(f=f1;f<f2-1;f++)
	   {
	     iface=g->chainConn[f];
	     leftCell=g->faces[6*iface+2];
	     for(j=0;j<NQ;j++)
		  s->r[NVAR*leftCell+j]=g->F[m][j];
	     m++;
	   }      
	 } //nchains
    for(i=0;i<g->ncells*NVAR;i++) s->dq[i]+=s->r[i];
    computeLinearRHS(g,s,cflnum,&linearl2rho);
    //tracef(linearl2rho);
  } //sweep number

  //
  // update q
  //
  //tracef(s->dq[0]);
  m=0;
  for(i=0;i<g->ncells;i++)
    for(j=0;j<NVAR;j++)
    {
	   s->pq[m]+=s->dq[m];
	   m++;
    }

  //dual time step residual check
  //dtfac=cflnum/s->sigma[0];
  //dtfac = dtfac/(alp+dtfac/dtau); 
  //dual_res = (s->pq[0]-s->q[0])/(cflnum/s->sigma[0])+fff/dtfac;
  //tracef(dual_res);
  
}
