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


/*!
 * @file pipe_mgr_drv_intf.h
 * @date
 *
 * Definitions for pipe_mgr's interface to the Low Level Driver.
 */

#ifndef _PIPE_MGR_DRV_INTF_H
#define _PIPE_MGR_DRV_INTF_H

/* Standard includes */
#include <sys/types.h>
#include <inttypes.h>

/* Module includes */
#include <dvm/bf_drv_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include "pipe_mgr_bitmap.h"

///////////////////////////////////////////////////

/*! An individual DMA buffer */
typedef struct pipe_mgr_drv_buf_t pipe_mgr_drv_buf_t;
struct pipe_mgr_drv_buf_t {
  bf_sys_dma_pool_handle_t
      pool;      /*!< Handle of the pool to which this buffer belongs */
  uint8_t *addr; /*!< Virtual address of the buffer, use when reading/writing */
  bf_phys_addr_t phys_addr; /*!< Physical address of the buffer */
  uint32_t size;            /*!< Buffer size in bytes. */
  uint32_t used;      /*!< How many bytes have written to the buffer so far. */
  uint8_t devId;      /*!< Which device the buffer is associated with. */
  uint8_t subdev;     /*!< Which subdevice the buffer is associated with. */
  uint8_t pipeMask;   /*!< Which pipeline(s) the buffer is targeting. */
  uint8_t buf_pushed; /*!< Which pipeline(s) the buffer has been pushed */
  uint64_t msgId;     /*!< Which message the buffer is associated with. */
  pipe_mgr_drv_buf_t *next; /*!< Next buffer in this list (free/used). */
  pipe_mgr_drv_buf_t *prev; /*!< Next buffer in this list (free/used). */
};

pipe_status_t pipe_mgr_drv_init(void);
pipe_status_t pipe_mgr_drv_init_dev(uint8_t devId, bf_dma_info_t *dma_info);

/**
 * @brief Blocking call to read a single 32 bit register.
 * @param sess The session to execute the read in.
 * @param devId The logical device to read from.
 * @param addr The address to read (must be in the PCIe register space).
 * @param val A pointer to the address where the register value will be written.
 * @return Returns @c PIPE_INVALID_ARG if any input parameters are invalid.
 *         Returns @c PIPE_SUCCESS otherwise.
 */
pipe_status_t pipe_mgr_drv_reg_rd(pipe_sess_hdl_t *sess,
                                  uint8_t devId,
                                  uint32_t addr,
                                  uint32_t *val);

/**
 * @brief Blocking call to read a single 32 bit register from a given sub
 * device.
 * @param sess The session to execute the read in.
 * @param dev_id The logical device to read from.
 * @param subdev The logical sub device to read from.
 * @param addr The address to read (must be in the PCIe register space).
 * @param val A pointer to the address where the register value will be written.
 * @return Returns @c PIPE_INVALID_ARG if any input parameters are invalid.
 *         Returns @c PIPE_SUCCESS otherwise.
 */
pipe_status_t pipe_mgr_drv_subdev_reg_rd(pipe_sess_hdl_t *sess,
                                         bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev,
                                         uint32_t addr,
                                         uint32_t *val);

/**
 * Defination of the read block operation's completion callback.
 * The arguments are as follows:
 * A buffer containing the data read.
 * An "is errored" indication
 * A pointer to a caller suplied argument.
 */
typedef void (*pipe_mgr_drv_rd_blk_cb)(pipe_mgr_drv_buf_t *,
                                       uint32_t offset,
                                       uint32_t entryCount,
                                       bool hadError,
                                       void *userData);
