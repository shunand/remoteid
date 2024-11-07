#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <canard.h>


#define COM_HOBBYWING_ESC_SETANGLE_RESPONSE_MAX_SIZE 6
#define COM_HOBBYWING_ESC_SETANGLE_RESPONSE_SIGNATURE (0x81D9B10761C28E0AULL)
#define COM_HOBBYWING_ESC_SETANGLE_RESPONSE_ID 217

#if defined(__cplusplus) && defined(DRONECAN_CXX_WRAPPERS)
class com_hobbywing_esc_SetAngle_cxx_iface;
#endif

struct com_hobbywing_esc_SetAngleResponse {
#if defined(__cplusplus) && defined(DRONECAN_CXX_WRAPPERS)
    using cxx_iface = com_hobbywing_esc_SetAngle_cxx_iface;
#endif
    uint8_t option;
    struct { uint8_t len; uint16_t data[2]; }angle;
};

#ifdef __cplusplus
extern "C"
{
#endif

uint32_t com_hobbywing_esc_SetAngleResponse_encode(struct com_hobbywing_esc_SetAngleResponse* msg, uint8_t* buffer
#if CANARD_ENABLE_TAO_OPTION
    , bool tao
#endif
);
bool com_hobbywing_esc_SetAngleResponse_decode(const CanardRxTransfer* transfer, struct com_hobbywing_esc_SetAngleResponse* msg);

#if defined(CANARD_DSDLC_INTERNAL)
static inline void _com_hobbywing_esc_SetAngleResponse_encode(uint8_t* buffer, uint32_t* bit_ofs, struct com_hobbywing_esc_SetAngleResponse* msg, bool tao);
static inline void _com_hobbywing_esc_SetAngleResponse_decode(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct com_hobbywing_esc_SetAngleResponse* msg, bool tao);
void _com_hobbywing_esc_SetAngleResponse_encode(uint8_t* buffer, uint32_t* bit_ofs, struct com_hobbywing_esc_SetAngleResponse* msg, bool tao) {
    (void)buffer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;

    canardEncodeScalar(buffer, *bit_ofs, 8, &msg->option);
    *bit_ofs += 8;
    if (!tao) {
        canardEncodeScalar(buffer, *bit_ofs, 2, &msg->angle.len);
        *bit_ofs += 2;
    }
    for (size_t i=0; i < msg->angle.len; i++) {
        canardEncodeScalar(buffer, *bit_ofs, 16, &msg->angle.data[i]);
        *bit_ofs += 16;
    }
}

void _com_hobbywing_esc_SetAngleResponse_decode(const CanardRxTransfer* transfer, uint32_t* bit_ofs, struct com_hobbywing_esc_SetAngleResponse* msg, bool tao) {
    (void)transfer;
    (void)bit_ofs;
    (void)msg;
    (void)tao;

    canardDecodeScalar(transfer, *bit_ofs, 8, false, &msg->option);
    *bit_ofs += 8;

    if (!tao) {
        canardDecodeScalar(transfer, *bit_ofs, 2, false, &msg->angle.len);
        *bit_ofs += 2;
    } else {
        msg->angle.len = ((transfer->payload_len*8)-*bit_ofs)/16;
    }

    for (size_t i=0; i < msg->angle.len; i++) {
        canardDecodeScalar(transfer, *bit_ofs, 16, false, &msg->angle.data[i]);
        *bit_ofs += 16;
    }

}
#endif
#ifdef CANARD_DSDLC_TEST_BUILD
struct com_hobbywing_esc_SetAngleResponse sample_com_hobbywing_esc_SetAngleResponse_msg(void);
#endif
#ifdef __cplusplus
} // extern "C"

#ifdef DRONECAN_CXX_WRAPPERS
#include <canard/cxx_wrappers.h>
#endif
#endif
