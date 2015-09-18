/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
   Contributing author: Mark Stevens (SNL)
------------------------------------------------------------------------- */

#include "math.h"
#include "stdlib.h"
#include "dihedral_opls.h"
#include "atom.h"
#include "comm.h"
#include "neighbor.h"
#include "domain.h"
#include "force.h"
#include "update.h"
#include "memory.h"
#include "error.h"

using namespace LAMMPS_NS;

#define TOLERANCE 0.05
#define SMALL     0.001
#define SMALLER   0.00001

/* ---------------------------------------------------------------------- */

DihedralOPLS::DihedralOPLS(LAMMPS *lmp) : Dihedral(lmp)
{
  writedata = 1;
}

/* ---------------------------------------------------------------------- */

DihedralOPLS::~DihedralOPLS()
{
  if (allocated && !copymode) {
    memory->destroy(setflag);
    memory->destroy(k1);
    memory->destroy(k2);
    memory->destroy(k3);
    memory->destroy(k4);
  }
}

/* ---------------------------------------------------------------------- */

void DihedralOPLS::compute(int eflag, int vflag)
{
  int i1,i2,i3,i4,n,type;
  double vb1x,vb1y,vb1z,vb2x,vb2y,vb2z,vb3x,vb3y,vb3z,vb2xm,vb2ym,vb2zm;
  double edihedral,f1[3],f2[3],f3[3],f4[3];
  double sb1,sb2,sb3,rb1,rb3,c0,b1mag2,b1mag,b2mag2;
  double b2mag,b3mag2,b3mag,ctmp,r12c1,c1mag,r12c2;
  double c2mag,sc1,sc2,s1,s12,c,p,pd,a,a11,a22;
  double a33,a12,a13,a23,sx2,sy2,sz2;
  double s2,cx,cy,cz,cmag,dx,phi,si,siinv,sin2;

  edihedral = 0.0;
  if (eflag || vflag) ev_setup(eflag,vflag);
  else evflag = 0;

  double **x = atom->x;
  double **f = atom->f;
  int **dihedrallist = neighbor->dihedrallist;
  int ndihedrallist = neighbor->ndihedrallist;
  int nlocal = atom->nlocal;
  int newton_bond = force->newton_bond;

  for (n = 0; n < ndihedrallist; n++) {
    i1 = dihedrallist[n][0];
    i2 = dihedrallist[n][1];
    i3 = dihedrallist[n][2];
    i4 = dihedrallist[n][3];
    type = dihedrallist[n][4];

    // 1st bond

    vb1x = x[i1][0] - x[i2][0];
    vb1y = x[i1][1] - x[i2][1];
    vb1z = x[i1][2] - x[i2][2];

    // 2nd bond

    vb2x = x[i3][0] - x[i2][0];
    vb2y = x[i3][1] - x[i2][1];
    vb2z = x[i3][2] - x[i2][2];

    vb2xm = -vb2x;
    vb2ym = -vb2y;
    vb2zm = -vb2z;

    // 3rd bond

    vb3x = x[i4][0] - x[i3][0];
    vb3y = x[i4][1] - x[i3][1];
    vb3z = x[i4][2] - x[i3][2];

    // c0 calculation

    sb1 = 1.0 / (vb1x*vb1x + vb1y*vb1y + vb1z*vb1z);
    sb2 = 1.0 / (vb2x*vb2x + vb2y*vb2y + vb2z*vb2z);
    sb3 = 1.0 / (vb3x*vb3x + vb3y*vb3y + vb3z*vb3z);

    rb1 = sqrt(sb1);
    rb3 = sqrt(sb3);

    c0 = (vb1x*vb3x + vb1y*vb3y + vb1z*vb3z) * rb1*rb3;

    // 1st and 2nd angle

    b1mag2 = vb1x*vb1x + vb1y*vb1y + vb1z*vb1z;
    b1mag = sqrt(b1mag2);
    b2mag2 = vb2x*vb2x + vb2y*vb2y + vb2z*vb2z;
    b2mag = sqrt(b2mag2);
    b3mag2 = vb3x*vb3x + vb3y*vb3y + vb3z*vb3z;
    b3mag = sqrt(b3mag2);

    ctmp = vb1x*vb2x + vb1y*vb2y + vb1z*vb2z;
    r12c1 = 1.0 / (b1mag*b2mag);
    c1mag = ctmp * r12c1;

    ctmp = vb2xm*vb3x + vb2ym*vb3y + vb2zm*vb3z;
    r12c2 = 1.0 / (b2mag*b3mag);
    c2mag = ctmp * r12c2;

    // cos and sin of 2 angles and final c

    sin2 = MAX(1.0 - c1mag*c1mag,0.0);
    sc1 = sqrt(sin2);
    if (sc1 < SMALL) sc1 = SMALL;
    sc1 = 1.0/sc1;

    sin2 = MAX(1.0 - c2mag*c2mag,0.0);
    sc2 = sqrt(sin2);
    if (sc2 < SMALL) sc2 = SMALL;
    sc2 = 1.0/sc2;

    s1 = sc1 * sc1;
    s2 = sc2 * sc2;
    s12 = sc1 * sc2;
    c = (c0 + c1mag*c2mag) * s12;

    cx = vb1y*vb2z - vb1z*vb2y;
    cy = vb1z*vb2x - vb1x*vb2z;
    cz = vb1x*vb2y - vb1y*vb2x;
    cmag = sqrt(cx*cx + cy*cy + cz*cz);
    dx = (cx*vb3x + cy*vb3y + cz*vb3z)/cmag/b3mag;

    // error check

    if (c > 1.0 + TOLERANCE || c < (-1.0 - TOLERANCE)) {
      int me;
      MPI_Comm_rank(world,&me);
      if (screen) {
        char str[128];
        sprintf(str,"Dihedral problem: %d " BIGINT_FORMAT " " 
                TAGINT_FORMAT " " TAGINT_FORMAT " " 
                TAGINT_FORMAT " " TAGINT_FORMAT,
                me,update->ntimestep,
                atom->tag[i1],atom->tag[i2],atom->tag[i3],atom->tag[i4]);
        error->warning(FLERR,str,0);
        fprintf(screen,"  1st atom: %d %g %g %g\n",
                me,x[i1][0],x[i1][1],x[i1][2]);
        fprintf(screen,"  2nd atom: %d %g %g %g\n",
                me,x[i2][0],x[i2][1],x[i2][2]);
        fprintf(screen,"  3rd atom: %d %g %g %g\n",
                me,x[i3][0],x[i3][1],x[i3][2]);
        fprintf(screen,"  4th atom: %d %g %g %g\n",
                me,x[i4][0],x[i4][1],x[i4][2]);
      }
    }

    if (c > 1.0) c = 1.0;
    if (c < -1.0) c = -1.0;

    // force & energy
    // p = sum (i=1,4) k_i * (1 + (-1)**(i+1)*cos(i*phi) )
    // pd = dp/dc

    phi = acos(c);
    if (dx < 0.0) phi *= -1.0;
    si = sin(phi);
    if (fabs(si) < SMALLER) si = SMALLER;
    siinv = 1.0/si;

    p = k1[type]*(1.0 + c) + k2[type]*(1.0 - cos(2.0*phi)) +
      k3[type]*(1.0 + cos(3.0*phi)) + k4[type]*(1.0 - cos(4.0*phi)) ;
    pd = k1[type] - 2.0*k2[type]*sin(2.0*phi)*siinv +
      3.0*k3[type]*sin(3.0*phi)*siinv - 4.0*k4[type]*sin(4.0*phi)*siinv;

    if (eflag) edihedral = p;

    a = pd;
    c = c * a;
    s12 = s12 * a;
    a11 = c*sb1*s1;
    a22 = -sb2 * (2.0*c0*s12 - c*(s1+s2));
    a33 = c*sb3*s2;
    a12 = -r12c1 * (c1mag*c*s1 + c2mag*s12);
    a13 = -rb1*rb3*s12;
    a23 = r12c2 * (c2mag*c*s2 + c1mag*s12);

    sx2  = a12*vb1x + a22*vb2x + a23*vb3x;
    sy2  = a12*vb1y + a22*vb2y + a23*vb3y;
    sz2  = a12*vb1z + a22*vb2z + a23*vb3z;

    f1[0] = a11*vb1x + a12*vb2x + a13*vb3x;
    f1[1] = a11*vb1y + a12*vb2y + a13*vb3y;
    f1[2] = a11*vb1z + a12*vb2z + a13*vb3z;

    f2[0] = -sx2 - f1[0];
    f2[1] = -sy2 - f1[1];
    f2[2] = -sz2 - f1[2];

    f4[0] = a13*vb1x + a23*vb2x + a33*vb3x;
    f4[1] = a13*vb1y + a23*vb2y + a33*vb3y;
    f4[2] = a13*vb1z + a23*vb2z + a33*vb3z;

    f3[0] = sx2 - f4[0];
    f3[1] = sy2 - f4[1];
    f3[2] = sz2 - f4[2];

    // apply force to each of 4 atoms

    if (newton_bond || i1 < nlocal) {
      f[i1][0] += f1[0];
      f[i1][1] += f1[1];
      f[i1][2] += f1[2];
    }

    if (newton_bond || i2 < nlocal) {
      f[i2][0] += f2[0];
      f[i2][1] += f2[1];
      f[i2][2] += f2[2];
    }

    if (newton_bond || i3 < nlocal) {
      f[i3][0] += f3[0];
      f[i3][1] += f3[1];
      f[i3][2] += f3[2];
    }

    if (newton_bond || i4 < nlocal) {
      f[i4][0] += f4[0];
      f[i4][1] += f4[1];
      f[i4][2] += f4[2];
    }

    if (evflag)
      ev_tally(i1,i2,i3,i4,nlocal,newton_bond,edihedral,f1,f3,f4,
               vb1x,vb1y,vb1z,vb2x,vb2y,vb2z,vb3x,vb3y,vb3z);
  }
}

