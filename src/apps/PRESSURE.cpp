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
/// PRESSURE kernel reference implementation:
///
/// for (Index_type i = ibegin; i < iend; ++i ) {
///   bvc[i] = cls * (compression[i] + 1.0);
/// }
///
/// for (Index_type i = ibegin; i < iend; ++i ) {
///   p_new[i] = bvc[i] * e_old[i] ; 
///   if ( fabs(p_new[i]) <  p_cut ) p_new[i] = 0.0 ; 
///   if ( vnewc[i] >= eosvmax ) p_new[i] = 0.0 ; 
///   if ( p_new[i]  <  pmin ) p_new[i]   = pmin ;
/// }
///

#include "PRESSURE.hpp"

#include "common/DataUtils.hpp"

#include "RAJA/RAJA.hpp"

#include <iostream>

namespace rajaperf 
{
namespace apps
{

#define PRESSURE_DATA \
  ResReal_ptr compression = m_compression; \
  ResReal_ptr bvc = m_bvc; \
  ResReal_ptr p_new = m_p_new; \
  ResReal_ptr e_old  = m_e_old; \
  ResReal_ptr vnewc  = m_vnewc; \
  const Real_type cls = m_cls; \
  const Real_type p_cut = m_p_cut; \
  const Real_type pmin = m_pmin; \
  const Real_type eosvmax = m_eosvmax; 
   

#define PRESSURE_BODY1 \
  bvc[i] = cls * (compression[i] + 1.0);

#define PRESSURE_BODY2 \
  p_new[i] = bvc[i] * e_old[i] ; \
  if ( fabs(p_new[i]) <  p_cut ) p_new[i] = 0.0 ; \
  if ( vnewc[i] >= eosvmax ) p_new[i] = 0.0 ; \
  if ( p_new[i]  <  pmin ) p_new[i]   = pmin ;


#if defined(RAJA_ENABLE_CUDA)

  //
  // Define thread block size for CUDA execution
  //
  const size_t block_size = 256;


#define PRESSURE_DATA_SETUP_CUDA \
  Real_ptr compression; \
  Real_ptr bvc; \
  Real_ptr p_new; \
  Real_ptr e_old; \
  Real_ptr vnewc; \
  const Real_type cls = m_cls; \
  const Real_type p_cut = m_p_cut; \
  const Real_type pmin = m_pmin; \
  const Real_type eosvmax = m_eosvmax; \
\
  allocAndInitCudaDeviceData(compression, m_compression, iend); \
  allocAndInitCudaDeviceData(bvc, m_bvc, iend); \
  allocAndInitCudaDeviceData(p_new, m_p_new, iend); \
  allocAndInitCudaDeviceData(e_old, m_e_old, iend); \
  allocAndInitCudaDeviceData(vnewc, m_vnewc, iend);

#define PRESSURE_DATA_TEARDOWN_CUDA \
  getCudaDeviceData(m_p_new, p_new, iend); \
  deallocCudaDeviceData(compression); \
  deallocCudaDeviceData(bvc); \
  deallocCudaDeviceData(p_new); \
  deallocCudaDeviceData(e_old); \
  deallocCudaDeviceData(vnewc);

__global__ void pressurecalc1(Real_ptr bvc, Real_ptr compression,
                              const Real_type cls,
                              Index_type iend)
{
   Index_type i = blockIdx.x * blockDim.x + threadIdx.x;
   if (i < iend) {
     PRESSURE_BODY1;
   }
}

__global__ void pressurecalc2(Real_ptr p_new, Real_ptr bvc, Real_ptr e_old,
                              Real_ptr vnewc,
                              const Real_type p_cut, const Real_type eosvmax,
                              const Real_type pmin,
                              Index_type iend)
{
   Index_type i = blockIdx.x * blockDim.x + threadIdx.x;
   if (i < iend) {
     PRESSURE_BODY2;
   }
}

#endif // if defined(RAJA_ENABLE_CUDA)


PRESSURE::PRESSURE(const RunParams& params)
  : KernelBase(rajaperf::Apps_PRESSURE, params)
{
  setDefaultSize(100000);
  setDefaultReps(7000);
}

PRESSURE::~PRESSURE() 
{
}

void PRESSURE::setUp(VariantID vid)
{
  allocAndInitData(m_compression, getRunSize(), vid);
  allocAndInitData(m_bvc, getRunSize(), vid);
  allocAndInitData(m_p_new, getRunSize(), vid);
  allocAndInitData(m_e_old, getRunSize(), vid);
  allocAndInitData(m_vnewc, getRunSize(), vid);
  
  initData(m_cls);
  initData(m_p_cut);
  initData(m_pmin);
  initData(m_eosvmax);
}

void PRESSURE::runKernel(VariantID vid)
{
  const Index_type run_reps = getRunReps();
  const Index_type ibegin = 0;
  const Index_type iend = getRunSize();

  switch ( vid ) {

    case Base_Seq : {

      PRESSURE_DATA;
  
      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        for (Index_type i = ibegin; i < iend; ++i ) {
          PRESSURE_BODY1;
        }

        for (Index_type i = ibegin; i < iend; ++i ) {
          PRESSURE_BODY2;
        }

      }
      stopTimer();

      break;
    } 

    case RAJA_Seq : {

      PRESSURE_DATA;
 
      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::forall<RAJA::simd_exec>(
          RAJA::RangeSegment(ibegin, iend), [=](int i) {
          PRESSURE_BODY1;
        }); 

        RAJA::forall<RAJA::simd_exec>(
          RAJA::RangeSegment(ibegin, iend), [=](int i) {
          PRESSURE_BODY2;
        }); 

      }
      stopTimer(); 

      break;
    }

#if defined(RAJA_ENABLE_OPENMP)      
    case Base_OpenMP : {

//
// NOTE: This kernel should be written to have an OpenMP parallel 
//       region around it and then use an OpenMP for-nowait for
//       each loop inside it. We currently don't have a clean way to
//       do this in RAJA. So, the base OpenMP variant is coded the
//       way it is to be able to do an "apples to apples" comparison.
//
//       This will be changed in the future when the required feature 
//       is added to RAJA.
//

      PRESSURE_DATA;
      
      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {
    
        #pragma omp parallel for schedule(static)
        for (Index_type i = ibegin; i < iend; ++i ) {
          PRESSURE_BODY1;
        }

        #pragma omp parallel for schedule(static)
        for (Index_type i = ibegin; i < iend; ++i ) {
          PRESSURE_BODY2;
        }

      }
      stopTimer();

      break;
    }

