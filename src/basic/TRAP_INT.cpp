//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-738930
//
// All rights reserved.
//
// This file is part of the RAJA Performance Suite.
//
// For details about use and distribution, please read raja-perfsuite/LICENSE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

///
/// TRAP_INT kernel reference implementation:
///
/// Real_type trap_int_func(Real_type x,
///                         Real_type y,
///                         Real_type xp,
///                         Real_type yp)
/// {
///    Real_type denom = (x - xp)*(x - xp) + (y - yp)*(y - yp);
///    denom = 1.0/sqrt(denom);
///    return denom;
/// }
///
/// for (Index_type i = ibegin; i < iend; ++i ) {
///    Real_type x = x0 + i*h;
///    sumx += trap_int_func(x, y, xp, yp);
/// }
///

#include "TRAP_INT.hpp"

#include "common/DataUtils.hpp"

#include "RAJA/RAJA.hpp"
#include "RAJA/policy/cuda.hpp"

#include <iostream>

namespace rajaperf 
{
namespace basic
{

//
// Function used in TRAP_INT loop.
//
RAJA_INLINE
RAJA_HOST_DEVICE
Real_type trap_int_func(Real_type x,
                        Real_type y,
                        Real_type xp,
                        Real_type yp)
{
   Real_type denom = (x - xp)*(x - xp) + (y - yp)*(y - yp);
   denom = 1.0/sqrt(denom);
   return denom;
}


#define TRAP_INT_DATA \
  Real_type x0 = m_x0; \
  Real_type xp = m_xp; \
  Real_type y = m_y; \
  Real_type yp = m_yp; \
  Real_type h = m_h;

#define TRAP_INT_BODY \
  Real_type x = x0 + i*h; \
  sumx += trap_int_func(x, y, xp, yp);


#if defined(RAJA_ENABLE_CUDA)

  //
  // Define thread block size for CUDA execution
  //
  const size_t block_size = 256;


#define TRAP_INT_DATA_SETUP_CUDA // nothing to do here...

#define TRAP_INT_DATA_TEARDOWN_CUDA // nothing to do here...

__global__ void trapint(Real_type x0, Real_type xp,
                        Real_type y, Real_type yp, 
                        Real_type h, 
                        Real_ptr sumx,
                        Index_type iend)
{
  extern __shared__ Real_type psumx[ ];

  Index_type i = blockIdx.x * blockDim.x + threadIdx.x;

  psumx[ threadIdx.x ] = 0.0;
  for ( ; i < iend ; i += gridDim.x * blockDim.x ) {
    Real_type x = x0 + i*h;
    Real_type val = trap_int_func(x, y, xp, yp);
    psumx[ threadIdx.x ] += val;
  }
  __syncthreads();

  for ( i = blockDim.x / 2; i > 0; i /= 2 ) {
    if ( threadIdx.x < i ) {
      psumx[ threadIdx.x ] += psumx[ threadIdx.x + i ];
    }
     __syncthreads();
  }

#if 1 // serialized access to shared data;
  if ( threadIdx.x == 0 ) {
    RAJA::atomic::atomicAdd<RAJA::atomic::cuda_atomic>( sumx, psumx[ 0 ] );
  }
#else // this doesn't work due to data races
  if ( threadIdx.x == 0 ) {
    *sumx += psumx[ 0 ];
  }
#endif

}

#endif // if defined(RAJA_ENABLE_CUDA)


TRAP_INT::TRAP_INT(const RunParams& params)
  : KernelBase(rajaperf::Basic_TRAP_INT, params)
{
   setDefaultSize(100000);
   setDefaultReps(2000);
}

TRAP_INT::~TRAP_INT() 
{
}

void TRAP_INT::setUp(VariantID vid)
{
  Real_type xn; 
  initData(xn, vid);

  initData(m_x0, vid);
  initData(m_xp, vid);
  initData(m_y,  vid);
  initData(m_yp, vid);

  m_h = xn - m_x0;

  m_sumx_init = 0.5*( trap_int_func(m_x0, m_y, m_xp, m_yp) +
                      trap_int_func(xn, m_y, m_xp, m_yp) );  

  m_sumx = 0;
}

void TRAP_INT::runKernel(VariantID vid)
{
  const Index_type run_reps = getRunReps();
  const Index_type ibegin = 0;
  const Index_type iend = getRunSize();

  switch ( vid ) {

    case Base_Seq : {

      TRAP_INT_DATA;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        Real_type sumx = m_sumx_init;

        for (Index_type i = ibegin; i < iend; ++i ) {
          TRAP_INT_BODY;
        }

        m_sumx += sumx * h;

      }
      stopTimer();

      break;
    }

    case RAJA_Seq : {

      TRAP_INT_DATA;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::ReduceSum<RAJA::seq_reduce, Real_type> sumx(m_sumx_init);

        RAJA::forall<RAJA::seq_exec>(
          RAJA::RangeSegment(ibegin, iend), [=](int i) {
          TRAP_INT_BODY;
        });

        m_sumx += static_cast<Real_type>(sumx.get()) * h;

      }
      stopTimer();

      break;
    }

#if defined(RAJA_ENABLE_OPENMP)
    case Base_OpenMP : {

      TRAP_INT_DATA;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        Real_type sumx = m_sumx_init;

        #pragma omp parallel for reduction(+:sumx)
        for (Index_type i = ibegin; i < iend; ++i ) {
          TRAP_INT_BODY;
        }

        m_sumx += sumx * h;

      }
      stopTimer();