/**
 * Read a contigious block of data from the hardware.
 * @param sess The session to execute the read in.
 * @param devId The logical device to read from.
 * @param memoryWidth The width in bytes of each entry; must be 4, 8, or 16.
 * @param entryCount The number of entries to read.
 * @param addr_step Value to increment the address by at each write, generally
 *                  1 for memories and 4 for registers.
 * @param addr The address to read.
 * @param cb_func A callback function to execute when the data is ready.
 * @param usrData A caller supplied argument to the callback function.
 * @return Returns @c PIPE_INVALID_ARG if any input parameters are invalid.
 *         Returns @c PIPE_ALREADY_EXISTS if a read block operation is pending
 *         for the session already.
 *         Returns @c PIPE_TRY_AGAIN if no DMA buffers are currently available.
 *         Returns @c PIPE_COMM_FAIL if the push to LLD fails.
 *         Returns @c PIPE_SUCCESS otherwise.
 */
pipe_status_t pipe_mgr_drv_blk_rd(pipe_sess_hdl_t *sess,
                                  uint8_t devId,
                                  uint8_t memoryWidth,
                                  uint32_t entryCount,
                                  int addr_step,
                                  uint64_t addr,
                                  pipe_mgr_drv_rd_blk_cb cb_func,
                                  void *usrData);

/**
 * Write a contigious block of data to the hardware.  The caller should prepare
 * the buffer of data ahead of time and simply pass that buffer into this
 * function with the @p buf paramater.  The buffer should be allocated using
 * the @c pipe_mgr_drv_buf_alloc() function.
 * @param sess The session to execute the write in.
 * @param memoryWidth The width in bytes of each entry; must be 4, 8, or 16.
 * @param entryCount The number of entries to write.
 * @param addr_step Value to increment the address by at each write, generally
 *                  1 for memories and 4 for registers.
 * @param addr The first address to write.
 * @param mask Bit map of logical pipe ids.
 * @param buf A DMA buffer, previously allocated with @c pipe_mgr_drv_buf_alloc,
 *            containing the data to write.
 * @return Returns @c PIPE_INVALID_ARG if any input parameters are invalid.
 *         Returns @c PIPE_NO_SYS_RESOURCES if memory to store the operation's
 *         state cannot be allocated.
 *         Returns @c PIPE_COMM_FAIL if the push to LLD fails.
 *         Returns @c PIPE_SUCCESS otherwise.
 */
pipe_status_t pipe_mgr_drv_blk_wr(pipe_sess_hdl_t *sess,
                                  uint8_t entry_sz,
                                  uint32_t entryCount,
                                  uint8_t addr_step,
                                  uint64_t addr,
                                  int log_pipe_mask,
                                  pipe_mgr_drv_buf_t *buf);

/* Pass data for a single entry and write it to multiple locations. */
pipe_status_t pipe_mgr_drv_blk_wr_data(pipe_sess_hdl_t *sess,
                                       rmt_dev_info_t *dev_info,
                                       uint8_t entry_sz,
                                       uint32_t entryCount,
                                       uint8_t addr_step,
                                       uint64_t addr,
                                       int pipe_mask,
                                       uint8_t *one_entry);

pipe_status_t pipe_mgr_drv_rd_blk_cmplt_all(pipe_sess_hdl_t sess,
                                            bf_dev_id_t dev_id);
pipe_status_t pipe_mgr_drv_wr_blk_cmplt_all(pipe_sess_hdl_t sess,
                                            bf_dev_id_t dev_id);

