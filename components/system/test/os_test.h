#ifndef __OS_TEST_H__
#define __OS_TEST_H__

#define CONFIG_SYS_LOG_COLOR_ON 1
#undef CONFIG_SYS_LOG_LEVEL
#define CONFIG_SYS_LOG_LEVEL SYS_LOG_LEVEL_DBG
#include "sys_log.h"

#include "os/os.h"
#include <stdio.h>
#include <stdlib.h>

#define OSTest_Assert 1

/* 测试使用到的内存结构 */
typedef struct
{
	u8_t UseRate;				//记录 CPU 使用率
	uint CyclePreMS;			//计算平均每毫秒运行了任务的循环次数
	uint StickCount;			//累计系统嘀嗒时钟的中断次数
	uint Error_Line;			//调度暂停位置
	void (*SysTickRoute)(void); //每当产生系统节拍时钟中断时运行的测试函数

	os_mutex_t Mem_Mutex1;
	os_mutex_t Mem_Mutex2;
	os_mutex_t Mem_Mutex3;
	os_mutex_t Mem_Mutex4;

	uint Mutex_Value;

	struct
	{
		os_thread_t thread_handle;			 //任务句柄
		os_timer_t timer_handle;			 //任务句柄
		struct Type_OS_TaskMemDef *AllocMem; //申请到的内存地址
		uint RunTimes;						 //任务循环次数
		uint EventTimes;					 //
		uint Stack_Used;					 //任务最大使用堆栈
		uint Stack_Empty;					 //任务最大剩余堆栈
		vuint Count;
		double Result_dFPU;
		volatile float Result_fFPU;
	} Task[8];

	struct
	{
		uint TotalWrite;
		uint TotalRead;
		uint WriteLog;
		uint ReadComp;
		uint DebugTime1;
		uint DebugTime2;
	} BufTest;

} Type_OS_TestMem_Def;

extern Type_OS_TestMem_Def rm_Test;

extern os_work_t test_work_handle[10];
extern os_fifo_t fifo_handle[10];
extern os_queue_t queue_handle[10];
extern os_pipe_t pipe_handle[10];

/* 用于测试的简便工具 */
os_thread_t *os_test_create_thread(os_thread_t *thread_handle, void (*TaskRoute)(void *), char *name, s16_t Priority); //简单创建任务

#endif
//
