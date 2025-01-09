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
 * @file pipe_mgr_exm_tbl_dump.c
 * @date
 *
 * Exact match table manager dump routines to dump debug information.
 */

/* Standard header includes */

/* Module header includes */
#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "pipe_mgr_exm_tbl_init.h"
#include "pipe_mgr_exm_tbl_dump.h"

void pipe_mgr_exm_dump_tbl_info(ucli_context_t *uc,
                                uint8_t device_id,
                                pipe_mat_tbl_hdl_t exm_tbl_hdl) {
  pipe_mgr_exm_tbl_t *exm_tbl;
  uint32_t stage_idx = 0;
  dev_target_t dev_tgt = {.device_id = device_id,
                          .dev_pipe_id = BF_DEV_PIPE_ALL};

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, exm_tbl_hdl);

  if (exm_tbl == NULL) {
    aim_printf(&uc->pvs,
               "No information found for device %d table 0x%x\n",
               device_id,
               exm_tbl_hdl);
    return;
  }

  aim_printf(&uc->pvs, "-------------------------------------------------\n");
  aim_printf(&uc->pvs,
             "Exact match table info for table %s (0x%x) device %d\n",
             exm_tbl->name,
             exm_tbl_hdl,
             device_id);
  aim_printf(&uc->pvs, "-------------------------------------------------\n");

  if (exm_tbl->proxy_hash) {
    aim_printf(&uc->pvs, " Proxy hash table\n");
  }
  if (exm_tbl->hash_action) {
    aim_printf(&uc->pvs, " Hash action table\n");
  }
  uint32_t num_entries_occupied = 0;
  pipe_mgr_exm_tbl_get_programmed_entry_count(
      dev_tgt, exm_tbl_hdl, &num_entries_occupied);
  aim_printf(&uc->pvs,
             "Size : %d, Number occupied : %d\n",
             exm_tbl->num_entries,
             num_entries_occupied);
  aim_printf(
      &uc->pvs, "Symmetric : %s\n", exm_tbl->symmetric ? "true" : "false");
  aim_printf(&uc->pvs, "Num of scopes : %d\n", exm_tbl->num_scopes);
  aim_printf(&uc->pvs, "Scopes pipe Bitmap:\n");
  for (uint32_t i = 0; i < exm_tbl->num_scopes; i++) {
    aim_printf(&uc->pvs, "  Scope[%d] : 0x%x\n", i, exm_tbl->scope_pipe_bmp[i]);
  }
  aim_printf(&uc->pvs,
             "Number of references to action data tables : %d\n",
             exm_tbl->num_adt_refs);
  for (uint32_t i = 0; i < exm_tbl->num_adt_refs; i++) {
    aim_printf(
        &uc->pvs, "  Table handle : 0x%x,", exm_tbl->adt_tbl_refs[i].tbl_hdl);
    aim_printf(
        &uc->pvs,
        "Reference type : %s\n",
        pipe_mgr_rmt_tbl_ref_type2str(exm_tbl->adt_tbl_refs[i].ref_type));
  }

  aim_printf(&uc->pvs,
             "Number of references to selection tables : %d\n",
             exm_tbl->num_adt_refs);

  for (uint32_t i = 0; i < exm_tbl->num_sel_tbl_refs; i++) {
    aim_printf(
        &uc->pvs, "  Table handle : 0x%x,", exm_tbl->sel_tbl_refs[i].tbl_hdl);
    aim_printf(
        &uc->pvs,
        "Reference type : %s\n",
        pipe_mgr_rmt_tbl_ref_type2str(exm_tbl->sel_tbl_refs[i].ref_type));
  }

  if (exm_tbl->symmetric) {
    aim_printf(&uc->pvs, "Pipe id : All pipes\n");
    aim_printf(&uc->pvs,
               "Number of stages : %d\n",
               exm_tbl->exm_tbl_data[0].num_stages);
    aim_printf(&uc->pvs, "Stages : ");
    for (stage_idx = 0; stage_idx < exm_tbl->exm_tbl_data[0].num_stages;
         stage_idx++) {
      aim_printf(&uc->pvs,
                 "%d, ",
                 exm_tbl->exm_tbl_data[0].exm_stage_info[stage_idx].stage_id);
    }

    aim_printf(&uc->pvs, "\n");

    for (stage_idx = 0; stage_idx < exm_tbl->exm_tbl_data[0].num_stages;
         stage_idx++) {
      aim_printf(&uc->pvs, "---------------------------\n");
      aim_printf(&uc->pvs,
                 "Stage %d info : \n",
                 exm_tbl->exm_tbl_data[0].exm_stage_info[stage_idx].stage_id);
      aim_printf(&uc->pvs, "---------------------------\n");
      pipe_mgr_exm_dump_stage_info(
          uc, &exm_tbl->exm_tbl_data[0].exm_stage_info[stage_idx]);
    }
  } else {
    uint32_t q = 0;
    uint32_t tbl_idx = 0;
    for (tbl_idx = 0; tbl_idx < exm_tbl->num_tbls; tbl_idx++) {
      aim_printf(
          &uc->pvs, "Pipe id : %d\n", exm_tbl->exm_tbl_data[tbl_idx].pipe_id);
      aim_printf(&uc->pvs, "   Pipes in bitmap : ");
      PIPE_BITMAP_ITER(&(exm_tbl->exm_tbl_data[tbl_idx].pipe_bmp), q) {
        aim_printf(&uc->pvs, "%d ", q);
      }
      aim_printf(&uc->pvs, "\n");
      aim_printf(&uc->pvs,
                 "  Number of stages : %d\n",
                 exm_tbl->exm_tbl_data[tbl_idx].num_stages);
      for (stage_idx = 0; stage_idx < exm_tbl->exm_tbl_data[tbl_idx].num_stages;
           stage_idx++) {
        aim_printf(
            &uc->pvs,
            "%d, ",
            exm_tbl->exm_tbl_data[tbl_idx].exm_stage_info[stage_idx].stage_id);
      }

      for (stage_idx = 0; stage_idx < exm_tbl->exm_tbl_data[tbl_idx].num_stages;
           stage_idx++) {
        aim_printf(
            &uc->pvs,
            "  Stage %d info\n",
            exm_tbl->exm_tbl_data[tbl_idx].exm_stage_info[stage_idx].stage_id);
        pipe_mgr_exm_dump_stage_info(
            uc, &exm_tbl->exm_tbl_data[tbl_idx].exm_stage_info[stage_idx]);
      }
    }
  }

  return;
}