/**
 * Push a register write to LLD.  Once LLD has executed the write and notified
 * Pipeline Manager the callback @p cb_func will be called with the provided
 * argument @p usrData.
 * Note that this interface provides access to direct mapped registers.
 * @param sess The session to execute the write in.
 * @param devId The device to issue the write to.
 * @param addr The address to write.
 * @param val The value to write.
 * @return Returns @c PIPE_INVALID_ARG if the input paramaters are invalid.
 *         Returns @c PIPE_NO_SYS_RESOURCES if memory cannot be allocated to
 *         store state for the operation.
 *         Returns @c PIPE_TRY_AGAIN if there is no space on the FIFO to LLD.
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_reg_wr(pipe_sess_hdl_t *sess,
                                  uint8_t devId,
                                  bf_subdev_id_t subdev,
                                  uint32_t addr,
                                  uint32_t val);
pipe_status_t pipe_mgr_sess_ilist_add_register_write(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t dev_id,
                                                     bf_subdev_id_t subdev_id,
                                                     uint32_t reg_addr,
                                                     uint32_t reg_data);
pipe_status_t pipe_mgr_sess_pipes_ilist_add_register_write(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_bitmap_t *pipe_bmp,
    uint8_t stage_id,
    uint32_t reg_addr,
    uint32_t reg_data);
pipe_status_t pipe_mgr_ilist_add_register_write(bf_dev_id_t dev_id,
                                                bf_subdev_id_t subdev_id,
                                                uint32_t reg_addr,
                                                uint32_t reg_data);
pipe_status_t pipe_mgr_write_register(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      uint32_t reg_addr,
                                      uint32_t reg_data);

/**
 * Defination of the Instruction List operation's completion callback.
 * The arguments are as follows:
 * A pointer to a caller suplied argument.
 * An "is errored" indication
 */
typedef void (*pipe_mgr_drv_ilist_cb)(void *, bool);

/* Cookie, device, pipe, byte offset, buffer ptr, buffer byte len, last,
 * user-cb-safe*/
typedef void (*pipe_mgr_drv_rd_ilist_cb)(void *,
                                         bf_dev_id_t,
                                         bf_dev_pipe_t,
                                         uint32_t,
                                         uint8_t *,
                                         uint32_t,
                                         bool,
                                         bool);
/**
 * Add an instruction to the sessions instruction list, this API is similar
 * to the pipe_mgr_drv_ilist_add() except that it takes the instruction
 * and data as separate arguments
 * @param sess The client's session.
 * @param devId The device to issue the instruction to.
 * @param pipeId The pipeline to issue the instruction to @c BF_DEV_PIPE_ALL for
 * all.
 * @param stage The pipeline stage to issue the instruction to.
 * @param instr The formatted instruction.
 * @param instr_len The byte length of the formatted instruction.  Legal lengths
 * are
 *        4, 8, 12, 16, or 20 bytes.
 * @param data_len The formatted data.
 * @param data_len The byte length of the formatted instruction.  Legal lengths
 * are
 *        4, 8, 16 bytes.
 * @return Returns @c PIPE_INVALID_ARG if the input paramaters are invalid.
 *         Returns @c PIPE_NO_SYS_RESOURCES if there is no pending instruction
 *         list and memory allocation for a new list state fails.
 *         Returns @c PIPE_NO_SPACE if the number of instructions between a
 *         lock and unlock will not fit in a single DMA buffer.
 *         Returns @c PIPE_TRY_AGAIN if there are no free DMA buffers.
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_ilist_add_2(pipe_sess_hdl_t *sess,
                                       rmt_dev_info_t *dev_info,
                                       pipe_bitmap_t *pipe_bmp,
                                       uint8_t stage,
                                       uint8_t *instr,
                                       uint8_t instr_len,
                                       uint8_t *data,
                                       uint8_t data_len);
/**
 * Add an instruction to the sessions instruction list, instructions should be
 * formatted for execution prior to calling this API.  Note that there is a
 * single list of outstanding instructions per session, calls to this API
 * continue to add instructions to it.  Only after calling
 * @c pipe_mgr_drv_ilist_push() or @c pipe_mgr_drv_ilist_abort() will a new
 * list begin.  As operations are added to the list via calls to this function,
 * DMA buffers will be reserved automatically.
 * @param sess The client's session.
 * @param devId The device to issue the instruction to.
 * @param pipeId The pipeline to issue the instruction to @c BF_DEV_PIPE_ALL for
 * all.
 * @param stage The pipeline stage to issue the instruction to.
 * @param data The formatted instruction.
 * @param len The byte length of the formatted instruction.  Legal lengths are
 *        4, 8, 12, 16, or 20 bytes.
 * @return Returns @c PIPE_INVALID_ARG if the input paramaters are invalid.
 *         Returns @c PIPE_NO_SYS_RESOURCES if there is no pending instruction
 *         list and memory allocation for a new list state fails.
 *         Returns @c PIPE_NO_SPACE if the number of instructions between a
 *         lock and unlock will not fit in a single DMA buffer.
 *         Returns @c PIPE_TRY_AGAIN if there are no free DMA buffers.
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_ilist_add(pipe_sess_hdl_t *sess,
                                     rmt_dev_info_t *dev_info,
                                     pipe_bitmap_t *pipe_bmp,
                                     uint8_t stage,
                                     uint8_t *data,
                                     uint8_t len);

/**
 * Push an instruction list to LLD.
 * @param sess The client's session.
 * @param cb_func A callback function to execute once the hardware notifies
 *        that all operations in the list are complete or @c NULL.
 * @param usrData An argument to the callback function or @c NULL.
 * @return Returns @c PIPE_INVALID_ARG if the input paramaters are invalid.
 *         Returns @c PIPE_OBJ_NOT_FOUND if the list doesn't have any
 *         instructions added to it (i.e. no data to push).
 *         Returns @c PIPE_TRY_AGAIN if the FIFOs to LLD do not have space to
 *         hold all buffers of the list.
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_ilist_push(pipe_sess_hdl_t *sess,
                                      pipe_mgr_drv_ilist_cb cb_func,
                                      void *usrData);

/* Push ilist after fast-reconfig */
pipe_status_t pipe_mgr_drv_reconfig_ilist_push(pipe_sess_hdl_t *sess,
                                               bf_dev_id_t dev_id);

