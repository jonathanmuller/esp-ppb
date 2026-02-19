#ifndef FTM_H
#define FTM_H

#include <stdint.h>

struct __attribute__((packed))ftm_action_hdr_t {
    uint8_t category;
    uint8_t action;
};


struct __attribute__((packed))ftm_dialog_t {
    struct ftm_action_hdr_t hdr;
    uint8_t dialog_token;
    uint8_t followup_token;
    uint8_t tod[6];
    uint8_t toa[6];
    uint16_t tod_err;
    uint16_t toa_err;
};


struct ftm_exchange {
    uint8_t dialog_token;
    uint64_t tod;
    uint64_t recv_ts;
};


#endif //FTM_H