void pipe_mgr_exm_dump_stage_info(ucli_context_t *uc,
                                  pipe_mgr_exm_stage_info_t *exm_stage_info) {
  uint32_t hash_way_idx = 0;

  aim_printf(&uc->pvs,
             "  Num entries : %d, Num Occupied : %d\n",
             exm_stage_info->num_entries,
             exm_stage_info->num_entries_placed);
  aim_printf(&uc->pvs, "  Packing format : \n");
  aim_printf(&uc->pvs,
             "      Number of entries per wide-word : %d\n",
             exm_stage_info->pack_format->num_entries_per_wide_word);
  aim_printf(&uc->pvs,
             "      Number of rams in wide-word : %d\n",
             exm_stage_info->pack_format->num_rams_in_wide_word);
  aim_printf(
      &uc->pvs, "  Number of hash-ways : %d\n", exm_stage_info->num_hash_ways);

  for (hash_way_idx = 0; hash_way_idx < exm_stage_info->num_hash_ways;
       hash_way_idx++) {
    aim_printf(&uc->pvs, "  Hashway %d info :\n", hash_way_idx);
    pipe_mgr_exm_dump_hashway_info(
        uc,
        &exm_stage_info->hashway_data[hash_way_idx],
        exm_stage_info->pack_format->num_rams_in_wide_word,
        exm_stage_info->pack_format->num_entries_per_wide_word);
  }

  return;
}

void pipe_mgr_exm_dump_hashway_info(
    ucli_context_t *uc,
    pipe_mgr_exm_hash_way_data_t *exm_hash_way_data,
    uint8_t num_rams_in_wide_word,
    uint8_t num_entries_in_wide_word) {
  uint32_t i = 0;
  uint32_t j = 0;
  uint32_t k = 0;

  aim_printf(&uc->pvs, "      Ram allocation info :\n");
  aim_printf(&uc->pvs,
             "          Number of wide-word blocks : %d\n",
             exm_hash_way_data->ram_alloc_info->num_wide_word_blks);
  aim_printf(&uc->pvs, "          Mem ids : \n");
  for (i = 0; i < exm_hash_way_data->ram_alloc_info->num_wide_word_blks; i++) {
    aim_printf(&uc->pvs, "              [");
    for (j = 0; j < num_rams_in_wide_word; j++) {
      aim_printf(&uc->pvs,
                 "%d, ",
                 exm_hash_way_data->ram_alloc_info->tbl_word_blk[i].mem_id[j]);
    }

    aim_printf(&uc->pvs, "]\n");

    aim_printf(&uc->pvs, "      Entry VPN info : \n");
    for (k = 0; k < num_entries_in_wide_word; k++) {
      aim_printf(&uc->pvs,
                 "      Sub-entry %d, VPN = %d\n",
                 k,
                 exm_hash_way_data->ram_alloc_info->tbl_word_blk[i].vpn_id[k]);
    }
  }
}

/* A uCLI helper to print out the hash used by an EXM table.
 * The hash is displayed in two different formats; the two formats provide
 * similar information but are easier to read for different purposes.
 * The first format is a tabular representation where the rows are the bits of
 * the key (match spec) and columns represent the 52 hash bits.  A one indicates
 * that key bit contributes to that hash bit.
 * The second format has one row per hash bit, the contents of the row name each
 * key bit which contributes to that hash bit.  To reduce the length of a row,
 * rather than give the full field name, the name is substituted with a short
 * one or two character name and the mapping is shown at the end. */