/**
 * Aborts an instructions list discarding all instructions in it and freeing
 * all resources.  Note this cannot be used to abort a list already pushed to
 * the LLD via @c pipe_mgr_drv_ilist_push.
 * @param sess The client's session.
 * @return Returns @c PIPE_INVALID_ARG if the input paramaters are invalid.
 *         Returns @c PIPE_OBJ_NOT_FOUND if the list is empty.
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_ilist_abort(pipe_sess_hdl_t *sess);

pipe_status_t pipe_mgr_drv_ilist_chkpt(pipe_sess_hdl_t shdl);
pipe_status_t pipe_mgr_drv_ilist_rollback(pipe_sess_hdl_t shdl);

pipe_status_t pipe_mgr_drv_i_list_cmplt_all(pipe_sess_hdl_t *sess);

pipe_status_t pipe_mgr_drv_ilist_rd_add(pipe_sess_hdl_t *sess,
                                        rmt_dev_info_t *dev_info,
                                        bf_dev_pipe_t pipe,
                                        uint8_t stage,
                                        uint8_t *data,
                                        uint8_t len);
pipe_status_t pipe_mgr_drv_ilist_rd_push(pipe_sess_hdl_t *sess,
                                         pipe_mgr_drv_rd_ilist_cb cb_func,
                                         void *usrData);
pipe_status_t pipe_mgr_drv_ilist_rd_abort(pipe_sess_hdl_t *sess);
pipe_status_t pipe_mgr_drv_ilist_rd_cmplt_all(pipe_sess_hdl_t *sess);

/**
 * Free a DMA buffer previously allocated with @c pipe_mgr_drv_buf_alloc().
 * @param buf The buffer to free
 */
void pipe_mgr_drv_buf_free(pipe_mgr_drv_buf_t *buf);

/**
 * Service the pipe related DRs
 * @param dev_id Device id
 */
void pipe_mgr_drv_service_drs(bf_dev_id_t dev_id);

