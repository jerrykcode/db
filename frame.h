#ifndef FRAME_H__
#define FRAME_H__

#define FRM_TABLE_NAME_OFFSET           0
#define FRM_TABLE_NAME_SIZE             16
#define FRM_NUM_KEYS_OFFSET             FRM_TABLE_NAME_SIZE
#define FRM_NUM_KEYS_SIZE           	sizeof(size_t)
#define FRM_FIRST_KEY_OFFSET            (FRM_NUM_KEYS_OFFSET + FRM_NUM_KEYS_SIZE)
#define FRM_KEY_NAME_SIZE               64
#define FRM_KEY_TYPE_SIZE               16
#define FRM_KEY_SIZE                    (FRM_KEY_NAME_SIZE + FRM_KEY_TYPE_SIZE)
#define FRM_KEY_NAME_OFFSET(i)          (FRM_FIRST_KEY_OFFSET + i * FRM_KEY_SIZE)
#define FRM_KEY_TYPE_OFFSET(i)          (FRM_KEY_NAME_OFFSET(i) + FRM_KEY_TYPE_SIZE)
#define FRM_SIZE(num_keys)              (FRM_FIRST_KEY_OFFSET + num_keys * FRM_KEY_SIZE)

#endif
