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


#ifndef _BF_RT_TM_TABLE_KEY_IMPL_HPP
#define _BF_RT_TM_TABLE_KEY_IMPL_HPP

#include <bf_rt_common/bf_rt_table_impl.hpp>
#include <bf_rt_common/bf_rt_table_key_impl.hpp>
#include <bf_rt_common/bf_rt_table_field_utils.hpp>

namespace bfrt {

#define BFRT_TM_LINK_KEY_FIELD(p_tbl_, a_field_id_, a_field_name_)         \
  {                                                                        \
    a_field_id_ = 0;                                                       \
    auto status_ = p_tbl_->keyFieldIdGet((a_field_name_), &(a_field_id_)); \
    if (status_ != BF_SUCCESS)                                             \
      LOG_ERROR("%s:%d %s Can't find key field id for %s",                 \
                __func__,                                                  \
                __LINE__,                                                  \
                p_tbl_->table_name_get().c_str(),                          \
                (a_field_name_));                                          \
  }

class BfRtTMPpgTableKey : public BfRtTableKeyObj {
 public:
  BfRtTMPpgTableKey(const BfRtTableObj *tbl_obj) : BfRtTableKeyObj(tbl_obj) {
    this->ppg_id = 0;
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, ppg_id_field, "ppg_id")
  }

  ~BfRtTMPpgTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &s) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &s,
                       uint8_t *value) const override final;

  bf_status_t reset() override final;

  void setId(const bf_tm_ppg_id_t ppg_id_) { this->ppg_id = ppg_id_; }

  bf_status_t getId(bf_tm_ppg_id_t &ppg_id_) const {
    if (this->table_ == nullptr || this->ppg_id_field == 0) {
      return BF_OBJECT_NOT_FOUND;
    }
    ppg_id_ = this->ppg_id;
    return BF_SUCCESS;
  }

 private:
  bf_rt_id_t ppg_id_field;

  bf_tm_ppg_id_t ppg_id;
};

class BfRtTMPortGroupTableKey : public BfRtTableKeyObj {
 public:
  BfRtTMPortGroupTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj) {
    this->pg_id = 0;
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, pg_id_field, "pg_id")
  }

  ~BfRtTMPortGroupTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &s) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &s,
                       uint8_t *value) const override final;

  bf_status_t reset() override final;

  void setId(const bf_tm_pg_t pg_id_) { this->pg_id = pg_id_; }

  bf_status_t getId(bf_tm_pg_t &pg_id_) const {
    if (this->table_ == nullptr || this->pg_id_field == 0) {
      return BF_OBJECT_NOT_FOUND;
    }
    pg_id_ = this->pg_id;
    return BF_SUCCESS;
  }

 private:
  bf_rt_id_t pg_id_field;

  bf_tm_pg_t pg_id;
};

class BfRtTMQueueTableKey : public BfRtTableKeyObj {
 public:
  BfRtTMQueueTableKey(const BfRtTableObj *tbl_obj) : BfRtTableKeyObj(tbl_obj) {
    this->pg_id = 0;
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, pg_id_field, "pg_id")
    this->pg_queue = 0;
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, pg_queue_field, "pg_queue")
  }

  ~BfRtTMQueueTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &s) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &s,
                       uint8_t *value) const override final;

  bf_status_t reset() override final;

  void setId(const bf_tm_pg_t &pg_id_, const uint8_t pg_queue_) {
    this->pg_id = pg_id_;
    this->pg_queue = pg_queue_;
  }

  bf_status_t getId(bf_tm_pg_t &pg_id_, uint8_t &pg_queue_) const {
    if (this->table_ == nullptr || this->pg_id_field == 0 ||
        this->pg_queue_field == 0) {
      return BF_OBJECT_NOT_FOUND;
    }
    pg_id_ = this->pg_id;
    pg_queue_ = this->pg_queue;
    return BF_SUCCESS;
  }

 private:
  bf_rt_id_t pg_id_field;
  bf_rt_id_t pg_queue_field;

  bf_tm_pg_t pg_id;
  uint8_t pg_queue;
};

class BfRtTML1NodeTableKey : public BfRtTableKeyObj {
 public:
  BfRtTML1NodeTableKey(const BfRtTableObj *tbl_obj) : BfRtTableKeyObj(tbl_obj) {
    this->pg_id = 0;
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, pg_id_field, "pg_id")
    this->pg_l1_node = 0;
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, pg_l1_node_field, "pg_l1_node")
  }

  ~BfRtTML1NodeTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &s) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &s,
                       uint8_t *value) const override final;

  bf_status_t reset() override final;

  void setId(const bf_tm_pg_t &pg_id_, const bf_tm_l1_node_t &pg_l1_node_) {
    this->pg_id = pg_id_;
    this->pg_l1_node = pg_l1_node_;
  }

  bf_status_t getId(bf_tm_pg_t &pg_id_, bf_tm_l1_node_t &pg_l1_node_) const {
    if (this->table_ == nullptr || this->pg_id_field == 0 ||
        this->pg_l1_node_field == 0) {
      return BF_OBJECT_NOT_FOUND;
    }
    pg_id_ = this->pg_id;
    pg_l1_node_ = this->pg_l1_node;
    return BF_SUCCESS;
  }

 private:
  bf_rt_id_t pg_id_field;
  bf_rt_id_t pg_l1_node_field;

  bf_tm_pg_t pg_id;
  bf_tm_l1_node_t pg_l1_node;
};

