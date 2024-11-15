# 编译环境：
1. [ArduRemoteID] ( https://github.com/ArduPilot/ArduRemoteID)先拉取工程详情请参考他的buildingmd文件进行实现
2. [mavlink](https://mavlink.io/en/services/opendroneid.html.git)消息定义请参考
3. [DroneCAN](https://github.com/dronecan/DSDL/tree/master/dronecan/remoteid)DroneCAN请参考
4. [opendroneid-core-c](https://github.com/opendroneid/opendroneid-core-c/tree/5dc9df4283335b618fa4f12985aecdf46283f8f8)详细介绍了美国要求
5. [libcanard] (https://github.com/dronecan/libcanard)这个是libcanard库
6. [esp-idf-arduino](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/storage/partition.html)这份文档详细说明如何讲arduino做位esp-idf的组件
7. [esp32](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/storage/partition.html)
___
# 测试：
1. 硬件需求esp32cedev，带（串口）telem口的飞控该飞控ardupliot的remoteid进行使能
2. [opendroneid](https://ardupilot.org/dev/docs/opendroneid.html#opendroneid)
3. 需要在ardupilot/libraries/AP_HAL_ChibiOS/hwdef/创建文件路径 需要自己写三个文件才可启用ardupliot的remoteid的功能
~~~
{
DID_ENABLE 1
DID_OPTIONS 1
DID_MAVPORT7
DID_CANDRIVER 0
AHRS_EKF_TYPE 3
GPS_TYPE 1
GPS_TYPE2 0
SERIAL7_BAUD 57
SERIAL7_PROTOCOL 2（
}
~~~
~~~
{
include ../MatekH743/hwdef.dat
APJ_BOARD_ID 17411
define AP_OPENDRONEID_ENABLED 1
}
~~~
~~~
{
include ../MatekH743/hwdef-bl.dat
APJ_BOARD_ID 17411
define AP_OPENDRONEID_ENABLED 1
}
~~~
4. 软件测试请参考[goole](https://play.google.com/store/apps/details?id=org.opendroneid.android_osm)
5. 请注意查看手机型号是不是不支持https://github.com/opendroneid/receiver-android/blob/master/supported-smartphones.md
___
# 代码解读：
1. >函数接口openDriver()//这个接口是实现remoteid的函数入口 
2. >initArduino();//这个函数是remoteid的初始化Arduino的环境无需展开 
3. >WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);//这是esp的底层写寄存器的函数主要的目的是禁止过压保护，具体可以参考
4. >g.init();的实际原型是parameters.cpp里extern Parameters g;
他创建了一个对象使用对象里的公有函数进行初始化，他创建这个对象并extern的原因是他设定了
一些初始值在const Parameters::Param Parameters::params[] 这个数组里 
其中load_defaults();加载值作为初始化配置的，第二部分nvs_get_u8类似这些其实他想做的是
从 NVS 中加载实际数据，覆盖默认值这样可以做到恢复设备之前的状态，而这个函数esp_read_mac主要是 把MAC 地址读取到 mac 数组中，他其实想做的是生成WiFi名字
g.to_factory_defaults == 1指示设备是否需要恢复到出厂默认设置 set_by_name_char64他使用的是romfs这个是个文件系统他想做的是验证固件更新和安全更新参数（不重要）
5.led.set_state(Led::LedState::INIT);这个是灯的控制，这个函数搭配led.update();使用，也就说你设置状态之后需要进行更新这个灯才会刷新状态
6. > if (g.webserver_enable) {
        // need WiFi for web server
        wifi.init();
    }这个参数默认是使能的源头请参考const Parameters::Param Parameters::params[] 
7. >wifi.init();这个需要web服务没有移值，详情请跳转参考函数里面有函数解释
8. >Serial.begin(g.baudrate);这个是串口打印的主要用来debug，好像是为了和ardupliot传输设为56700，esp-idf是115200，可以设为115200来查看打印
9. >odid_initUasData(&UAS_data);这个函数主要是清0，做初始化
10. > mavlink1.init();打印初始化的版本信息 mavlink_system.sysid = g.mavlink_sysid;这是一个系统id，是0来的后面会应用到
11. >dronecan.init();//暂不使用没看
12. >set_efuses(); CheckFirmware::check_OTA_running();这两个函数主要是生成固件签名和web升级使用的不重要
13. >esp_ota_mark_app_valid_cancel_rollback();标记当前应用程序为有效
14.  
~~~
{ 
    mavlink1.update();这个是地面站的主要接口函数
    const uint32_t now_ms = millis(); //arduino的函数获取程序启动以来经过的事件

    if (mavlink_system.sysid != 0)
    {
        
        update_send();   //心跳发送函数请看下面的展开
    }
    else if (g.mavlink_sysid != 0)
    {
        mavlink_system.sysid = g.mavlink_sysid;
    }
    else if (now_ms - last_hb_warn_ms >= 2000)  
    {
        last_hb_warn_ms = millis();
        serial.printf("Waiting for heartbeat\n");  //心跳
    }
    update_receive(); //这个是接收函数请看展开
    if (param_request_last_ms != 0 && now_ms - param_request_last_ms > 50) //目的是定期发送请求
    {
        param_request_last_ms = now_ms;
        float value;
        if (param_next->get_as_float(value))
        {
            mavlink_msg_param_value_send(chan,   
                                         param_next->name, value,
                                         MAV_PARAM_TYPE_REAL32,
                                         g.param_count_float(),  
                                         g.param_index_float(param_next));//发送飞控参数param_count_float获取数量
        }                                                                //param_index_float返回系统的一些数
        param_next++;
        if (param_next->ptype == Parameters::ParamType::NONE)
        {
            param_next = nullptr;
            param_request_last_ms = 0;
        }
    }
}
~~~
~~~
{
    void MAVLinkSerial::update_send(void)
    {
    uint32_t now_ms = millis();
    if (now_ms - last_hb_ms >= 1000)
    {
        last_hb_ms = now_ms;
        mavlink_msg_heartbeat_send(  //这个函数主要是发送心跳
            chan,
            MAV_TYPE_ODID,
            MAV_AUTOPILOT_INVALID,
            0,
            0,
            0);

        // send arming status
        arm_status_send(); //发送odid的状态信息
    }
    }
}
~~~
~~~
{
void MAVLinkSerial::update_receive(void)
{
    mavlink_message_t msg;       // 存储接收到的 MAVLink 消息
    mavlink_status_t status;     // 存储 MAVLink 状态

    status.packet_rx_drop_count = 0;  // 丢包计数

    // 获取串口缓冲区可用字节数
    const uint16_t nbytes = serial.available();

  
    for (uint16_t i = 0; i < nbytes; i++)
    {
       
        const uint8_t c = (uint8_t)serial.read();
        if (mavlink_parse_char(chan, c, &msg, &status)) // 尝试解析当前字节，如果成功解析为一个新的 MAVLink 消息
        {
            // 处理接收到的完整 MAVLink 包,
            process_packet(status, msg);
        }
    }
}
}
~~~ 
~~~
{
#if AP_DRONECAN_ENABLED
    dronecan.update();
#endif  //未使用不介绍
}
~~~

~~~
   const uint32_t now_ms = millis();  //获取系统运行的时间

    // the transports have common static data, so we can just use the
    // first for status
#if AP_MAVLINK_ENABLED
    auto &transport = mavlink1;  //对mavlink1的引用
#elif AP_DRONECAN_ENABLED
    auto &transport = dronecan;//对dronecan的引用他这里想做的用统一的接口来操作不同的对象
#else
    #error "Must enable DroneCAN or MAVLink"
#endif
~~~
~~~
 bool have_location = false;  //位置信息无效
    const uint32_t last_location_ms = transport. get_last_location_ms();  //获取最后一次位置信息
    const uint32_t last_system_ms = transport.get_last_system_ms(); //获取最后一次系统信息
~~~
~~~
led.update();

    status_reason = "";

    if (last_location_ms == 0 ||
        now_ms - last_location_ms > 5000) {
        UAS_data.Location.Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE ;
    } //大于5s远程ID系统发生故障

    if (last_system_ms == 0 ||
        now_ms - last_system_ms > 5000) {
        UAS_data.Location.Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE ;
    }//大于5s远程id系统发生故障
    if (transport.get_parse_fail() != nullptr) {
        UAS_data.Location.Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE ;
        status_reason = String(transport.get_parse_fail()); //打印错误消息
    }
~~~
~~~
  if (g.webserver_enable) {
        webif.update();
    }    //未使用
~~~
~~~
 if (g.bcast_powerup) {
        if (!UAS_data.LocationValid) { // 在开机时广播，始终将位置标记为有效 发送默认的数据
            UAS_data.Location.Status = ODID_STATUS_REMOTE_ID_SYSTEM_FAILURE;
            UAS_data.LocationValid = 1;
        }
    } else {
        if (last_location_ms == 0) {    // 如果没有启用开机时广播 那么接收到一次有效的位置数据时才广播
            delay(1);
            return;
        }
    }
~~~
~~~ //发送数据并进行广播
set_data(transport);

    static uint32_t last_update_wifi_nan_ms;
    if (g.wifi_nan_rate > 0 &&
        now_ms - last_update_wifi_nan_ms > 1000/g.wifi_nan_rate) {
        last_update_wifi_nan_ms = now_ms;
        wifi.transmit_nan(UAS_data);
    }

    static uint32_t last_update_wifi_beacon_ms;
    if (g.wifi_beacon_rate > 0 &&
        now_ms - last_update_wifi_beacon_ms > 1000/g.wifi_beacon_rate) {
        last_update_wifi_beacon_ms = now_ms;
        wifi.transmit_beacon(UAS_data);
    }

    static uint32_t last_update_bt5_ms;
    if (g.bt5_rate > 0 &&
        now_ms - last_update_bt5_ms > 1000/g.bt5_rate) {
        last_update_bt5_ms = now_ms;
        ble.transmit_longrange(UAS_data);
    }

    static uint32_t last_update_bt4_ms;
    int bt4_states = UAS_data.BasicIDValid[1] ? 7 : 6;
    if (g.bt4_rate > 0 &&
        now_ms - last_update_bt4_ms > (1000.0f/bt4_states)/g.bt4_rate) {
        last_update_bt4_ms = now_ms;
        ble.transmit_legacy(UAS_data);
    }
 ~~~

