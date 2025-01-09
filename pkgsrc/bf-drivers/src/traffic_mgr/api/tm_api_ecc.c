/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/


#include <traffic_mgr/traffic_mgr_types.h>
#include <tof2_regs/tof2_mem_addr.h>
#include <tof3_regs/tof3_mem_addr.h>
#include "traffic_mgr/common/tm_ctx.h"
#include "traffic_mgr/common/tm_hw_access.h"

bf_status_t bf_tm_ecc_correct_addr(bf_dev_id_t dev, uint64_t addr) {
  bf_status_t rc;
  uint64_t hi = 0, lo = 0;

  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  bf_tm_complete_ops(dev);
  rc = bf_tm_read_memory(dev, addr, &hi, &lo);
  if (rc != BF_SUCCESS) {
    TM_UNLOCK(dev, g_tm_ctx[dev]->lock);
    return rc;
  }
  rc = bf_tm_write_memory(dev, addr, 16, hi, lo);
  if (rc != BF_SUCCESS) {
    TM_UNLOCK(dev, g_tm_ctx[dev]->lock);
    return rc;
  }
  TM_UNLOCK_AND_FLUSH(dev);
  return (rc);
}

bf_status_t bf_tm_ecc_correct_wac_ppg_map(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint32_t l_addr) {
  uint64_t addr =
      tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_port_ppg_mapping(pipe,
                                                                      l_addr);
  return bf_tm_ecc_correct_addr(dev, addr);
}

bf_status_t bf_tm_ecc_correct_wac_qid_map(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint32_t l_addr) {
  uint64_t addr =
      tof2_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_qid_map(pipe, l_addr);
  return bf_tm_ecc_correct_addr(dev, addr);
}

bf_status_t bf_tm_ecc_correct_qac_qid_map(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          uint32_t l_addr) {
  uint64_t addr =
      tof2_mem_tm_tm_qac_qac_pipe_mem_csr_memory_qac_qid_mapping(pipe, l_addr);
  return bf_tm_ecc_correct_addr(dev, addr);
}

bf_status_t bf_tm_ecc_correct_sch_q_minrate(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bool dyn,
                                            uint32_t l_addr) {
  uint64_t addr;
  if (pipe == 0 || pipe == 1) {
    if (dyn) {
      addr =
          tof2_mem_tm_tm_scha_sch_pipe_mem_q_min_lb_dynamic_mem(pipe, l_addr);
    } else {
      addr = tof2_mem_tm_tm_scha_sch_pipe_mem_q_min_lb_static_mem(pipe, l_addr);
    }
  } else if (pipe == 2 || pipe == 3) {
    if (dyn) {
      addr =
          tof2_mem_tm_tm_schb_sch_pipe_mem_q_min_lb_dynamic_mem(pipe, l_addr);
    } else {
      addr = tof2_mem_tm_tm_schb_sch_pipe_mem_q_min_lb_static_mem(pipe, l_addr);
    }
  } else {
    return BF_INVALID_ARG;
  }
  return bf_tm_ecc_correct_addr(dev, addr);
}

bf_status_t bf_tm_ecc_correct_sch_q_excrate(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bool dyn,
                                            uint32_t l_addr) {
  uint64_t addr;
  if (pipe == 0 || pipe == 1) {
    if (dyn) {
      addr = tof2_mem_tm_tm_scha_sch_pipe_mem_q_exc_dynamic_mem(pipe, l_addr);
    } else {
      addr = tof2_mem_tm_tm_scha_sch_pipe_mem_q_exc_static_mem(pipe, l_addr);
    }
  } else if (pipe == 2 || pipe == 3) {
    if (dyn) {
      addr = tof2_mem_tm_tm_schb_sch_pipe_mem_q_exc_dynamic_mem(pipe, l_addr);
    } else {
      addr = tof2_mem_tm_tm_schb_sch_pipe_mem_q_exc_static_mem(pipe, l_addr);
    }
  } else {
    return BF_INVALID_ARG;
  }
  return bf_tm_ecc_correct_addr(dev, addr);
}

bf_status_t bf_tm_ecc_correct_sch_q_maxrate(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bool dyn,
                                            uint32_t l_addr) {
  uint64_t addr;
  if (pipe == 0 || pipe == 1) {
    if (dyn) {
      addr =
          tof2_mem_tm_tm_scha_sch_pipe_mem_q_max_lb_dynamic_mem(pipe, l_addr);
    } else {
      addr = tof2_mem_tm_tm_scha_sch_pipe_mem_q_max_lb_static_mem(pipe, l_addr);
    }
  } else if (pipe == 2 || pipe == 3) {
    if (dyn) {
      addr =
          tof2_mem_tm_tm_schb_sch_pipe_mem_q_max_lb_dynamic_mem(pipe, l_addr);
    } else {
      addr = tof2_mem_tm_tm_schb_sch_pipe_mem_q_max_lb_static_mem(pipe, l_addr);
    }
  } else {
    return BF_INVALID_ARG;
  }
  return bf_tm_ecc_correct_addr(dev, addr);
}