    case RAJA_OpenMP : {

      PRESSURE_DATA;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::forall<RAJA::omp_parallel_for_exec>(
          RAJA::RangeSegment(ibegin, iend), [=](int i) {
          PRESSURE_BODY1;
        });

        RAJA::forall<RAJA::omp_parallel_for_exec>(
          RAJA::RangeSegment(ibegin, iend), [=](int i) {
          PRESSURE_BODY2;
        });

      }
      stopTimer();

      break;
    }

#if defined(RAJA_ENABLE_TARGET_OPENMP)

#define NUMTEAMS 128

    case Base_OpenMPTarget : {

      PRESSURE_DATA;

      int n=getRunSize();
      #pragma omp target enter data map(to:compression[0:n], bvc[0:n], p_new[0:n], e_old[0:n], \
       vnewc[0:n], cls, p_cut, pmin, eosvmax )

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        #pragma omp target teams distribute parallel for num_teams(NUMTEAMS) schedule(static, 1) 
        for (Index_type i = ibegin; i < iend; ++i ) {
          PRESSURE_BODY1;
        }

        #pragma omp target teams distribute parallel for num_teams(NUMTEAMS) schedule(static, 1) 
        for (Index_type i = ibegin; i < iend; ++i ) {
          PRESSURE_BODY2;
        }

      }
      stopTimer();

      #pragma omp target exit data map(from:p_new[0:n]) map(delete:compression[0:n],bvc[0:n],e_old[0:n],vnewc[0:n],cls,p_cut,pmin,eosvmax)

      break;
    }

    case RAJA_OpenMPTarget : {

      PRESSURE_DATA;

      int n=getRunSize();
      #pragma omp target enter data map(to:compression[0:n], bvc[0:n], p_new[0:n], e_old[0:n], \
       vnewc[0:n], cls, p_cut, pmin, eosvmax )

      startTimer();
      #pragma omp target data use_device_ptr(compression, bvc, p_new, e_old,vnewc)
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

        RAJA::forall<RAJA::omp_target_parallel_for_exec<NUMTEAMS>>(
            RAJA::RangeSegment(ibegin, iend), [=](int i) {
          PRESSURE_BODY1;
        });

        RAJA::forall<RAJA::omp_target_parallel_for_exec<NUMTEAMS>>(
            RAJA::RangeSegment(ibegin, iend), [=](int i) {
          PRESSURE_BODY2;
        });

      }
      stopTimer();

      #pragma omp target exit data map(from:p_new[0:n]) map(delete:compression[0:n],bvc[0:n],e_old[0:n],vnewc[0:n],cls,p_cut,pmin,eosvmax)

      break;
    }
#endif
#endif                             

#if defined(RAJA_ENABLE_CUDA)
    case Base_CUDA : {

      PRESSURE_DATA_SETUP_CUDA;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

         const size_t grid_size = RAJA_DIVIDE_CEILING_INT(iend, block_size);

         pressurecalc1<<<grid_size, block_size>>>( bvc, compression,
                                                   cls,
                                                   iend );

         pressurecalc2<<<grid_size, block_size>>>( p_new, bvc, e_old,
                                                   vnewc,
                                                   p_cut, eosvmax, pmin,
                                                   iend );

      }
      stopTimer();

      PRESSURE_DATA_TEARDOWN_CUDA;

      break;
    }

    case RAJA_CUDA : {

      PRESSURE_DATA_SETUP_CUDA;

      startTimer();
      for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

         RAJA::forall< RAJA::cuda_exec<block_size, true /*async*/> >(
           RAJA::RangeSegment(ibegin, iend), [=] __device__ (Index_type i) {
           PRESSURE_BODY1;
         });

         RAJA::forall< RAJA::cuda_exec<block_size, true /*async*/> >(
           RAJA::RangeSegment(ibegin, iend), [=] __device__ (Index_type i) {
           PRESSURE_BODY2;
         });

      }
      stopTimer();

      PRESSURE_DATA_TEARDOWN_CUDA;

      break;
    }
#endif

    default : {
      std::cout << "\n  PRESSURE : Unknown variant id = " << vid << std::endl;
    }

  }
}

void PRESSURE::updateChecksum(VariantID vid)
{
  checksum[vid] += calcChecksum(m_p_new, getRunSize());
}

void PRESSURE::tearDown(VariantID vid)
{
  (void) vid;
 
  deallocData(m_compression);
  deallocData(m_bvc);
  deallocData(m_p_new);
  deallocData(m_e_old);
  deallocData(m_vnewc);
}

} // end namespace apps
} // end namespace rajaperf
