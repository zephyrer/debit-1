/*
 * Copyright (C) 2006, 2007 Jean-Baptiste Note <jean-baptiste.note@m4x.org>
 *
 * This file is part of debit.
 *
 * Debit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Debit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with debit.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DESIGN_V5_H
#define _DESIGN_V5_H

#include <stdint.h>
#include "bitstream_parser.h"

#define CHIP "virtex5"

typedef enum _id_v5 {
  XC5VLX30 = 0,
  XC5VLX50, XC5VLX85,
  XC5VLX110, XC5VLX220,
  XC5VLX330, XC5VLX__NUM,
} id_v5vlx_t;

/*
 * FAR Register implementation
 */

/*
 * FAR register -- hardware
 */

/* TODO: get rid of this. Higly non-portable */
typedef struct {
  unsigned int mna :7;
  unsigned int col :8;
  unsigned int row :5;
  unsigned int tb :1;
  unsigned int type :3;
} v5_frame_addr_t;

#define FAR_V5_MNA_OFFSET 0
#define FAR_V5_MNA_LEN 7
#define FAR_V5_MNA_MASK ((1<<FAR_V5_MNA_LEN) - 1) << FAR_V5_MNA_OFFSET

#define FAR_V5_COL_OFFSET (FAR_V5_MNA_OFFSET + FAR_V5_MNA_LEN)
#define FAR_V5_COL_LEN 8
#define FAR_V5_COL_MASK ((1<<FAR_V5_COL_LEN) - 1) << FAR_V5_COL_OFFSET

#define FAR_V5_ROW_OFFSET (FAR_V5_COL_OFFSET + FAR_V5_COL_LEN)
#define FAR_V5_ROW_LEN 5
#define FAR_V5_ROW_MASK ((1<<FAR_V5_ROW_LEN) - 1) << FAR_V5_ROW_OFFSET

#define FAR_V5_TB_OFFSET (FAR_V5_ROW_OFFSET + FAR_V5_ROW_LEN)
#define FAR_V5_TB_LEN 1
#define FAR_V5_TB_MASK ((1<<FAR_V5_TB_LEN) - 1) << FAR_V5_TB_OFFSET

#define FAR_V5_TYPE_OFFSET (FAR_V5_TB_OFFSET + FAR_V5_TB_LEN)
#define FAR_V5_TYPE_LEN 3
#define FAR_V5_TYPE_MASK ((1<<FAR_V5_TYPE_LEN) - 1) << FAR_V5_TYPE_OFFSET

typedef enum _v5_col_type {
  V5_TYPE_CLB = 0,
  V5_TYPE_BRAM,
  V5_TYPE_CFG_CLB,
  V5__NB_COL_TYPES,
} v5_col_type_t;

/* V5 bitstreams typically don't include V5_TYPE_CFG_CLB frames */
#define LAST_COL_TYPE V5_TYPE_BRAM

typedef enum _v5_tb_t {
  V5_TB_TOP = 0,
  V5_TB_BOTTOM,
} v5_tb_t;

static inline unsigned
v5_mna_of_far(const uint32_t far) {
	return (far & FAR_V5_MNA_MASK) >> FAR_V5_MNA_OFFSET;
}

static inline unsigned
v5_col_of_far(const uint32_t far) {
	return (far & FAR_V5_COL_MASK) >> FAR_V5_COL_OFFSET;
}

static inline unsigned
v5_row_of_far(const uint32_t far) {
	return (far & FAR_V5_ROW_MASK) >> FAR_V5_ROW_OFFSET;
}

static inline unsigned
v5_tb_of_far(const uint32_t far) {
	return (far & FAR_V5_TB_MASK) >> FAR_V5_TB_OFFSET;
}

static inline unsigned
v5_type_of_far(const uint32_t far) {
	return (far & FAR_V5_TYPE_MASK) >> FAR_V5_TYPE_OFFSET;
}

/*
 * FAR register -- our more simple software vision of it
 */

typedef struct sw_far_v5 {
  unsigned tb;
  unsigned type;
  unsigned row;
  unsigned col;
  unsigned mna;
} sw_far_v5_t;

static inline void
fill_swfar_v5(sw_far_v5_t *sw_far, const uint32_t hwfar) {
  sw_far->tb = v5_tb_of_far(hwfar);
  sw_far->type = v5_type_of_far(hwfar);
  sw_far->row = v5_row_of_far(hwfar);
  sw_far->col = v5_col_of_far(hwfar);
  sw_far->mna = v5_mna_of_far(hwfar);
}

static inline uint32_t
get_hwfar_v5(const sw_far_v5_t *sw_far) {
  return (sw_far->mna +
	  (sw_far->col << FAR_V5_COL_OFFSET) +
	  (sw_far->row << FAR_V5_ROW_OFFSET) +
	  (sw_far->type << FAR_V5_TYPE_OFFSET) +
	  (sw_far->tb << FAR_V5_TB_OFFSET));
}

typedef enum {
  V5C_IOB = 0,
  V5C_CLB,
  V5C_DSP48,
  V5C_GCLK,
  V5C_BRAM_INT,
  V5C_BRAM,
  V5C_PAD,
  V5C__NB_CFG,
} v5_design_col_t;

typedef struct _chip_struct_v5 {
  id_v5vlx_t chip;
  guint32 idcode;
  guint32 framelen;
  const unsigned *frame_count;
  const unsigned col_count[V5__NB_COL_TYPES];
  unsigned bram_count;
  unsigned row_count;
  const v5_design_col_t *col_type;
} chip_struct_t;

static inline unsigned
type_frame_count(const chip_struct_t *chip,
		 const v5_design_col_t type) {
  return chip->frame_count[type];
}

#define design_col_t v5_design_col_t

static inline unsigned
type_col_count_v5(const unsigned *col_count,
		  const v5_design_col_t type) {
  switch (type) {
  case V5C_IOB:
    return 3;
  case V5C_GCLK:
    return 1;
  case V5C_DSP48:
    if (col_count[V5_TYPE_BRAM] >= 6)
      return 2;
    return 1;
  case V5C_CLB:
    if (col_count[V5_TYPE_BRAM] >= 6)
      return col_count[V5_TYPE_CLB] - 6 - col_count[V5_TYPE_BRAM];
    return col_count[V5_TYPE_CLB] - 5 - col_count[V5_TYPE_BRAM];
  case V5C_BRAM:
    return col_count[V5_TYPE_BRAM];
  case V5C_BRAM_INT:
    return col_count[V5_TYPE_BRAM];
  case V5C_PAD:
    return V5__NB_COL_TYPES-1;
  case V5C__NB_CFG:
    /* return the total ? */
  default:
    g_assert_not_reached();
  }
  return 0;
}

#define type_col_count type_col_count_v5
#define V__NB_CFG V5C__NB_CFG

#include "design_common.h"

#endif /* design_v5.h */
