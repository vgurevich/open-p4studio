#include "gen-cpp/knet_mgr.h"

extern "C" {
#include <tofino/pdfixed/pd_common.h>
#include <tofino/pdfixed/pd_knet_mgr.h>
}

using namespace ::knet_mgr_pd_rpc;
using namespace ::res_pd_rpc;

class knet_mgrHandler : virtual public knet_mgrIf {
 public:
  knet_mgrHandler() {}

  void knet_cpuif_ndev_add(knet_cpuif_res_t &_return,
                           const std::string &cpuif_netdev_name) {
    char kname[IFNAMSIZ];
    p4_pd_knet_cpuif_t id;
    int32_t status =
        p4_knet_cpuif_ndev_add(cpuif_netdev_name.c_str(), kname, &id);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.name.assign((const char *)kname, IFNAMSIZ);
      _return.knet_cpuif_id = id;
    }
  }

  status_t knet_cpuif_ndev_delete(const knet_cpuif_t knet_cpuif_id) {
    int32_t status = p4_knet_cpuif_ndev_delete(knet_cpuif_id);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void knet_hostif_kndev_add(knet_hostif_kndev_res_t &_return,
                             const knet_cpuif_t knet_cpuif_id,
                             const std::string &hostif_name) {
    p4_pd_knet_hostif_t id;
    if (hostif_name.length() > IFNAMSIZ) _return.status = BF_INVALID_ARG;

    int32_t status =
        p4_knet_hostif_kndev_add(knet_cpuif_id, hostif_name.c_str(), &id);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.knet_hostif_id = id;
    }
  }

  status_t knet_hostif_kndev_delete(const knet_cpuif_t knet_cpuif_id,
                                    const knet_hostif_t knet_hostif_id) {
    int32_t status = p4_knet_hostif_kndev_delete(knet_cpuif_id, knet_hostif_id);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void knet_rx_filter_add(knet_filter_res_t &_return,
                          const knet_cpuif_t knet_cpuif_id,
                          const knet_rx_filter_t &rx_filter) {
    p4_pd_knet_rx_filter_t knet_rx_filter;

    knet_rx_filter.spec.priority = rx_filter.spec.priority;
    knet_rx_filter.spec.filter_size = rx_filter.spec.filter_size;
    rx_filter.spec.filter.copy(
        (char *)knet_rx_filter.spec.filter, BF_KNET_FILTER_BYTES_MAX, 0);
    rx_filter.spec.mask.copy(
        (char *)knet_rx_filter.spec.mask, BF_KNET_FILTER_BYTES_MAX, 0);

    switch (rx_filter.action.dest_type) {
      case BF_KNET_FILTER_DESTINATION_CPUIF:
        knet_rx_filter.action.dest_type = BF_KNET_FILTER_DESTINATION_CPUIF;
        break;
      case BF_KNET_FILTER_DESTINATION_HOSTIF:
        knet_rx_filter.action.dest_type = BF_KNET_FILTER_DESTINATION_HOSTIF;
        break;
      default:
        knet_rx_filter.action.dest_type = BF_KNET_FILTER_DESTINATION_NULL;
        break;
    }

    knet_rx_filter.action.knet_hostif_id = rx_filter.action.knet_hostif_id;
    knet_rx_filter.action.dest_proto = rx_filter.action.dest_proto;
    knet_rx_filter.action.count = rx_filter.action.count;

    if (rx_filter.action.pkt_mutation.size() != knet_rx_filter.action.count) {
      InvalidMcOperation iop;
      iop.code = BF_UNEXPECTED;
      throw iop;
    }

    knet_rx_filter.action.pkt_mutation =
        (p4_pd_knet_packet_mutation_t *)bf_sys_malloc(
            sizeof(p4_pd_knet_packet_mutation_t) * rx_filter.action.count);
    if (!knet_rx_filter.action.pkt_mutation) {
      InvalidMcOperation iop;
      iop.code = BF_NO_SYS_RESOURCES;
      throw iop;
    }

    int count = 0;
    for (auto &i : rx_filter.action.pkt_mutation) {
      switch (i.mutation_type) {
        case BF_KNET_RX_MUT_INSERT:
          knet_rx_filter.action.pkt_mutation[count].mutation_type =
              BF_KNET_RX_MUT_INSERT;
          break;
        case BF_KNET_RX_MUT_STRIP:
          knet_rx_filter.action.pkt_mutation[count].mutation_type =
              BF_KNET_RX_MUT_STRIP;
          break;
        default:
          knet_rx_filter.action.pkt_mutation[count].mutation_type =
              BF_KNET_RX_MUT_NONE;
          break;
      }
      knet_rx_filter.action.pkt_mutation[count].offset = i.offset;
      knet_rx_filter.action.pkt_mutation[count].len = i.len;
      i.data.copy((char *)knet_rx_filter.action.pkt_mutation[count].data,
                  BF_KNET_DATA_BYTES_MAX,
                  0);
      count++;
    }

    int32_t status = p4_knet_rx_filter_add(knet_cpuif_id, &knet_rx_filter);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      if (!knet_rx_filter.action.pkt_mutation) {
        bf_sys_free(knet_rx_filter.action.pkt_mutation);
      }
      throw iop;
    }
    bf_sys_free(knet_rx_filter.action.pkt_mutation);
    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.filter_id = knet_rx_filter.spec.filter_id;
    }
  }

  status_t knet_rx_filter_delete(const knet_cpuif_t knet_cpuif_id,
                                 const knet_filter_t filter_id) {
    int32_t status = p4_knet_rx_filter_delete(knet_cpuif_id, filter_id);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  void knet_rx_filter_get(knet_rx_filter_res_t &_return,
                          const knet_cpuif_t knet_cpuif_id,
                          const knet_filter_t filter_id,
                          const knet_count_t rx_mutation_count) {
    p4_pd_knet_rx_filter_t rx_filter;
    rx_filter.action.pkt_mutation =
        (p4_pd_knet_packet_mutation_t *)bf_sys_calloc(
            1, sizeof(p4_pd_knet_packet_mutation_t) * rx_mutation_count);
    if (rx_mutation_count && !rx_filter.action.pkt_mutation) {
      InvalidMcOperation iop;
      iop.code = BF_NO_SYS_RESOURCES;
      throw iop;
    }

    int32_t status = p4_knet_rx_filter_get(
        knet_cpuif_id, filter_id, &rx_filter, rx_mutation_count);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      if (rx_mutation_count && !rx_filter.action.pkt_mutation) {
        bf_sys_free(rx_filter.action.pkt_mutation);
      }
      throw iop;
    }
    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.rx_filter.spec.filter_id = rx_filter.spec.filter_id;
      _return.rx_filter.spec.priority = rx_filter.spec.priority;
      _return.rx_filter.spec.filter_size = rx_filter.spec.filter_size;
      _return.rx_filter.spec.filter =
          std::string(reinterpret_cast<char *>(rx_filter.spec.filter),
                      BF_KNET_FILTER_BYTES_MAX);
      _return.rx_filter.spec.mask =
          std::string(reinterpret_cast<char *>(rx_filter.spec.mask),
                      BF_KNET_FILTER_BYTES_MAX);

      switch (rx_filter.action.dest_type) {
        case BF_KNET_FILTER_DESTINATION_CPUIF:
          _return.rx_filter.action.dest_type = knet_mgr_pd_rpc::
              knet_filter_dest_t::BF_KNET_FILTER_DESTINATION_CPUIF;
          break;
        case BF_KNET_FILTER_DESTINATION_HOSTIF:
          _return.rx_filter.action.dest_type = knet_mgr_pd_rpc::
              knet_filter_dest_t::BF_KNET_FILTER_DESTINATION_HOSTIF;
          break;
        default:
          _return.rx_filter.action.dest_type = knet_mgr_pd_rpc::
              knet_filter_dest_t::BF_KNET_FILTER_DESTINATION_NULL;
          break;
      }
      _return.rx_filter.action.knet_hostif_id = rx_filter.action.knet_hostif_id;
      _return.rx_filter.action.dest_proto = rx_filter.action.dest_proto;
      _return.rx_filter.action.count = rx_filter.action.count;
      _return.rx_filter.action.pkt_mutation.resize(rx_filter.action.count);
      for (int i = 0; i < rx_filter.action.count; i++) {
        _return.rx_filter.action.pkt_mutation[i].offset =
            rx_filter.action.pkt_mutation[i].offset;
        _return.rx_filter.action.pkt_mutation[i].len =
            rx_filter.action.pkt_mutation[i].len;
        switch (rx_filter.action.pkt_mutation[i].mutation_type) {
          case BF_KNET_RX_MUT_NONE:
            _return.rx_filter.action.pkt_mutation[i].mutation_type =
                knet_mgr_pd_rpc::knet_mutation_type_t::BF_KNET_RX_MUT_NONE;
            break;
          case BF_KNET_RX_MUT_INSERT:
            _return.rx_filter.action.pkt_mutation[i].mutation_type =
                knet_mgr_pd_rpc::knet_mutation_type_t::BF_KNET_RX_MUT_INSERT;
            break;
          default:
            _return.rx_filter.action.pkt_mutation[i].mutation_type =
                knet_mgr_pd_rpc::knet_mutation_type_t::BF_KNET_RX_MUT_STRIP;
            break;
        }
        _return.rx_filter.action.pkt_mutation[i].data = std::string(
            reinterpret_cast<char *>(rx_filter.action.pkt_mutation[i].data),
            BF_KNET_FILTER_BYTES_MAX);
      }
    }
    if (rx_mutation_count && rx_filter.action.pkt_mutation != NULL) {
      bf_sys_free(rx_filter.action.pkt_mutation);
    }
  }

  status_t knet_tx_action_add(const knet_cpuif_t knet_cpuif_id,
                              const knet_hostif_t knet_hostif_id,
                              const knet_tx_action_t &tx_action) {
    p4_pd_knet_tx_action_t knet_tx_action;

    knet_tx_action.count = tx_action.count;

    if (tx_action.pkt_mutation.size() != knet_tx_action.count) {
      InvalidMcOperation iop;
      iop.code = BF_UNEXPECTED;
      throw iop;
    }

    knet_tx_action.pkt_mutation = (p4_pd_knet_packet_mutation_t *)bf_sys_malloc(
        sizeof(p4_pd_knet_packet_mutation_t) * tx_action.count);
    if (!knet_tx_action.pkt_mutation) {
      InvalidMcOperation iop;
      iop.code = BF_NO_SYS_RESOURCES;
      throw iop;
    }

    int count = 0;
    for (auto &i : tx_action.pkt_mutation) {
      switch (i.mutation_type) {
        case BF_KNET_RX_MUT_INSERT:
          knet_tx_action.pkt_mutation[count].mutation_type =
              BF_KNET_RX_MUT_INSERT;
          break;
        case BF_KNET_RX_MUT_STRIP:
          knet_tx_action.pkt_mutation[count].mutation_type =
              BF_KNET_RX_MUT_STRIP;
          break;
        default:
          knet_tx_action.pkt_mutation[count].mutation_type =
              BF_KNET_RX_MUT_NONE;
          break;
      }
      knet_tx_action.pkt_mutation[count].offset = i.offset;
      knet_tx_action.pkt_mutation[count].offset = i.offset;
      knet_tx_action.pkt_mutation[count].len = i.len;
      if (i.mutation_type ==
          knet_mutation_type_t::type::BF_KNET_RX_MUT_INSERT) {
        i.data.copy((char *)knet_tx_action.pkt_mutation[count].data,
                    BF_KNET_DATA_BYTES_MAX,
                    0);
      }
      count++;
    }

    int32_t status =
        p4_knet_tx_action_add(knet_cpuif_id, knet_hostif_id, &knet_tx_action);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      if (!knet_tx_action.pkt_mutation) {
        bf_sys_free(knet_tx_action.pkt_mutation);
      }
      throw iop;
    }
    bf_sys_free(knet_tx_action.pkt_mutation);
    return status;
  }

  void knet_tx_action_get(knet_tx_action_res_t &_return,
                          const knet_cpuif_t knet_cpuif_id,
                          const knet_hostif_t knet_hostif_id,
                          const knet_count_t tx_mutation_count) {
    p4_pd_knet_tx_action_t tx_action;
    tx_action.pkt_mutation = (p4_pd_knet_packet_mutation_t *)bf_sys_calloc(
        1, sizeof(p4_pd_knet_packet_mutation_t) * tx_mutation_count);
    if (tx_mutation_count && !tx_action.pkt_mutation) {
      InvalidMcOperation iop;
      iop.code = BF_NO_SYS_RESOURCES;
      throw iop;
    }

    int32_t status = p4_knet_tx_action_get(
        knet_cpuif_id, knet_hostif_id, &tx_action, tx_mutation_count);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      if (tx_mutation_count && !tx_action.pkt_mutation) {
        bf_sys_free(tx_action.pkt_mutation);
      }
      throw iop;
    }
    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.tx_action.count = tx_action.count;
      _return.tx_action.pkt_mutation.resize(tx_action.count);
      for (int i = 0; i < tx_action.count; i++) {
        _return.tx_action.pkt_mutation[i].offset =
            tx_action.pkt_mutation[i].offset;
        _return.tx_action.pkt_mutation[i].len = tx_action.pkt_mutation[i].len;
        switch (tx_action.pkt_mutation[i].mutation_type) {
          case BF_KNET_RX_MUT_NONE:
            _return.tx_action.pkt_mutation[i].mutation_type =
                knet_mgr_pd_rpc::knet_mutation_type_t::BF_KNET_RX_MUT_NONE;
            break;
          case BF_KNET_RX_MUT_INSERT:
            _return.tx_action.pkt_mutation[i].mutation_type =
                knet_mgr_pd_rpc::knet_mutation_type_t::BF_KNET_RX_MUT_INSERT;
            break;
          default:
            _return.tx_action.pkt_mutation[i].mutation_type =
                knet_mgr_pd_rpc::knet_mutation_type_t::BF_KNET_RX_MUT_STRIP;
            break;
        }
        if (tx_action.pkt_mutation[i].mutation_type == BF_KNET_RX_MUT_INSERT) {
          _return.tx_action.pkt_mutation[i].data = std::string(
              reinterpret_cast<char *>(tx_action.pkt_mutation[i].data),
              BF_KNET_FILTER_BYTES_MAX);
        }
      }
    }
    if (tx_mutation_count && tx_action.pkt_mutation != NULL) {
      bf_sys_free(tx_action.pkt_mutation);
    }
  }

  status_t knet_tx_action_delete(const knet_cpuif_t knet_cpuif_id,
                                 const knet_hostif_t knet_hostif_id) {
    int32_t status = p4_knet_tx_action_delete(knet_cpuif_id, knet_hostif_id);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    return status;
  }

  bool knet_module_is_inited() { return p4_knet_module_is_inited(); }

  void knet_get_cpuif_cnt(knet_count_res_t &_return) {
    p4_pd_knet_count_t obj_count;
    int32_t status = p4_knet_get_cpuif_cnt(&obj_count);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.obj_count = obj_count;
    }
    return;
  }

  void knet_get_hostif_cnt(knet_count_res_t &_return,
                           const knet_cpuif_t knet_cpuif_id) {
    p4_pd_knet_count_t obj_count;
    int32_t status = p4_knet_get_hostif_cnt(knet_cpuif_id, &obj_count);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.obj_count = obj_count;
    }
    return;
  }

  void knet_get_rx_filter_cnt(knet_count_res_t &_return,
                              const knet_cpuif_t knet_cpuif_id) {
    p4_pd_knet_count_t obj_count;
    int32_t status = p4_knet_get_rx_filter_cnt(knet_cpuif_id, &obj_count);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.obj_count = obj_count;
    }
  }

  void knet_get_rx_mutation_cnt(knet_count_res_t &_return,
                                const knet_cpuif_t knet_cpuif_id,
                                const knet_filter_t filter_id) {
    p4_pd_knet_count_t obj_count;
    int32_t status =
        p4_knet_get_rx_mutation_cnt(knet_cpuif_id, filter_id, &obj_count);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.obj_count = obj_count;
    }
    return;
  }

  void knet_get_tx_mutation_cnt(knet_count_res_t &_return,
                                const knet_cpuif_t knet_cpuif_id,
                                const knet_hostif_t hostif_id) {
    p4_pd_knet_count_t obj_count;
    int32_t status =
        p4_knet_get_tx_mutation_cnt(knet_cpuif_id, hostif_id, &obj_count);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      throw iop;
    }
    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.obj_count = obj_count;
    }
    return;
  }

  void knet_rx_filter_list_get(knet_filter_list_res_t &_return,
                               const knet_cpuif_t knet_cpuif_id,
                               const knet_count_t filter_count) {
    p4_pd_knet_count_t fcount;
    p4_pd_knet_filter_t *filter_list = NULL;
    fcount = filter_count;

    if (fcount > 0) {
      filter_list = (p4_pd_knet_filter_t *)bf_sys_calloc(
          1, sizeof(p4_pd_knet_filter_t) * fcount);
    }
    if (fcount && !filter_list) {
      InvalidMcOperation iop;
      iop.code = BF_NO_SYS_RESOURCES;
      throw iop;
    }
    int32_t status =
        p4_knet_get_rx_filter_list(knet_cpuif_id, filter_list, &fcount);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      if (fcount && !filter_list) {
        bf_sys_free(filter_list);
      }
      throw iop;
    }

    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.filter_list.resize(fcount);
      for (int i = 0; i < fcount; i++) {
        _return.filter_list[i] = filter_list[i];
      }
    }
    if (fcount && !filter_list) {
      bf_sys_free(filter_list);
    }
  }

  void knet_cpuif_list_get(knet_cpuif_list_res_t &_return,
                           const knet_count_t cpuif_count) {
    p4_pd_knet_count_t cpu_count;
    p4_pd_knet_cpuif_list_t *cpuif_list = NULL;
    cpu_count = cpuif_count;

    if (cpu_count > 0) {
      cpuif_list = (p4_pd_knet_cpuif_list_t *)bf_sys_calloc(
          1, sizeof(p4_pd_knet_cpuif_list_t) * cpu_count);
      if (!cpuif_list) {
        InvalidMcOperation iop;
        iop.code = BF_NO_SYS_RESOURCES;
        throw iop;
      }
    }
    int32_t status = p4_knet_get_cpuif_list(cpuif_list, &cpu_count);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      if (cpu_count && !cpuif_list) {
        bf_sys_free(cpuif_list);
      }
      throw iop;
    }

    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.cpuif_list.resize(cpu_count);
      for (int i = 0; i < cpu_count; i++) {
        _return.cpuif_list[i].id = cpuif_list[i].id;
        _return.cpuif_list[i].name.assign((const char *)cpuif_list[i].name);
      }
    }
    if (cpu_count && !cpuif_list) {
      bf_sys_free(cpuif_list);
    }
  }

  void knet_hostif_list_get(knet_hostif_list_res_t &_return,
                            const knet_cpuif_t knet_cpuif_id,
                            const knet_count_t hostif_count) {
    p4_pd_knet_count_t host_count;
    p4_pd_knet_hostif_list_t *hostif_list = NULL;
    host_count = hostif_count;

    if (host_count > 0) {
      hostif_list = (p4_pd_knet_hostif_list_t *)bf_sys_calloc(
          1, sizeof(p4_pd_knet_hostif_list_t) * host_count);
      if (!hostif_list) {
        InvalidMcOperation iop;
        iop.code = BF_NO_SYS_RESOURCES;
        throw iop;
      }
    }
    int32_t status =
        p4_knet_get_hostif_list(knet_cpuif_id, hostif_list, &host_count);
    if (status == BF_UNEXPECTED) {
      InvalidMcOperation iop;
      iop.code = status;
      if (host_count && !hostif_list) {
        bf_sys_free(hostif_list);
      }
      throw iop;
    }

    _return.status = status;
    if (status == BF_SUCCESS) {
      _return.hostif_list.resize(host_count);
      for (int i = 0; i < host_count; i++) {
        _return.hostif_list[i].id = hostif_list[i].id;
        _return.hostif_list[i].name.assign((const char *)hostif_list[i].name);
      }
    }
    if (host_count && !hostif_list) {
      bf_sys_free(hostif_list);
    }
  }
};
