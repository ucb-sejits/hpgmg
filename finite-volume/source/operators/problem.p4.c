//------------------------------------------------------------------------------------------------------------------------------
// Samuel Williams
// SWWilliams@lbl.gov
// Lawrence Berkeley National Lab
//------------------------------------------------------------------------------------------------------------------------------
void evaluateBeta(double x, double y, double z, double *B, double *Bx, double *By, double *Bz){
  double Bmin =  1.0;
  double Bmax = 10.0;
  double c2 = (Bmax-Bmin)/2; // coefficients to affect this transition
  double c1 = (Bmax+Bmin)/2;
  double c3 = 10.0;          // how sharply (B)eta transitions
  double xcenter = 0.50;
  double ycenter = 0.50;
  double zcenter = 0.50;
  // calculate distance from center of the domain (0.5,0.5,0.5)
  double r2   = pow((x-xcenter),2) +  pow((y-ycenter),2) +  pow((z-zcenter),2);
  double r2x  = 2.0*(x-xcenter);
  double r2y  = 2.0*(y-ycenter);
  double r2z  = 2.0*(z-zcenter);
//double r2xx = 2.0;
//double r2yy = 2.0;
//double r2zz = 2.0;
  double r    = pow(r2,0.5);
  double rx   = 0.5*r2x*pow(r2,-0.5);
  double ry   = 0.5*r2y*pow(r2,-0.5);
  double rz   = 0.5*r2z*pow(r2,-0.5);
//double rxx  = 0.5*r2xx*pow(r2,-0.5) - 0.25*r2x*r2x*pow(r2,-1.5);
//double ryy  = 0.5*r2yy*pow(r2,-0.5) - 0.25*r2y*r2y*pow(r2,-1.5);
//double rzz  = 0.5*r2zz*pow(r2,-0.5) - 0.25*r2z*r2z*pow(r2,-1.5);
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  *B  =           c1+c2*tanh( c3*(r-0.25) );
  *Bx = c2*c3*rx*(1-pow(tanh( c3*(r-0.25) ),2));
  *By = c2*c3*ry*(1-pow(tanh( c3*(r-0.25) ),2));
  *Bz = c2*c3*rz*(1-pow(tanh( c3*(r-0.25) ),2));
}


//------------------------------------------------------------------------------------------------------------------------------
void evaluateU(double x, double y, double z, double *U, double *Ux, double *Uy, double *Uz, double *Uxx, double *Uyy, double *Uzz, int isPeriodic){
  // should be continuous in u, u', and u''
  // v(w) = w^4 - 2w^3 + w^2 + c
  // u(x,y,z) = v(x)v(y)v(z)
  // If Periodic, then the integral of the RHS should sum to zero.
  //   Setting shift=1/30 should ensure that the integrals of X, Y, or Z should sum to zero... 
  //   That should(?) make the integrals of u,ux,uy,uz,uxx,uyy,uzz sum to zero and thus make the integral of f sum to zero
  // If dirichlet, then w(0)=w(1) = 0.0
  //   Setting shift to 0 should ensure that U(x,y,z) = 0 on boundary
  double shift = 0.0;if(isPeriodic)shift= -1.0/30.0;
  double X   =  1.0*pow(x,4) -  2.0*pow(x,3) + 1.0*pow(x,2) + shift;
  double Y   =  1.0*pow(y,4) -  2.0*pow(y,3) + 1.0*pow(y,2) + shift;
  double Z   =  1.0*pow(z,4) -  2.0*pow(z,3) + 1.0*pow(z,2) + shift;
  double Xx  =  4.0*pow(x,3) -  6.0*pow(x,2) + 2.0*x;
  double Yy  =  4.0*pow(y,3) -  6.0*pow(y,2) + 2.0*y;
  double Zz  =  4.0*pow(z,3) -  6.0*pow(z,2) + 2.0*z;
  double Xxx = 12.0*pow(x,2) - 12.0*x        + 2.0;
  double Yyy = 12.0*pow(y,2) - 12.0*y        + 2.0;
  double Zzz = 12.0*pow(z,2) - 12.0*z        + 2.0;
        *U   = X*Y*Z;
        *Ux  = Xx*Y*Z;
        *Uy  = X*Yy*Z;
        *Uz  = X*Y*Zz;
        *Uxx = Xxx*Y*Z;
        *Uyy = X*Yyy*Z;
        *Uzz = X*Y*Zzz;
}