void pipe_mgr_exm_tbl_dump_hash(ucli_context_t *uc,
                                bf_dev_id_t device_id,
                                pipe_mat_tbl_hdl_t exm_tbl_hdl) {
  pipe_mgr_exm_tbl_t *exm_tbl = pipe_mgr_exm_tbl_get(device_id, exm_tbl_hdl);
  if (exm_tbl == NULL) {
    aim_printf(&uc->pvs,
               "No information found for device %d table 0x%x\n",
               device_id,
               exm_tbl_hdl);
    return;
  }
  aim_printf(&uc->pvs,
             "Exact match hash info for table %s (0x%x) device %d\n",
             exm_tbl->name,
             exm_tbl_hdl,
             device_id);

  /* Go over all stages and display the hash config for that stage.  Since the
   * hash config is usually fixed, only display it for the first instance.  */
  for (uint32_t stage_idx = 0; stage_idx < exm_tbl->exm_tbl_data[0].num_stages;
       stage_idx++) {
    pipe_mgr_exm_stage_info_t *exm_stage_info =
        &exm_tbl->exm_tbl_data[0].exm_stage_info[stage_idx];
    dev_stage_t stage_id = exm_stage_info->stage_id;

    aim_printf(&uc->pvs, "\nHash Config for Stage %2d\n", stage_id);
    aim_printf(&uc->pvs, "=========================\n");

    /* Note that Hash Action tables will not have any hash way data
     * (num_hash_ways will be zero) so this section will be skipped for them. */
    for (unsigned i = 0; i < exm_stage_info->num_hash_ways; ++i) {
      pipe_mgr_exm_hash_way_data_t *way_info = &exm_stage_info->hashway_data[i];
      aim_printf(&uc->pvs, "Way %d: ", i);
      /* If the ways are only a single ram deep there will not be any RAM select
       * bits so we conditionally disaplay them. */
      if (way_info->num_ram_select_bits) {
        aim_printf(&uc->pvs,
                   "Hash Bits for Ram Select %d..%d, ",
                   way_info->ram_select_start_offset +
                       way_info->num_ram_select_bits - 1,
                   way_info->ram_select_start_offset);
      }
      aim_printf(
          &uc->pvs,
          "Hash Bits for Ram Line %2d..%d\n",
          way_info->ram_line_start_offset + way_info->num_ram_line_bits - 1,
          way_info->ram_line_start_offset);
    }

    uint64_t seed;
    pipe_status_t sts = bf_hash_get_hash_seed(exm_tbl->dev_id,
                                              exm_tbl->profile_id,
                                              stage_id,
                                              exm_tbl->mat_tbl_hdl,
                                              false,
                                              &seed);
    if (sts != PIPE_SUCCESS) {
      aim_printf(&uc->pvs, "Error %s getting hash seed\n", pipe_str_err(sts));
    } else {
      aim_printf(&uc->pvs, "Hash Seed 0x%" PRIx64 "\n", seed);
    }

    /* A map from field_name (pointer) to an integer field-id.  This will be
     * populated as the match spec is scanned and used to display short names
     * for the, potentially, long key field names. */
    bf_map_t field_to_id;
    bf_map_init(&field_to_id);
    int fld_id, next_id = 0;

    char *hash_strs[TOF_MAX_GFM_COLUMNS] = {0};
    size_t hash_str_len[TOF_MAX_GFM_COLUMNS] = {0};

    /* Dump out a big table showing which key bits contribute to which hash
     * bits.
     * We add a little extra spacing between 10-bit segments since that is
     * generally where the hash is divided for each way and makes it easier to
     * read. */
    aim_printf(&uc->pvs,
               "%-40s |     |                                           Hash "
               "Contribution (to hash bits 51..0)\n",
               "");
    aim_printf(
        &uc->pvs,
        "%-40s |Field| Key | 5|5|4|4|4|4|4|4|4|4|4|4|  3|3|3|3|3|3|3|3|3|3|  "
        "2|2|2|2|2|2|2|2|2|2|  1|1|1|1|1|1|1|1|1|1|   | | | | | | | | | \n",
        "");
    aim_printf(
        &uc->pvs,
        "%-40s | Bit | Bit | 1|0|9|8|7|6|5|4|3|2|1|0|  9|8|7|6|5|4|3|2|1|0|  "
        "9|8|7|6|5|4|3|2|1|0|  9|8|7|6|5|4|3|2|1|0|  9|8|7|6|5|4|3|2|1|0|  Raw "
        "Contrib   | Error\n",
        "Field Name");
    aim_printf(&uc->pvs,
               "%-40s-+-----+-----+--+-+-+-+-+-+-+-+-+-+-+-+---+-+-+-+-+-+-+-+-"
               "+-+---+-+-+-+-+-+-+-+-+-+---+-+-+-+-+-+-+-+-+-+---+-+-+-+-+-+-+"
               "-+-+-+----------------+------\n",
               "----------------------------------------");
    for (unsigned ms_byte = 0; ms_byte < exm_tbl->num_match_spec_bytes;
         ++ms_byte) {
      for (int bit = 7; bit >= 0; --bit) {
        int ms_bit = ms_byte * 8 + bit;
        char *field_name = NULL;
        int field_bit = 0;
        bool valid_field = true;
        sts = pipe_mgr_entry_format_ms_bit_to_field(exm_tbl->dev_id,
                                                    exm_tbl->profile_id,
                                                    exm_tbl->mat_tbl_hdl,
                                                    ms_bit,
                                                    &field_name,
                                                    &field_bit);
        if (sts != PIPE_SUCCESS) {
          /* This is okay, not all match spec bits belong to a key field since
           * the match spec fields are padded to byte boundaries. */
          valid_field = false;
        } else {
          /* Save a mapping from the field's name to an integer id.  This will
           * be used later to generate short names for the fields for the second
           * display of the hash. */
          void *val = NULL;
          bf_map_sts_t m =
              bf_map_get(&field_to_id, (unsigned long)field_name, &val);
          if (m != BF_MAP_OK) {
            bf_map_add(&field_to_id,
                       (unsigned long)field_name,
                       (void *)(uintptr_t)next_id);
            fld_id = next_id;
            ++next_id;
          } else {
            fld_id = (int)(uintptr_t)val;
          }
        }

        uint64_t hash_bits = 0;
        sts = bf_hash_get_hash_bits_for_key_bit(exm_tbl->dev_id,
                                                exm_tbl->profile_id,
                                                stage_id,
                                                exm_tbl->mat_tbl_hdl,
                                                ms_bit,
                                                false,
                                                &hash_bits);
        if (sts != PIPE_SUCCESS) {
          /* This is NOT expected, every match spec bit should have a
           * corresponding hash config! */
          aim_printf(&uc->pvs,
                     "Error %s getting hash info for match spec bit %d\n",
                     pipe_str_err(sts),
                     ms_bit);
          continue;
        }

        if (valid_field) {
          for (int i = 0; i < TOF_MAX_GFM_COLUMNS; ++i) {
            /* If the field doesn't contribute to this bit, move on. */
            if (0 == (hash_bits & (1ull << i))) continue;
            /* If this is the contribution to the bit allocate a new string of
             * a reasonable length. */
            bool first = false;
            if (!hash_strs[i]) {
              int initial_len = 256;
              hash_strs[i] = PIPE_MGR_CALLOC(initial_len, sizeof(char));
              if (!hash_strs[i]) {
                aim_printf(&uc->pvs, "Malloc failure\n");
                goto cleanup;
              }
              hash_str_len[i] = initial_len;
              first = true;
            }
            /* As the field names may be very long, map them from their integer
             * id to a two letter name for a simpler display.  Write it into a
             * temp string which we know is large enough, then check if the temp
             * can be appended to the existing string, if not re-alloc a larger
             * string to hold everything. */
            char tmp_str[128];
            int x = 0;
            if (!first) {
              tmp_str[x++] = ' ';
              tmp_str[x++] = '^';
              tmp_str[x++] = ' ';
            }
            /* Fields will be abbreviated with a two letter name as follows:
             * id 0 is A, id 1 is B, ..., id 25 is Z, id 26 is AA, id 27 is AB,
             * etc. up to ZZ.  This means we can support a maximum of 26*26
             * different field names.  Check now if we've not gone over that
             * limit and give up if we have. */
            if (fld_id >= 26 * 26) {
              aim_printf(
                  &uc->pvs, "Too many fields to display, fld_id=%d\n", fld_id);
              goto cleanup;
            } else if (fld_id >= 26) {
              tmp_str[x++] = 'A' + fld_id / 26;
            }
            tmp_str[x++] = 'A' + (fld_id % 26);
            sprintf(tmp_str + x, "[%d]", field_bit);
            if (strlen(hash_strs[i]) + strlen(tmp_str) >= hash_str_len[i]) {
              int alloc_sz = strlen(hash_strs[i]) + strlen(tmp_str) + 128;
              char *t = PIPE_MGR_CALLOC(alloc_sz, sizeof(char));
              if (!t) {
                aim_printf(&uc->pvs, "Malloc failure\n");
                goto cleanup;
              }
              sprintf(t, "%s", hash_strs[i]);
              PIPE_MGR_FREE(hash_strs[i]);
              hash_strs[i] = t;
              hash_str_len[i] = alloc_sz;
            }
            sprintf(hash_strs[i] + strlen(hash_strs[i]), "%s", tmp_str);
          }
        }

        /* If this is not a valid key bit then the hash contribution is expected
         * to be zero, print something out if this is not the case. */
        if (!valid_field && !hash_bits) {
          continue;
        } else if (!valid_field) {
          field_name = "ERROR INVALID KEY BIT";
        }

        /* If the key field's name is longer than 40 characters only show the
         * suffix.  Printing the whole thing will break the formatting of the
         * table making it very difficult to interpret the data. */
        char field_name_tmp[41] = {0};
        if (strlen(field_name) <= 40) {
          sprintf(field_name_tmp, "%s", field_name);
        } else {
          int x = strlen(field_name);
          for (int i = 0; i < 40; ++i)
            field_name_tmp[i] = field_name[x - 40 + i];
          field_name_tmp[40] = '\0';
        }

        char hash_str[128] = {0};
        for (int hash_bit = TOF_MAX_GFM_COLUMNS - 1, i = 0; hash_bit >= 0;
             --hash_bit) {
          /* Just as when printing the header, add a little extra space every
           * ten bits to make it easier to read. */
          if (hash_bit == 39 || hash_bit == 29 || hash_bit == 19 ||
              hash_bit == 9) {
            hash_str[i++] = ' ';
            hash_str[i++] = ' ';
          }
          hash_str[i++] = hash_bits & (1ull << hash_bit) ? '1' : ' ';
          hash_str[i++] = '|';
        }

        /* Display the hash contribution.  Suffix the line with a few astrisks
         * for attention if there is anything unexpected like a key bit that
         * does not contribute to the hash or a non-key bit which does
         * contribute to the hash. */
        bool unexpected = (!valid_field && hash_bits);
        aim_printf(&uc->pvs,
                   "%-40s | %3d | %3d | %s 0x%013" PRIx64 "%s\n",
                   field_name_tmp,
                   field_bit,
                   ms_bit,
                   hash_str,
                   hash_bits,
                   unexpected ? " *****" : "");
      }
    }
    aim_printf(&uc->pvs, "\n\nHash Bit :  Field Contribution\n");
    for (int i = TOF_MAX_GFM_COLUMNS - 1; i >= 0; --i) {
      if (!hash_strs[i]) continue;
      aim_printf(&uc->pvs, "%2d: %s\n", i, hash_strs[i]);
    }
    aim_printf(&uc->pvs, "\n");
    /* Double loop over the map contents to display the entries in order. */
    int num_keys = bf_map_count(&field_to_id);
    unsigned long key;
    void *f_val;
    for (int i = 0; i < num_keys; ++i) {
      for (bf_map_sts_t s = bf_map_get_first(&field_to_id, &key, &f_val);
           s == BF_MAP_OK;
           s = bf_map_get_next(&field_to_id, &key, &f_val)) {
        fld_id = (int)(uintptr_t)f_val;
        if (fld_id != i) continue;

        char *f_name = (char *)key;
        char tmp_str[4];
        int x = 0;
        if (fld_id >= 26) {
          tmp_str[x++] = 'A' + fld_id / 26;
        }
        tmp_str[x++] = 'A' + (fld_id % 26);
        tmp_str[x] = '\0';
        aim_printf(&uc->pvs, "%s == %s\n", tmp_str, f_name);
      }
    }
    while (BF_MAP_OK == bf_map_get_first_rmv(&field_to_id, &key, &f_val)) {
      /* Nothing to do, just emptying the map. */
    }
  cleanup:
    bf_map_destroy(&field_to_id);
    for (int i = 0; i < TOF_MAX_GFM_COLUMNS; ++i) {
      if (!hash_strs[i]) continue;
      PIPE_MGR_FREE(hash_strs[i]);
    }
  }
}

