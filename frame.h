#ifndef FRAME_H__
#define FRAME_H__

#define FRM_TABLE_NAME_OFFSET           0
#define FRM_TABLE_NAME_SIZE             16
#define FRM_NUM_COLS_OFFSET             FRM_TABLE_NAME_SIZE
#define FRM_NUM_COLS_SIZE           	sizeof(size_t)
#define FRM_FIRST_COL_OFFSET            (FRM_NUM_COLS_OFFSET + FRM_NUM_COLS_SIZE)
#define FRM_COL_NAME_SIZE               64
#define FRM_COL_TYPE_SIZE               16
#define FRM_COL_INDEX_FLAG_SIZE         1
#define FRM_COL_SIZE                    (FRM_COL_NAME_SIZE + FRM_COL_TYPE_SIZE + FRM_COL_INDEX_FLAG_SIZE)
#define FRM_COL_NAME_OFFSET(i)          (FRM_FIRST_COL_OFFSET + i * FRM_COL_SIZE)
#define FRM_COL_TYPE_OFFSET(i)          (FRM_COL_NAME_OFFSET(i) + FRM_COL_NAME_SIZE)
#define FRM_COL_INDEX_FLAG_OFFSET(i)    (FRM_COL_TYPE_OFFSET(i) + FRM_COL_TYPE_SIZE)
#define FRM_SIZE(num_cols)              (FRM_FIRST_COL_OFFSET + num_cols * FRM_COL_SIZE)

#endif