      break;
    }

    case RAJA_OpenMP : {

      TRAP_INT_DATA;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::ReduceSum<RAJA::omp_reduce, Real_type> sumx(m_sumx_init);

        RAJA::forall<RAJA::omp_parallel_for_exec>(
          RAJA::RangeSegment(ibegin, iend), [=](Index_type i) {
          TRAP_INT_BODY;
        });

        m_sumx += static_cast<Real_type>(sumx.get()) * h;

      }
      stopTimer();

      break;
    }

#if defined(RAJA_ENABLE_TARGET_OPENMP)

#define NUMTEAMS 128

    case Base_OpenMPTarget : {

      TRAP_INT_DATA;

      #pragma omp target data map(to:x0,xp,y,yp,h)

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        Real_type sumx = m_sumx_init;
        #pragma omp target teams distribute parallel for map(tofrom: sumx) reduction(+:sumx) \
                           num_teams(NUMTEAMS) schedule(static, 1) 
        
        for (Index_type i = ibegin; i < iend; ++i ) {
          TRAP_INT_BODY;
        }
        m_sumx += sumx * h;

      }
      stopTimer();

      #pragma omp target exit data map(delete: x0,xp,y,yp,h) 

      break;
    }

    case RAJA_OpenMPTarget : {

      TRAP_INT_DATA;

      #pragma omp target data map(to:x0,xp,y,yp,h)
      {

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::ReduceSum<RAJA::omp_target_reduce<NUMTEAMS>, Real_type> sumx(m_sumx_init);

        RAJA::forall<RAJA::omp_target_parallel_for_exec<NUMTEAMS>>(
            RAJA::RangeSegment(ibegin, iend), [=](Index_type i) {
          TRAP_INT_BODY;
        });

        m_sumx += static_cast<Real_type>(sumx.get()) * h;

      }
      stopTimer();

      #pragma omp target exit data map(delete: x0,xp,y,yp,h) 

      }

      break;
    }
#endif //RAJA_ENABLE_TARGET_OPENMP
#endif //RAJA_ENABLE_OMP       

#if defined(RAJA_ENABLE_CUDA)
    case Base_CUDA : {

      TRAP_INT_DATA;

      Real_ptr sumx;
      allocAndInitCudaDeviceData(sumx, &m_sumx_init, 1);

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        initCudaDeviceData(sumx, &m_sumx_init, 1); 

        const size_t grid_size = RAJA_DIVIDE_CEILING_INT(iend, block_size);
        trapint<<<grid_size, block_size, 
                  sizeof(Real_type)*block_size>>>(x0, xp,
                                                  y, yp,
                                                  h,
                                                  sumx,
                                                  iend);

        Real_type lsumx;
        Real_ptr plsumx = &lsumx;
        getCudaDeviceData(plsumx, sumx, 1);
        m_sumx += lsumx * h;

      }
      stopTimer();

      deallocCudaDeviceData(sumx);

      break;
    }

    case RAJA_CUDA : {

      TRAP_INT_DATA;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::ReduceSum<RAJA::cuda_reduce<block_size>, Real_type> sumx(m_sumx_init);

        RAJA::forall< RAJA::cuda_exec<block_size, true /*async*/> >(
          RAJA::RangeSegment(ibegin, iend), [=] __device__ (Index_type i) {
          TRAP_INT_BODY;
        });

        m_sumx += static_cast<Real_type>(sumx.get()) * h;

      }
      stopTimer();

      break;
    }
#endif


    default : {
      std::cout << "\n  TRAP_INT : Unknown variant id = " << vid << std::endl;
    }

  }

}

void TRAP_INT::updateChecksum(VariantID vid)
{
  checksum[vid] += (m_sumx + 0.00123) / (m_sumx - 0.00123);
}

void TRAP_INT::tearDown(VariantID vid)
{
  (void) vid;
}

} // end namespace basic
} // end namespace rajaperf