void pipe_mgr_exm_tbl_dump_entries(ucli_context_t *uc,
                                   bf_dev_id_t device_id,
                                   pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                   bool show_entry_handles) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  uint32_t ram_line_num = 0;
  int ent_hdl = -1;
  bool is_hash_action = false;
  dev_target_t dev_tgt = {.device_id = device_id,
                          .dev_pipe_id = BF_DEV_PIPE_ALL};

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    aim_printf(&uc->pvs, "No information found for table\n");
    return;
  }
  bool virtual_dev = pipe_mgr_is_device_virtual(device_id);
  /* Loop over all entry handles and print them */
  if (virtual_dev) {
    status = pipe_mgr_exm_get_first_placed_entry_handle(
        mat_tbl_hdl, dev_tgt, &ent_hdl);
  } else {
    status = pipe_mgr_exm_get_first_programmed_entry_handle(
        mat_tbl_hdl, dev_tgt, &ent_hdl);
  }

  if (status != PIPE_SUCCESS) {
    /* Status will be errored if there are no handles in the table but this is
     * okay, just return. */
    return;
  }
  for (unsigned i = 0; i < exm_tbl->num_tbls; i++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[i];
    if (exm_tbl->symmetric) {
      if (virtual_dev) {
        if (exm_tbl_data->default_entry_placed) {
          aim_printf(&uc->pvs,
                     "Default entry hdl for ALL PIPES : %d\n",
                     exm_tbl_data->default_entry_hdl);
        }
      } else {
        if (exm_tbl_data->default_entry_installed) {
          aim_printf(&uc->pvs,
                     "Default entry hdl for ALL PIPES : %d\n",
                     exm_tbl_data->default_entry_hdl);
        }
      }
    } else {
      if (virtual_dev) {
        if (exm_tbl_data->default_entry_placed) {
          aim_printf(&uc->pvs,
                     "Default entry hdl for pipe %d : %d\n",
                     exm_tbl_data->pipe_id,
                     exm_tbl_data->default_entry_hdl);
        }
      } else {
        if (exm_tbl_data->default_entry_installed) {
          aim_printf(&uc->pvs,
                     "Default entry hdl for pipe %d : %d\n",
                     exm_tbl_data->pipe_id,
                     exm_tbl_data->default_entry_hdl);
        }
      }
    }
  }

  /* Match memory does not exist for hash action tables and there are zero ways
   * so per-way occupancy needs special handling. */
  is_hash_action = pipe_mgr_mat_tbl_is_hash_action(device_id, mat_tbl_hdl);

  if (show_entry_handles) {
    if (virtual_dev || is_hash_action) {
      aim_printf(&uc->pvs,
                 "%-10s|%-10s|%-5s|%-5s|%-10s\n",
                 "Entry Hdl",
                 "Pipe",
                 "Stage",
                 "Way",
                 "Logical idx)");

      aim_printf(&uc->pvs,
                 "%-10s|%-10s|%-5s|%-5s|%-10s\n",
                 "----------",
                 "----------",
                 "-----",
                 "-----",
                 "----------");
    } else {
      aim_printf(&uc->pvs,
                 "%-10s|%-10s|%-5s|%-5s|%-10s|%-10s\n",
                 "Entry Hdl",
                 "Pipe",
                 "Stage",
                 "Way",
                 "Mem-id(s)",
                 "Line-no");

      aim_printf(&uc->pvs,
                 "%-10s|%-10s|%-5s|%-5s|%-10s|%-10s\n",
                 "----------",
                 "----------",
                 "-----",
                 "-----",
                 "----------",
                 "----------");
    }
  }

  int ***way_occupancy = NULL;
  way_occupancy = PIPE_MGR_CALLOC(exm_tbl->num_tbls, sizeof *way_occupancy);
  if (!way_occupancy) goto done;
  for (unsigned i = 0; i < exm_tbl->num_tbls; ++i) {
    way_occupancy[i] = PIPE_MGR_CALLOC(exm_tbl->exm_tbl_data[i].num_stages,
                                       sizeof **way_occupancy);
    if (!way_occupancy[i]) goto done;
    for (unsigned j = 0; j < exm_tbl->exm_tbl_data[i].num_stages; ++j) {
      int num_ways = exm_tbl->exm_tbl_data[i].exm_stage_info[j].num_hash_ways;
      if (is_hash_action) num_ways = 1;
      way_occupancy[i][j] = PIPE_MGR_CALLOC(num_ways, sizeof ***way_occupancy);
      if (!way_occupancy[i][j]) goto done;
    }
  }

  while (ent_hdl != -1) {
    char mem_id_str[20] = "";
    unsigned long key = (pipe_mat_ent_hdl_t)ent_hdl;
    pipe_mgr_exm_stage_info_t *exm_tbl_stage_info = NULL;
    bf_dev_pipe_t pipe_id = 0;
    dev_stage_t stage_id = 0;
    unsigned way_id = 0;

    if (virtual_dev || is_hash_action) {
      pipe_mgr_exm_entry_info_t *entry_info =
          pipe_mgr_exm_get_entry_info(exm_tbl, (pipe_mat_ent_hdl_t)ent_hdl);
      if (entry_info == NULL) {
        LOG_ERROR(
            "%s:%d Error in getting entry info for entry hdl %lu, tbl 0x%x",
            __func__,
            __LINE__,
            key,
            exm_tbl->mat_tbl_hdl);
        break;
      }
      pipe_id = entry_info->pipe_id;
      stage_id = entry_info->stage_id;
      exm_tbl_stage_info =
          pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
      if (exm_tbl_stage_info == NULL) {
        LOG_ERROR(
            "%s:%d Error in getting exm tbl stage info for tbl 0x%x, pipe id "
            "%d stage id %d",
            __func__,
            __LINE__,
            mat_tbl_hdl,
            pipe_id,
            stage_id);
        break;
      }
      /* For hash action tables leave it at way zero. */
      if (!is_hash_action) {
        way_id = pipe_mgr_exm_get_entry_hashway(exm_tbl_stage_info,
                                                entry_info->entry_idx);
      }

      /* Now, do the print */
      if (show_entry_handles) {
        if (pipe_id == BF_DEV_PIPE_ALL) {
          aim_printf(&uc->pvs,
                     "0x%-8x|ALL PIPES |%5d|%5d|%5d\n",
                     (pipe_mat_ent_hdl_t)ent_hdl,
                     stage_id,
                     way_id,
                     entry_info->entry_idx);
        } else {
          aim_printf(&uc->pvs,
                     "0x%-8x|%10d|%5d|%5d|%5d\n",
                     (pipe_mat_ent_hdl_t)ent_hdl,
                     pipe_id,
                     stage_id,
                     way_id,
                     entry_info->entry_idx);
        }
      }
    } else {
      pipe_mgr_exm_phy_entry_info_t *entry_info =
          pipe_mgr_exm_get_phy_entry_info(exm_tbl, key);
      if (entry_info == NULL) {
        LOG_ERROR(
            "%s:%d Error in getting entry info for entry hdl %lu, tbl 0x%x",
            __func__,
            __LINE__,
            key,
            exm_tbl->mat_tbl_hdl);
        break;
      }

      pipe_id = entry_info->pipe_id;
      stage_id = entry_info->stage_id;
      for (unsigned i = 0; i < entry_info->num_ram_units; i++) {
        char this_str[5] = "";
        int n = snprintf(this_str, 5, "%d,", entry_info->mem_id[i]);
        if (n > 0 && n < 5) {
          strncat(mem_id_str, this_str, 5);
        }
      }
      exm_tbl_stage_info =
          pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
      if (exm_tbl_stage_info == NULL) {
        LOG_ERROR(
            "%s:%d Error in getting exm tbl stage info for tbl 0x%x, pipe id "
            "%d stage id %d",
            __func__,
            __LINE__,
            mat_tbl_hdl,
            pipe_id,
            stage_id);
        break;
      }
      way_id = pipe_mgr_exm_get_entry_hashway(exm_tbl_stage_info,
                                              entry_info->entry_idx);
      ram_line_num = pipe_mgr_exm_compute_ram_line_num(exm_tbl_stage_info,
                                                       entry_info->entry_idx);

      if (ram_line_num >= TOF_SRAM_UNIT_DEPTH) {
        LOG_ERROR("%s:%d Error in getting ram line num for ent %lu, tbl 0x%x",
                  __func__,
                  __LINE__,
                  key,
                  mat_tbl_hdl);
        break;
      }
      /* Now, do the print */
      if (show_entry_handles) {
        if (pipe_id == BF_DEV_PIPE_ALL) {
          aim_printf(&uc->pvs,
                     "0x%-8x|ALL PIPES |%5d|%5d|%-10s|%5d\n",
                     (pipe_mat_ent_hdl_t)ent_hdl,
                     stage_id,
                     way_id,
                     mem_id_str,
                     ram_line_num);
        } else {
          aim_printf(&uc->pvs,
                     "0x%-8x|%10d|%5d|%5d|%-10s|%5d\n",
                     (pipe_mat_ent_hdl_t)ent_hdl,
                     pipe_id,
                     stage_id,
                     way_id,
                     mem_id_str,
                     ram_line_num);
        }
      }
    }

    for (unsigned i = 0; i < exm_tbl->num_tbls; ++i) {
      if (exm_tbl->exm_tbl_data[i].pipe_id != pipe_id) continue;
      for (unsigned j = 0; j < exm_tbl->exm_tbl_data[i].num_stages; ++j) {
        if (exm_tbl->exm_tbl_data[i].exm_stage_info[j].stage_id != stage_id)
          continue;
        if (!is_hash_action &&
            way_id >=
                exm_tbl->exm_tbl_data[i].exm_stage_info[j].num_hash_ways) {
          LOG_ERROR(
              "Unexpected hash way %d, table 0x%x, pipe %X, stage %d, entry %u "
              "num hash ways %d",
              way_id,
              mat_tbl_hdl,
              pipe_id,
              stage_id,
              ent_hdl,
              exm_tbl->exm_tbl_data[i].exm_stage_info[j].num_hash_ways);
        } else {
          ++way_occupancy[i][j][way_id];
        }
      }
    }

    if (virtual_dev) {
      status = pipe_mgr_exm_get_next_placed_entry_handles(
          mat_tbl_hdl, dev_tgt, ent_hdl, 1, &ent_hdl);
    } else {
      status = pipe_mgr_exm_get_next_programmed_entry_handles(
          mat_tbl_hdl, dev_tgt, ent_hdl, 1, &ent_hdl);
    }
    if (status != PIPE_SUCCESS) {
      /* Status will be errored if there are no handles in the table but this is
       * okay, just return. */
      break;
    }
  }