//------------------------------------------------------------------------------------------------------------------------------
void initialize_problem(level_type * level, double hLevel, double a, double b){
  level->h = hLevel;

  int box;
  for(box=0;box<level->num_my_boxes;box++){
    memset(level->my_boxes[box].vectors[VECTOR_ALPHA ],0,level->my_boxes[box].volume*sizeof(double));
    memset(level->my_boxes[box].vectors[VECTOR_BETA_I],0,level->my_boxes[box].volume*sizeof(double));
    memset(level->my_boxes[box].vectors[VECTOR_BETA_J],0,level->my_boxes[box].volume*sizeof(double));
    memset(level->my_boxes[box].vectors[VECTOR_BETA_K],0,level->my_boxes[box].volume*sizeof(double));
    memset(level->my_boxes[box].vectors[VECTOR_UTRUE ],0,level->my_boxes[box].volume*sizeof(double));
    memset(level->my_boxes[box].vectors[VECTOR_F     ],0,level->my_boxes[box].volume*sizeof(double));
    int i,j,k;
    const int jStride = level->my_boxes[box].jStride;
    const int kStride = level->my_boxes[box].kStride;
    const int  ghosts = level->my_boxes[box].ghosts;
    const int   dim_i = level->my_boxes[box].dim;
    const int   dim_j = level->my_boxes[box].dim;
    const int   dim_k = level->my_boxes[box].dim;
    #ifdef _OPENMP
    #pragma omp parallel for private(k,j,i) collapse(3)
    #endif
    for(k=0;k<=dim_k;k++){ // include high face
    for(j=0;j<=dim_j;j++){ // include high face
    for(i=0;i<=dim_i;i++){ // include high face
      //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
      // FIX... move to quadrature version to initialize the problem.  
      // i.e. the value of an array element is the average value of the function over the cell (finite volume)
      //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
      int ijk = (i+ghosts) + (j+ghosts)*jStride + (k+ghosts)*kStride;
      double x = hLevel*( (double)(i+level->my_boxes[box].low.i) + 0.5 ); // +0.5 to get to the center of cell
      double y = hLevel*( (double)(j+level->my_boxes[box].low.j) + 0.5 );
      double z = hLevel*( (double)(k+level->my_boxes[box].low.k) + 0.5 );
      double A,B,Bx,By,Bz,Bi,Bj,Bk;
      double U,Ux,Uy,Uz,Uxx,Uyy,Uzz;
      //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
      A  = 1.0;
      B  = 1.0;
      Bx = 0.0;
      By = 0.0;
      Bz = 0.0; 
      Bi = 1.0;
      Bj = 1.0;
      Bk = 1.0;
      #ifdef STENCIL_VARIABLE_COEFFICIENT // variable coefficient problem...
      evaluateBeta(x-hLevel*0.5,y           ,z           ,&Bi,&Bx,&By,&Bz); // face-centered value of Beta for beta_i
      evaluateBeta(x           ,y-hLevel*0.5,z           ,&Bj,&Bx,&By,&Bz); // face-centered value of Beta for beta_j
      evaluateBeta(x           ,y           ,z-hLevel*0.5,&Bk,&Bx,&By,&Bz); // face-centered value of Beta for beta_k
      evaluateBeta(x           ,y           ,z           ,&B ,&Bx,&By,&Bz); // cell-centered value of Beta
      #endif
      //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
      evaluateU(x,y,z,&U,&Ux,&Uy,&Uz,&Uxx,&Uyy,&Uzz, (level->boundary_condition.type == BC_PERIODIC) );
      double F = a*A*U - b*( (Bx*Ux + By*Uy + Bz*Uz)  +  B*(Uxx + Uyy + Uzz) );
      //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
      level->my_boxes[box].vectors[VECTOR_BETA_I][ijk] = Bi;
      level->my_boxes[box].vectors[VECTOR_BETA_J][ijk] = Bj;
      level->my_boxes[box].vectors[VECTOR_BETA_K][ijk] = Bk;
      level->my_boxes[box].vectors[VECTOR_ALPHA ][ijk] = A;
      level->my_boxes[box].vectors[VECTOR_UTRUE ][ijk] = U;
      level->my_boxes[box].vectors[VECTOR_F     ][ijk] = F;
      //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
    }}}
  }

  // quick test for Poisson...
  if(level->alpha_is_zero==-1)level->alpha_is_zero = (dot(level,VECTOR_ALPHA,VECTOR_ALPHA) == 0.0);

}
//------------------------------------------------------------------------------------------------------------------------------