/* ---------------------------------------------------------------------- */

void DihedralOPLS::allocate()
{
  allocated = 1;
  int n = atom->ndihedraltypes;

  memory->create(k1,n+1,"dihedral:k1");
  memory->create(k2,n+1,"dihedral:k2");
  memory->create(k3,n+1,"dihedral:k3");
  memory->create(k4,n+1,"dihedral:k4");

  memory->create(setflag,n+1,"dihedral:setflag");
  for (int i = 1; i <= n; i++) setflag[i] = 0;
}

/* ----------------------------------------------------------------------
   set coeffs for one type
------------------------------------------------------------------------- */

void DihedralOPLS::coeff(int narg, char **arg)
{
  if (narg != 5) error->all(FLERR,"Incorrect args for dihedral coefficients");
  if (!allocated) allocate();

  int ilo,ihi;
  force->bounds(arg[0],atom->ndihedraltypes,ilo,ihi);

  double k1_one = force->numeric(FLERR,arg[1]);
  double k2_one = force->numeric(FLERR,arg[2]);
  double k3_one = force->numeric(FLERR,arg[3]);
  double k4_one = force->numeric(FLERR,arg[4]);

  // store 1/2 factor with prefactor

  int count = 0;
  for (int i = ilo; i <= ihi; i++) {
    k1[i] = 0.5*k1_one;
    k2[i] = 0.5*k2_one;
    k3[i] = 0.5*k3_one;
    k4[i] = 0.5*k4_one;
    setflag[i] = 1;
    count++;
  }

  if (count == 0) error->all(FLERR,"Incorrect args for dihedral coefficients");
}