done:
  /* Display the occupancy and release the memory. */
  if (way_occupancy) {
    for (unsigned i = 0; i < exm_tbl->num_tbls; ++i) {
      if (way_occupancy[i]) {
        for (unsigned j = 0; j < exm_tbl->exm_tbl_data[i].num_stages; ++j) {
          if (way_occupancy[i][j]) {
            int num_ways =
                exm_tbl->exm_tbl_data[i].exm_stage_info[j].num_hash_ways;
            if (is_hash_action) num_ways = 1;
            for (int k = 0; k < num_ways; ++k) {
              int way_size;
              if (is_hash_action) {
                way_size =
                    exm_tbl->exm_tbl_data[i].exm_stage_info[j].num_entries;
              } else {
                way_size = exm_tbl->exm_tbl_data[i]
                               .exm_stage_info[j]
                               .hashway_data[k]
                               .num_entries;
              }
              if (way_size == 0) way_size = 1;
              aim_printf(
                  &uc->pvs,
                  "Instance %X Stage %2d Way %d: %6d / %6d Entries, %3.2f%%\n",
                  exm_tbl->exm_tbl_data[i].pipe_id,
                  exm_tbl->exm_tbl_data[i].exm_stage_info[j].stage_id,
                  k,
                  way_occupancy[i][j][k],
                  way_size,
                  (100.0 * way_occupancy[i][j][k]) / (float)way_size);
            }
            PIPE_MGR_FREE(way_occupancy[i][j]);
          }
        }
        PIPE_MGR_FREE(way_occupancy[i]);
      }
    }
    PIPE_MGR_FREE(way_occupancy);
  }

  return;
}

