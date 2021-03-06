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


#ifndef RAJAPerf_Basic_TRAP_INT_HPP
#define RAJAPerf_Basic_TRAP_INT_HPP

#include "common/KernelBase.hpp"

namespace rajaperf 
{
class RunParams;

namespace basic
{

class TRAP_INT : public KernelBase
{
public:

  TRAP_INT(const RunParams& params);

  ~TRAP_INT();

  void setUp(VariantID vid);
  void runKernel(VariantID vid); 
  void updateChecksum(VariantID vid);
  void tearDown(VariantID vid);

private:
  Real_type m_x0;
  Real_type m_xp;
  Real_type m_y;
  Real_type m_yp;
  Real_type m_h;
  Real_type m_sumx_init;

  Real_type m_sumx;
};

} // end namespace basic
} // end namespace rajaperf

#endif // closing endif for header file include guard
