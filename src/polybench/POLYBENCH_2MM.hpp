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


#ifndef RAJAPerf_POLYBENCH_2MM_HXX
#define RAJAPerf_POLYBENCH_2MM_HXX

#include "common/KernelBase.hpp"

namespace rajaperf 
{

class RunParams;

namespace polybench
{

class POLYBENCH_2MM : public KernelBase
{
public:

  POLYBENCH_2MM(const RunParams& params);

  ~POLYBENCH_2MM();


  void setUp(VariantID vid);
  void runKernel(VariantID vid); 
  void updateChecksum(VariantID vid);
  void tearDown(VariantID vid);

private:
  Index_type m_ni;
  Index_type m_nj;
  Index_type m_nk;
  Index_type m_nl;
  Index_type m_run_reps;
  Real_type m_alpha;
  Real_type m_beta;
  Real_ptr m_tmp;
  Real_ptr m_A;
  Real_ptr m_B;
  Real_ptr m_C;
  Real_ptr m_D; 
  Real_ptr m_DD;
};

} // end namespace polybench
} // end namespace rajaperf

#endif // closing endif for header file include guard