bf_status_t bf_tm_ecc_correct_sch_l1_minrate(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bool dyn,
                                             uint32_t l_addr) {
  uint64_t addr;
  if (pipe == 0 || pipe == 1) {
    if (dyn) {
      addr =
          tof2_mem_tm_tm_scha_sch_pipe_mem_l1_min_lb_dynamic_mem(pipe, l_addr);
    } else {
      addr =
          tof2_mem_tm_tm_scha_sch_pipe_mem_l1_min_lb_static_mem(pipe, l_addr);
    }
  } else if (pipe == 2 || pipe == 3) {
    if (dyn) {
      addr =
          tof2_mem_tm_tm_schb_sch_pipe_mem_l1_min_lb_dynamic_mem(pipe, l_addr);
    } else {
      addr =
          tof2_mem_tm_tm_schb_sch_pipe_mem_l1_min_lb_static_mem(pipe, l_addr);
    }
  } else {
    return BF_INVALID_ARG;
  }
  return bf_tm_ecc_correct_addr(dev, addr);
}

bf_status_t bf_tm_ecc_correct_sch_l1_excrate(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bool dyn,
                                             uint32_t l_addr) {
  uint64_t addr;
  if (pipe == 0 || pipe == 1) {
    if (dyn) {
      addr = tof2_mem_tm_tm_scha_sch_pipe_mem_l1_exc_dynamic_mem(pipe, l_addr);
    } else {
      addr = tof2_mem_tm_tm_scha_sch_pipe_mem_l1_exc_static_mem(pipe, l_addr);
    }
  } else if (pipe == 2 || pipe == 3) {
    if (dyn) {
      addr = tof2_mem_tm_tm_schb_sch_pipe_mem_l1_exc_dynamic_mem(pipe, l_addr);
    } else {
      addr = tof2_mem_tm_tm_schb_sch_pipe_mem_l1_exc_static_mem(pipe, l_addr);
    }
  } else {
    return BF_INVALID_ARG;
  }
  return bf_tm_ecc_correct_addr(dev, addr);
}

bf_status_t bf_tm_ecc_correct_sch_l1_maxrate(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             bool dyn,
                                             uint32_t l_addr) {
  uint64_t addr;
  if (pipe == 0 || pipe == 1) {
    if (dyn) {
      addr =
          tof2_mem_tm_tm_scha_sch_pipe_mem_l1_max_lb_dynamic_mem(pipe, l_addr);
    } else {
      addr =
          tof2_mem_tm_tm_scha_sch_pipe_mem_l1_max_lb_static_mem(pipe, l_addr);
    }
  } else if (pipe == 2 || pipe == 3) {
    if (dyn) {
      addr =
          tof2_mem_tm_tm_schb_sch_pipe_mem_l1_max_lb_dynamic_mem(pipe, l_addr);
    } else {
      addr =
          tof2_mem_tm_tm_schb_sch_pipe_mem_l1_max_lb_static_mem(pipe, l_addr);
    }
  } else {
    return BF_INVALID_ARG;
  }
  return bf_tm_ecc_correct_addr(dev, addr);
}

bf_status_t bf_tm_ecc_correct_sch_p_maxrate(bf_dev_id_t dev,
                                            bf_dev_pipe_t pipe,
                                            bool dyn,
                                            uint32_t l_addr) {
  uint64_t addr;
  if (pipe == 0 || pipe == 1) {
    if (dyn) {
      addr = tof2_mem_tm_tm_scha_sch_pipe_mem_port_max_lb_dynamic_mem(pipe,
                                                                      l_addr);
    } else {
      addr =
          tof2_mem_tm_tm_scha_sch_pipe_mem_port_max_lb_static_mem(pipe, l_addr);
    }
  } else if (pipe == 2 || pipe == 3) {
    if (dyn) {
      addr = tof2_mem_tm_tm_schb_sch_pipe_mem_port_max_lb_dynamic_mem(pipe,
                                                                      l_addr);
    } else {
      addr =
          tof2_mem_tm_tm_schb_sch_pipe_mem_port_max_lb_static_mem(pipe, l_addr);
    }
  } else {
    return BF_INVALID_ARG;
  }
  return bf_tm_ecc_correct_addr(dev, addr);
}