void pipe_mgr_exm_dump_entry_info(ucli_context_t *uc,
                                  uint8_t device_id,
                                  pipe_mat_tbl_hdl_t exm_tbl_hdl,
                                  pipe_mat_ent_hdl_t entry_hdl) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_stage_info_t *exm_tbl_stage_info = NULL;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  pipe_tbl_match_spec_t *match_spec = NULL;
  pipe_action_spec_t *action_spec = NULL;

  uint32_t num_entries_per_wide_word = 0;
  uint32_t ram_line_num = 0;
  char buf[1000];
  char buf1[1000];
  size_t bytes_written = 0;

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, exm_tbl_hdl);

  if (exm_tbl == NULL) {
    aim_printf(&uc->pvs, "No information found for the table\n");
    return;
  }

  aim_printf(&uc->pvs,
             "Info for entry handle %d for exact match table %d\n",
             entry_hdl,
             exm_tbl_hdl);

  entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, entry_hdl);
  if (entry_info == NULL) {
    aim_printf(
        &uc->pvs,
        "Entry info for entry handle %d, tbl 0x%x, device id %d not found",
        entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    return;
  }
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, entry_info->pipe_id);
  if (!exm_tbl_data) {
    aim_printf(&uc->pvs,
               "Device %d table 0x%x, instance not found for pipe %x",
               exm_tbl->dev_id,
               exm_tbl->mat_tbl_hdl,
               entry_info->pipe_id);
    return;
  }

  if (entry_hdl == exm_tbl_data->default_entry_hdl) {
    aim_printf(&uc->pvs, "This is the default entry\n");
  } else {
    match_spec = unpack_mat_ent_data_ms(entry_info->entry_data);
    /* Print the entry match spec in P4 terms */
    pipe_mgr_entry_format_print_match_spec(device_id,
                                           exm_tbl->profile_id,
                                           exm_tbl_hdl,
                                           match_spec,
                                           buf,
                                           sizeof(buf),
                                           &bytes_written);
    aim_printf(&uc->pvs, "\tMatch Spec :\n \t-----------------\n%s\n", buf);
  }
  action_spec = unpack_mat_ent_data_as(entry_info->entry_data);
  if (!action_spec) {
    aim_printf(&uc->pvs,
               "Could not get action spec for entry handle %d tbl 0x%x",
               entry_hdl,
               exm_tbl_hdl);
    return;
  }

  if (action_spec->pipe_action_datatype_bmap & PIPE_ACTION_DATA_TYPE) {
    pipe_mgr_entry_format_print_action_spec(
        device_id,
        exm_tbl->profile_id,
        &action_spec->act_data,
        unpack_mat_ent_data_afun_hdl(entry_info->entry_data),
        buf1,
        sizeof(buf1),
        &bytes_written);

    aim_printf(&uc->pvs, "\tAction Spec: \n \t-----------------\n%s\n", buf1);
  } else if (IS_ACTION_SPEC_SEL_GRP(action_spec)) {
    aim_printf(
        &uc->pvs, "\tSelector group handle : %d\n", action_spec->sel_grp_hdl);
    aim_printf(&uc->pvs,
               "\tPort vector table len : %d\n",
               unpack_mat_ent_data_sel(entry_info->entry_data));
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(action_spec)) {
    aim_printf(
        &uc->pvs, "\t Action entry handle : %d\n", action_spec->adt_ent_hdl);
  }
  /* Print any resources attached */
  if (action_spec) {
    resource_trace(exm_tbl->dev_id,
                   action_spec->resources,
                   action_spec->resource_count,
                   buf1,
                   sizeof(buf1));
    aim_printf(&uc->pvs, "\t %s\n", buf1);
  }
  aim_printf(&uc->pvs,
             "\tAction function handle : 0x%x\n",
             unpack_mat_ent_data_afun_hdl(entry_info->entry_data));
  if (entry_hdl != exm_tbl_data->default_entry_hdl) {
    aim_printf(&uc->pvs, "\tEntry location info:\n");
    if (entry_info->pipe_id == BF_DEV_PIPE_ALL) {
      aim_printf(&uc->pvs, "\t\tPipe id : ALL PIPES\n");
    } else {
      aim_printf(&uc->pvs, "\t\tPipe id : %d\n", entry_info->pipe_id);
    }

    aim_printf(&uc->pvs, "\t\tStage id : %d\n", entry_info->stage_id);
    aim_printf(
        &uc->pvs, "\t\tLogical entry index : %d\n", entry_info->entry_idx);
    exm_tbl_stage_info = pipe_mgr_exm_tbl_get_stage_info(
        exm_tbl, entry_info->pipe_id, entry_info->stage_id);
    if (!exm_tbl_stage_info) {
      aim_printf(&uc->pvs,
                 "Could not get exm tbl stage info for tbl 0x%x, "
                 "pipe id %d, stage id 0x%x",
                 exm_tbl_hdl,
                 entry_info->pipe_id,
                 entry_info->stage_id);
      return;
    }

    num_entries_per_wide_word =
        exm_tbl_stage_info->pack_format->num_entries_per_wide_word;

    if (num_entries_per_wide_word) {
      ram_line_num = (entry_info->entry_idx / num_entries_per_wide_word) %
                     TOF_SRAM_UNIT_DEPTH;
      aim_printf(&uc->pvs, "\t\tRam line num : %d\n", ram_line_num);
    } else {
      aim_printf(&uc->pvs, "\t\tRam line num does not exist \n");
    }
  }
  return;
}

