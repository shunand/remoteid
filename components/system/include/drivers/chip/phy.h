#ifndef __PHY_H__
#define __PHY_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Defines the PHY link speed. This is align with the speed for MAC. */
enum phy_speed
{
    PHY_SPEED_10M = 0U, /* PHY 10M speed. */
    PHY_SPEED_100M      /* PHY 100M speed. */
};

/* Defines the PHY link duplex. */
enum phy_duplex
{
    PHY_HALF_DUPLEX = 0U, /* PHY half duplex. */
    PHY_FULL_DUPLEX       /* PHY full duplex. */
};

/*! @brief Defines the PHY loopback mode. */
enum phy_loop
{
    PHY_LOCAL_LOOP = 0U, /* PHY local loopback. */
    PHY_REMOTE_LOOP      /* PHY remote loopback. */
};

typedef struct
{
    enum phy_speed speed;
    enum phy_duplex duplex;
    enum phy_loop loop;
} phy_param_t;

int drv_phy_init(unsigned phy_addr, unsigned src_clock_hz, phy_param_t *param);
int drv_phy_read(unsigned reg, unsigned *data);
int drv_phy_write(unsigned reg, unsigned data);
int drv_phy_loopback(unsigned mode, unsigned speed, unsigned enable);
int drv_phy_get_link_status(unsigned *status);
int drv_phy_get_link_speed_duplex(unsigned *speed, unsigned *duplex);

#ifdef __cplusplus
}
#endif

#endif