/**
 * Service the instruction list completion DR
 * @param dev_id Device id
 * @return Returns @c PIPE_LLD_FAILED if the servicing fails
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_service_ilist_drs(bf_dev_id_t dev_id);

/**
 * Service the learn DR
 * @param dev_id Device id
 * @return Returns @c PIPE_LLD_FAILED if the servicing fails
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_service_learn_drs(bf_dev_id_t dev_id);

/**
 * Service the idle time DR
 * @param dev_id Device id
 * @return Returns @c PIPE_LLD_FAILED if the servicing fails
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_service_idle_time_drs(bf_dev_id_t dev_id);

/**
 * Service the read block completion DR
 * @param dev_id Device id
 * @return Returns @c PIPE_LLD_FAILED if the servicing fails
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_service_read_blk_drs(bf_dev_id_t dev_id);

/**
 * Service the write block completion DR
 * @param dev_id Device id
 * @param tx Service the Tx DR
 * @param rx Service the Rx DR
 * @return Returns @c PIPE_LLD_FAILED if the servicing fails
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_service_write_blk_drs(bf_dev_id_t dev_id,
                                                 bool tx,
                                                 bool rx);

/**
 * Service the Stats DR
 * @param dev_id Device id
 * @return Returns @c PIPE_LLD_FAILED if the servicing fails
 *         Returns @c PIPE_SUCCESS on success.
 */
pipe_status_t pipe_mgr_drv_service_stats_drs(bf_dev_id_t dev_id);

typedef pipe_status_t (*pipe_mgr_drv_service_drs_fn)(bf_dev_id_t dev_id);
typedef pipe_status_t (*pipe_mgr_drv_push_drs_fn)(bf_dev_id_t dev_id);

enum pipe_mgr_drv_buf_type {
  PIPE_MGR_DRV_BUF_FIRST = 0,
  PIPE_MGR_DRV_BUF_IL = 0,
  PIPE_MGR_DRV_BUF_LRN,
  PIPE_MGR_DRV_BUF_LRT,
  PIPE_MGR_DRV_BUF_IDL,
  PIPE_MGR_DRV_BUF_BRD,
  PIPE_MGR_DRV_BUF_BWR,
  PIPE_MGR_DRV_BUF_CNT
};

int pipe_mgr_drv_subdev_buf_size(bf_dev_id_t dev,
                                 bf_subdev_id_t subdev,
                                 enum pipe_mgr_drv_buf_type type);
int pipe_mgr_drv_buf_size(bf_dev_id_t dev, enum pipe_mgr_drv_buf_type type);
int pipe_mgr_drv_subdev_buf_count(bf_dev_id_t dev,
                                  bf_subdev_id_t subdev,
                                  enum pipe_mgr_drv_buf_type type);
bf_sys_dma_pool_handle_t pipe_mgr_drv_subdev_dma_pool_handle(
    bf_dev_id_t dev, bf_subdev_id_t subdev, enum pipe_mgr_drv_buf_type type);

/**
 * Allocate a DMA buffer.
 * @param sid The session allocating the buffer.
 * @param devId The device id that the buffer will be sent to.
 * @param size The required size.
 * @param waitOk @c true if the function should wait for buffers to become
 *        available when the session is out of buffers.
 * @param type Type of operation the buffer is for, this determines the pool
 *             from which the buffer is allocated.  See bf_dma_info_t for
 *             details.
 * @return Returns a pointer to a buffer on success or @c NULL on failure.
 */
pipe_mgr_drv_buf_t *pipe_mgr_drv_buf_alloc(uint8_t sid,
                                           uint8_t devId,
                                           uint32_t size,
                                           enum pipe_mgr_drv_buf_type type,
                                           bool waitOk);

pipe_mgr_drv_buf_t *pipe_mgr_drv_buf_alloc_subdev(
    uint8_t sid,
    uint8_t devId,
    bf_subdev_id_t subdev_id,
    uint32_t size,
    enum pipe_mgr_drv_buf_type type,
    bool waitOk);

/* Push write blks to DR */
pipe_status_t pipe_mgr_push_wr_blks_to_dr(pipe_sess_hdl_t *sess,
                                          bf_dev_id_t dev_id);

pipe_status_t pipe_mgr_decode_ilist(rmt_dev_info_t *dev_info,
                                    bf_dev_pipe_t log_pipe,
                                    uint32_t *addr,
                                    int buf_len,
                                    char *log_buf,
                                    int log_len);

#endif  //_PIPE_MGR_DRV_INTF_H