void pipe_mgr_exm_dump_phy_entry_info(ucli_context_t *uc,
                                      bf_dev_id_t device_id,
                                      pipe_mat_tbl_hdl_t exm_tbl_hdl,
                                      pipe_mat_ent_hdl_t entry_hdl) {
  pipe_mgr_exm_stage_info_t *exm_tbl_stage_info;
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data;
  bf_map_sts_t map_sts = BF_MAP_NO_KEY;
  pipe_mgr_exm_tbl_t *exm_tbl;
  bool is_hash_action = false;
  uint32_t i;

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, exm_tbl_hdl);
  if (exm_tbl == NULL) {
    aim_printf(&uc->pvs,
               "Exm table 0x%x, for device id %d not found",
               exm_tbl_hdl,
               device_id);
    return;
  }

  /* Get the physical entry info from the entry's table instance */

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance_from_entry(
      exm_tbl, entry_hdl, __func__, __LINE__);
  if (!exm_tbl_data) {
    aim_printf(&uc->pvs,
               "Table instance for exm table 0x%x, for device id %d not found",
               exm_tbl_hdl,
               device_id);
    return;
  }

  map_sts = bf_map_get(
      &exm_tbl_data->entry_phy_info_htbl, entry_hdl, (void **)&entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    aim_printf(
        &uc->pvs,
        "Entry info for entry handle %d, tbl 0x%x, device id %d not found",
        entry_hdl,
        exm_tbl_hdl,
        device_id);
    return;
  } else if (map_sts != BF_MAP_OK) {
    /* Unexpected error */
    aim_printf(&uc->pvs,
               "Error in getting entry info for entry handle %d, tbl 0x%x, "
               "device id %d, err 0x%x",
               entry_hdl,
               exm_tbl_hdl,
               device_id,
               map_sts);
    return;
  }

  exm_tbl_stage_info = pipe_mgr_exm_tbl_get_stage_info(
      exm_tbl, entry_info->pipe_id, entry_info->stage_id);
  if (!exm_tbl_stage_info) {
    aim_printf(&uc->pvs,
               "Could not get exm tbl stage info for tbl 0x%x, "
               "pipe id %d, stage id 0x%x",
               exm_tbl_hdl,
               entry_info->pipe_id,
               entry_info->stage_id);
    return;
  }
  if (entry_info->pipe_id == BF_DEV_PIPE_ALL) {
    aim_printf(&uc->pvs, "\t\t Pipe id : ALL PIPES\n");
  } else {
    aim_printf(&uc->pvs, "\t\t Pipe id : %d\n", entry_info->pipe_id);
  }
  aim_printf(&uc->pvs, "\t\t Stage id : %d\n", entry_info->stage_id);
  aim_printf(&uc->pvs, "\t\tNum ram units : %d\n", entry_info->num_ram_units);
  aim_printf(&uc->pvs, "\t\t");
  for (i = 0; i < entry_info->num_ram_units; i++) {
    aim_printf(&uc->pvs, "%d,", entry_info->mem_id[i]);
  }

  is_hash_action =
      pipe_mgr_mat_tbl_is_hash_action(device_id, exm_tbl->mat_tbl_hdl);
  if (is_hash_action) {
    /* Match memory does not exist for hash action tables. Also there cannot
       be any indirectly addressed resources. Hence just return from this
       point for hash action tables */
    aim_printf(&uc->pvs,
               "Hash action table does not have any location information \n");
    return;
  }
  uint32_t num_entries_per_wide_word =
      exm_tbl_stage_info->pack_format->num_entries_per_wide_word;

  uint32_t ram_line_num =
      (entry_info->entry_idx / num_entries_per_wide_word) % TOF_SRAM_UNIT_DEPTH;
  aim_printf(&uc->pvs, "\n\t\tRam line num : %d\n", ram_line_num);

  aim_printf(&uc->pvs, "\n");
  if (exm_tbl->num_sel_tbl_refs != 0) {
    aim_printf(&uc->pvs,
               "\tSelector group virtual address : 0x%x\n",
               entry_info->indirect_ptrs.sel_ptr);
  }
  if (exm_tbl->num_adt_refs != 0) {
    /* The action entry handle abstraction exists only for indirectly addressed
     * action tables.
     */
    if (exm_tbl->adt_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      aim_printf(&uc->pvs,
                 "\tAction data entry handle : %d\n",
                 entry_info->adt_ent_hdl);
      aim_printf(&uc->pvs,
                 "\tAction data base virtual address : 0x%x\n",
                 entry_info->indirect_ptrs.adt_ptr);
    }
  }
  /* Print indirect addresses for meters/stats/stateful */
  for (i = 0; i < exm_tbl->num_stat_tbl_refs; i++) {
    /* Currently one match table can only refer to one stats table, but keeping
     * this code durable for a day where there can be more than one reference
     */
    if (exm_tbl->stat_tbl_refs[i].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      aim_printf(&uc->pvs,
                 "\tStats address : 0x%x\n",
                 entry_info->indirect_ptrs.stats_ptr);
    }
  }

  for (i = 0; i < exm_tbl->num_meter_tbl_refs; i++) {
    if (exm_tbl->meter_tbl_refs[i].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      aim_printf(&uc->pvs,
                 "\tMeters address : 0x%x\n",
                 entry_info->indirect_ptrs.meter_ptr);
    }
  }

  for (i = 0; i < exm_tbl->num_stful_tbl_refs; i++) {
    if (exm_tbl->stful_tbl_refs[i].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      aim_printf(&uc->pvs,
                 "\tStful address : 0x%x\n",
                 entry_info->indirect_ptrs.stfl_ptr);
    }
  }
  return;
}