/** Pool Config Table **/
class BfRtTMPoolTableKey : public BfRtTableKeyObj {
 public:
  BfRtTMPoolTableKey(const BfRtTableObj *tbl_obj) : BfRtTableKeyObj(tbl_obj) {
    this->pool_ = "";
  }

  ~BfRtTMPoolTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &value) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *value) const override final;

  bf_status_t reset() override final;

 private:
  std::string pool_;
};

/** App pool Config Table **/
class BfRtTMPoolAppTableKey : public BfRtTableKeyObj {
 public:
  BfRtTMPoolAppTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj) {
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, pool_field_id_, "pool")
    this->pool_ = "";
  }

  ~BfRtTMPoolAppTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &value) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *value) const override final;

  bf_status_t reset() override final;

  bf_status_t getId(std::string &pool) const {
    if (this->table_ == nullptr || this->pool_field_id_ == 0) {
      return BF_OBJECT_NOT_FOUND;
    }
    pool = this->pool_;
    return BF_SUCCESS;
  }

  void setId(const std::string &pool) { this->pool_ = pool; }

 private:
  std::string pool_;

  bf_rt_id_t pool_field_id_;
};

/** Pool Color Config Table **/
class BfRtTMPoolColorTableKey : public BfRtTableKeyObj {
 public:
  BfRtTMPoolColorTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj) {
    this->color_ = "";
  }

  ~BfRtTMPoolColorTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &value) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *value) const override final;

  bf_status_t reset() override final;

 private:
  std::string color_;
};

/** App Pool PFC Config Table **/
class BfRtTMAppPoolPfcTableKey : public BfRtTableKeyObj {
 public:
  BfRtTMAppPoolPfcTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj) {
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, pool_field_id_, "pool")
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, cos_field_id_, "cos")
    this->reset();
  }

  ~BfRtTMAppPoolPfcTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::string &value) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::string *value) const override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &s) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &s,
                       uint8_t *value) const override final;

  bf_status_t reset() override final;

  void setIds(const std::string &pool, const uint8_t cos) {
    this->pool_ = pool;
    this->cos_ = cos;
  }

  bf_status_t getIds(std::string &pool, uint8_t &cos) const {
    if (this->table_ == nullptr || this->pool_field_id_ == 0 ||
        this->cos_field_id_ == 0) {
      return BF_OBJECT_NOT_FOUND;
    }
    pool = this->pool_;
    cos = this->cos_;
    return BF_SUCCESS;
  }

 private:
  std::string pool_;
  uint8_t cos_;

  bf_rt_id_t pool_field_id_;
  bf_rt_id_t cos_field_id_;
};

/** Dev port key **/
class BfRtTMDevPortKey : public BfRtTableKeyObj {
 public:
  BfRtTMDevPortKey(const BfRtTableObj *tbl_obj) : BfRtTableKeyObj(tbl_obj) {
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, dev_port_field_id_, "dev_port")
    this->reset();
  }

  ~BfRtTMDevPortKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &s) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &s,
                       uint8_t *value) const override final;

  bf_status_t reset() override final;

  void setId(bf_dev_port_t dev_port) {
    this->dev_port_ = static_cast<uint32_t>(dev_port);
  }

  bf_status_t getId(bf_dev_port_t &dev_port) const {
    if (this->table_ == nullptr || this->dev_port_field_id_ == 0) {
      return BF_OBJECT_NOT_FOUND;
    }
    dev_port = static_cast<bf_dev_port_t>(this->dev_port_);
    return BF_SUCCESS;
  }

 private:
  uint32_t dev_port_;

  bf_rt_id_t dev_port_field_id_;
};

/** PPG counter key **/
class BfRtTMPpgCounterTableKey : public BfRtTableKeyObj {
 public:
  BfRtTMPpgCounterTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj) {
    this->ppg_counter_id = 0;
    BFRT_TM_LINK_KEY_FIELD(tbl_obj, ppg_counter_id_field, "ppg_counter_id")
  }

  ~BfRtTMPpgCounterTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &s) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &s,
                       uint8_t *value) const override final;

  bf_status_t reset() override final;

  void setId(const bf_tm_ppg_id_t ppg_counter_id_) {
    this->ppg_counter_id = ppg_counter_id_;
  }

  bf_status_t getId(bf_tm_ppg_id_t &ppg_counter_id_) const {
    if (this->table_ == nullptr || this->ppg_counter_id_field == 0) {
      return BF_OBJECT_NOT_FOUND;
    }
    ppg_counter_id_ = this->ppg_counter_id;
    return BF_SUCCESS;
  }

 private:
  bf_rt_id_t ppg_counter_id_field;

  bf_tm_ppg_id_t ppg_counter_id;
};
}  // namespace bfrt
#endif
