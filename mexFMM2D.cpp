// A simplefunction.cpp that takes 3 arguments
// 1) A function handle  (kernel of Q)
// 2) An integer value M (dimension of Q)
// 3) An integer value N (number of columns of H)
// 4) A matrix U of size MxN
// 5) A vector of location x of Mx1
// 6) A vector of location y of Nx1
// and returns the output of the product QU
// Usage:
//mex('-largeArrayDims','-I/Users/yueli/Dropbox/Matlab/FastKF/Final_code/mexFMM/eigen','gatewayfunction.cpp')

#include <iostream>
#include "math.h"
#include "mex.h"
#include "matrix.h"
#include "environment.hpp"
#include "BBFMM2D.hpp"
#include <Eigen/Core>
#include "kernelfun.hpp"

using namespace Eigen;
using namespace std;
double pi 	=	4.0*atan(1);
extern void _main();

// Pass location from matlab to C
void read_location(const mxArray* x, const mxArray* y, vector<Point>& location){
    unsigned long N;
    double *xp, *yp;
    N = mxGetM(x);
    xp = mxGetPr(x);
    yp = mxGetPr(y);
    for (unsigned long i = 0; i < N; i++){
        Point new_Point;
        new_Point.x = xp[i];
        new_Point.y = yp[i];
        location.push_back(new_Point);
    }
}

void mexFunction(int nlhs,mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
    // Macros for the output and input arguments
    #define QHexact_OUT     plhs[0]
    #define QH_OUT          plhs[1]
    #define x_IN            prhs[0]
    #define y_IN            prhs[1]
    #define H_IN            prhs[2]
    #define nCheb_IN        prhs[3]

    unsigned long N;
    unsigned m;
    
    // Argument Checking:
// //   // First Argument should be a Function Handle
//     if( !mxIsClass( kernel , "function_handle")) {
//         mexErrMsgTxt("First input argument is not a function handle.");
//     }
    
//   // Second Argument is a Double Scalar
//   if (!mxIsClass(prhs[1], "double")||(mxGetM(prhs[1])>1)||(mxGetN(prhs[1])>1)) {
//     mexErrMsgTxt("Second input argument is not a double scalar.");
//   }
    // First and Second Argument are integer
    
    if( !mxIsDouble(x_IN)) {
        mexErrMsgTxt("Third input argument is not a real 2D full double array.");
    }
    if( !mxIsDouble(y_IN)) {
        mexErrMsgTxt("Third input argument is not a real 2D full double array.");
    }
    if( !mxIsDouble(H_IN)) {
        mexErrMsgTxt("Third input argument is not a real 2D full double array.");
    }
    
    //processing on input arguments
    N = mxGetM(H_IN); // get the first dimension of H
    m = mxGetN(H_IN); // get the second dimension of H
    unsigned short nChebNodes = *mxGetPr(nCheb_IN);
    cout << nChebNodes << endl;
    vector<Point> location;
    read_location(x_IN,y_IN,location);
    double *charges;
    charges = mxGetPr(H_IN);
    // Load data to local array using Eigen <Map>
    MatrixXd H = Map<MatrixXd>(charges, N, m); // Map<MatrixXd> H(charges,N,m);

    // Compute Fast matrix vector product
    // 1. Build Tree
    clock_t startBuild  = clock();
    H2_2D_Tree Atree(nChebNodes, charges, location, N, m); //Build the fmm tree
    clock_t endBuild = clock();

    double FMMTotalTimeBuild = double(endBuild-startBuild)/double(CLOCKS_PER_SEC);
    cout << endl << "Total time taken for FMM(build tree) is:" << FMMTotalTimeBuild <<endl;

    // 2.Calculateing potential
    clock_t startA = clock();
    // Create an uninitialized numeric array for dynamic memory allocation
    QH_OUT = mxCreateNumericMatrix(0, 0, mxDOUBLE_CLASS, mxREAL);     
    double *QHp;
    QHp = (double *) mxMalloc(N * m * sizeof(double));
    myKernel A;
    A.calculate_Potential(Atree, QHp);
    clock_t endA = clock();
    double FMMTotalTimeA = double(endA-startA)/double(CLOCKS_PER_SEC);
    cout << endl << "Total time taken for FMM(calculating potential) is:" << FMMTotalTimeA <<endl;
    MatrixXd QHfast = Map<MatrixXd>(QHp, N, m);

    // if #argin == 2
    /*///////////////////////////////
    // Compute exact covariance Q //
    ///////////////////////////////*/

    cout << endl << "Starting Exact computation..." << endl;
    clock_t start = clock();
    MatrixXd Q;
    A.kernel_2D(N, location, N, location, Q);// Q is initialized inside function A.kernel_2D
    // ckernel_2D(N,location,N,location,Q);
    clock_t end = clock();
    double exactAssemblyTime = double(end-start)/double(CLOCKS_PER_SEC);
    
    // Compute exact Matrix vector product
    start = clock();
    QHexact_OUT = mxCreateNumericMatrix(0, 0, mxDOUBLE_CLASS, mxREAL);
    double *QHexactp;
    QHexactp = (double *) mxMalloc(N * m * sizeof(double));
    Map<MatrixXd> QHT(QHexactp,N,m);
    QHT = Q*H;
    end = clock();
    double exactComputingTime = double(end-start)/double(CLOCKS_PER_SEC);

    cout << "the total computation time is " << exactAssemblyTime + exactComputingTime <<endl;
    
    // Compute the difference
    MatrixXd error = QHfast - QHT;
    double absoluteError = error.norm();
    double relativeError = absoluteError/QHT.norm();
    cout << "the relative difference is:" << relativeError <<endl;

    // Put the C array into the mxArray and define its dimension
    mxSetPr(QHexact_OUT,QHexactp);
    mxSetM(QHexact_OUT,N);
    mxSetN(QHexact_OUT,m);
    mxSetPr(QH_OUT,QHp);
    mxSetM(QH_OUT,N);
    mxSetN(QH_OUT,m);

    return;
}