void pipe_mgr_exm_entry_move_stats_clear(ucli_context_t *uc,
                                         bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  uint32_t stage, i, j, k;
  pipe_mgr_exm_stage_info_t *stage_info = NULL;
  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    if (uc) aim_printf(&uc->pvs, "No information found for table\n");
    return;
  }
  if (pipe_mgr_mat_tbl_is_hash_action(dev_id, mat_tbl_hdl)) {
    if (uc) aim_printf(&uc->pvs, "Hash Action table does not support!\n");
    return;
  }
  for (i = 0; i < exm_tbl->num_tbls; ++i) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[i];
    for (j = 0; j < exm_tbl_data->num_stages; j++) {
      stage_info = &exm_tbl_data->exm_stage_info[j];
      stage = stage_info->stage_id;
      exm_tbl_data->entry_stats.failed[stage] = 0;
      for (k = 0; k < CUCKOO_MAX_NUM_MOVES + 1; k++) {
        exm_tbl_data->entry_stats.stage_stats[stage].moves[k] = 0;
      }
    }
    exm_tbl_data->entry_stats.total_failed = 0;
  }
  if (uc) aim_printf(&uc->pvs, "Entry Move Stats Cleared\n");
  return;
}

void pipe_mgr_exm_entry_move_stats_dump(ucli_context_t *uc,
                                        bf_dev_id_t dev_id,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  uint32_t stage, i, j, k;
  pipe_mgr_exm_stage_info_t *stage_info = NULL;
  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    if (uc) aim_printf(&uc->pvs, "No information found for table\n");
    return;
  }
  if (pipe_mgr_mat_tbl_is_hash_action(dev_id, mat_tbl_hdl)) {
    if (uc) aim_printf(&uc->pvs, "Hash Action table does not support!\n");
    return;
  }
  aim_printf(&uc->pvs, "Entry Move Stats : \n");
  for (i = 0; i < exm_tbl->num_tbls; ++i) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[i];
    for (j = 0; j < exm_tbl_data->num_stages; j++) {
      stage_info = &exm_tbl_data->exm_stage_info[j];
      stage = stage_info->stage_id;
      aim_printf(&uc->pvs, "Pipe:%d Stage:%d\n", i, stage);
      aim_printf(&uc->pvs, "Move(s)  \t Entries \n");
      for (k = 0; k < CUCKOO_MAX_NUM_MOVES + 1; k++) {
        aim_printf(&uc->pvs,
                   " %d \t \t %d\n",
                   k,
                   exm_tbl_data->entry_stats.stage_stats[stage].moves[k]);
      }
      aim_printf(&uc->pvs,
                 " Num failed attempts in stage %d : %d \n",
                 stage,
                 exm_tbl_data->entry_stats.failed[stage]);
    }
    aim_printf(&uc->pvs,
               "Total failed entry adds on pipe %d: %d \n",
               i,
               exm_tbl_data->entry_stats.total_failed);
  }
  return;
}