/* ----------------------------------------------------------------------
   proc 0 writes out coeffs to restart file
------------------------------------------------------------------------- */

void DihedralOPLS::write_restart(FILE *fp)
{
  fwrite(&k1[1],sizeof(double),atom->ndihedraltypes,fp);
  fwrite(&k2[1],sizeof(double),atom->ndihedraltypes,fp);
  fwrite(&k3[1],sizeof(double),atom->ndihedraltypes,fp);
  fwrite(&k4[1],sizeof(double),atom->ndihedraltypes,fp);
}

/* ----------------------------------------------------------------------
   proc 0 reads coeffs from restart file, bcasts them
------------------------------------------------------------------------- */

void DihedralOPLS::read_restart(FILE *fp)
{
  allocate();

  if (comm->me == 0) {
    fread(&k1[1],sizeof(double),atom->ndihedraltypes,fp);
    fread(&k2[1],sizeof(double),atom->ndihedraltypes,fp);
    fread(&k3[1],sizeof(double),atom->ndihedraltypes,fp);
    fread(&k4[1],sizeof(double),atom->ndihedraltypes,fp);
  }
  MPI_Bcast(&k1[1],atom->ndihedraltypes,MPI_DOUBLE,0,world);
  MPI_Bcast(&k2[1],atom->ndihedraltypes,MPI_DOUBLE,0,world);
  MPI_Bcast(&k3[1],atom->ndihedraltypes,MPI_DOUBLE,0,world);
  MPI_Bcast(&k4[1],atom->ndihedraltypes,MPI_DOUBLE,0,world);

  for (int i = 1; i <= atom->ndihedraltypes; i++) setflag[i] = 1;
}

/* ----------------------------------------------------------------------
   proc 0 writes to data file
------------------------------------------------------------------------- */

void DihedralOPLS::write_data(FILE *fp)
{
  for (int i = 1; i <= atom->ndihedraltypes; i++)
    fprintf(fp,"%d %g %g %g %g\n",i,2.0*k1[i],2.0*k2[i],2.0*k3[i],2.0*k4[i]);
}
