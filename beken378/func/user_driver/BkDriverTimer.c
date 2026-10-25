/**
 ******************************************************************************
 * @file    BkDriverTimer.h
 * @brief   This file provides all the headers of timer operation functions.
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2017 BEKEN Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */

#include "include.h"
#include "rtos_pub.h"
#include "drv_model_pub.h"
#include "bk_timer_pub.h"

/**@brief Initialises a timer and start
 *
 * @note  user can user timer0 timer1 timer2 timer4 timer5
 *
 * @param timer_id      : the timer id
 * @param time_ms       : time value(unit:ms)
 * @param callback      : callback
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred with any step
 */
OSStatus bk_timer_initialize(uint8_t timer_id, uint32_t time_ms, void *callback)
{
    UINT32 ret;
    timer_param_t param;
    
    param.channel = timer_id;
    param.div = 1;              //timer0 timer1 timer2 26M // timer4 timer5 32K (n+1) division
    param.period = time_ms;
    param.t_Int_Handler= callback;

    ret = sddev_control(TIMER_DEV_NAME, CMD_TIMER_INIT_PARAM, &param);
    ASSERT(BK_TIMER_SUCCESS == ret);

    UINT32 timer_channel;
    timer_channel = param.channel;
    ret = sddev_control(TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &timer_channel);
    ASSERT(BK_TIMER_SUCCESS == ret);

    return kNoErr;
}


OSStatus bk_timer_initialize_us(uint8_t timer_id, uint32_t time_us, void *callback)
{
    UINT32 ret;
    timer_param_t param;
    
    param.channel = timer_id;
    param.div = 1;              //timer0 timer1 timer2 26M // timer4 timer5 32K (n+1) division
    param.period = time_us;
    param.t_Int_Handler= callback;

    ret = sddev_control(TIMER_DEV_NAME, CMD_TIMER_INIT_PARAM_US, &param);
    ASSERT(BK_TIMER_SUCCESS == ret);

    UINT32 timer_channel;
    timer_channel = param.channel;
    ret = sddev_control(TIMER_DEV_NAME, CMD_TIMER_UNIT_ENABLE, &timer_channel);
    ASSERT(BK_TIMER_SUCCESS == ret);

    return kNoErr;
}

UINT32 bk_get_timer_cnt(uint8_t timer_id)
{
    timer_param_t param;

    param.channel = timer_id;

    if(sddev_control(TIMER_DEV_NAME, CMD_TIMER_READ_CNT, &param) != BK_TIMER_SUCCESS){
		return 0xFFFFFFFFU;
    }

    return param.period;
}

/**@brief stop timer
 *
 * @note  user can user timer0 timer1 timer2 timer4 timer5
 *
 * @param timer_id      : the timer id
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred with any step
 */
OSStatus bk_timer_stop(uint8_t timer_id)
{
    UINT32 ret;
    UINT32 timer_channel;
    
    timer_channel = timer_id;
    ret = sddev_control(TIMER_DEV_NAME, CMD_TIMER_UNIT_DISABLE, &timer_channel);
    ASSERT(BK_TIMER_SUCCESS == ret);
    return kNoErr;
}

#if BKDRIVERTIMRE_TEST_DEMO
static void bk_timer_test_isr_cb(UINT8 arg)
{
	bk_printf("%s %d rtos-time: %d mS\r\n",__FUNCTION__,__LINE__,rtos_get_time());
}

void bk_timer_test_start(void)
{
	bk_timer_initialize(BKTIMER5,1000,bk_timer_test_isr_cb);
}

#include "include.h" // 引入 Beken SDK 的标准头文件

// 定义一个打印系统寄存器的任务
void sys_reg_dump_task(void *arg)
{
    uint32_t midr, ctr, tcm;

    // 【核心】使用 ARM 内联汇编读取 CP15 协处理器寄存器
    // MRC 指令：Move to Register from Coprocessor
    __asm volatile (
        "MRC p15, 0, %0, c0, c0, 0 \n"  // 读取主ID寄存器 (MIDR)
        "MRC p15, 0, %1, c0, c0, 1 \n"  // 读取 Cache Type 寄存器 (CTR)
        "MRC p15, 0, %2, c0, c0, 2 \n"  // 读取 TCM Status 寄存器 (TCMTR)
        : "=r" (midr), "=r" (ctr), "=r" (tcm) // 输出操作数
        :                                      // 无输入操作数
        :                                      // 无破坏的寄存器
    );

    bk_printf("\r\n--- System Register Dump ---\r\n");
    bk_printf("MIDR  (Main ID)       : 0x%08X\r\n", midr);
    bk_printf("CTR   (Cache Type)    : 0x%08X\r\n", ctr);
    bk_printf("TCMTR (TCM Status)    : 0x%08X\r\n", tcm);
    bk_printf("-----------------------------\r\n\r\n");

    // 打印完毕后，删除自己，释放内存
    rtos_delete_thread(NULL);
}

// 在你的 user_main 中创建这个任务
void user_main(void)
{
    bk_printf("%s %s\r\n", __FILE__, __FUNCTION__);
    
    // 创建系统寄存器打印任务
    rtos_create_thread(NULL, 
                       4,                    // 优先级
                       "reg_dump",           // 任务名
                       (beken_thread_function_t)sys_reg_dump_task, 
                       1024,                 // 栈大小
                       (beken_thread_arg_t)0);
}

//void user_main(void)
//{
//    bk_printf("%s %s\r\n",__FILE__,__FUNCTION__);
//    bk_timer_test_start();
//}

#endif
// eof

