/*
 * bsp_debug_uart.c
 *
 * Modified for LLVM/Picolibc (RA FSP)
 */

#include "bsp_debug_uart.h"
#include <stdio.h>
#include "led/bsp_led.h"
/* 发送完成标志位 */
volatile bool uart_send_complete_flag = false;

/* * -----------------------------------------------------------
 * 初始化函数
 * -----------------------------------------------------------
 */
void Debug_UART4_Init(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* 打开 UART4 */
    err = R_SCI_UART_Open(&g_uart4_ctrl, &g_uart4_cfg);

    assert(FSP_SUCCESS == err);
}

/* * -----------------------------------------------------------
 * 中断回调函数
 * -----------------------------------------------------------
 */
void debug_uart4_callback(uart_callback_args_t *p_args)
{
    switch(p_args -> event)
    {
        case UART_EVENT_RX_CHAR:        // 接收到数据
        {
            /* 简单的回显功能: 收到什么发回什么 */
          //  R_SCI_UART_Write(&g_uart4_ctrl, (uint8_t *)&p_args->data, 1);
            switch(p_args->data)
            {
                case'1':LED1_ON;break;
                case'2':LED2_ON;break;
                case'3':LED3_ON;break;
                case'4':LED1_OFF;break;
                case'5':LED2_OFF;break;
                case'6':LED3_OFF;break;
                default:
                    break;
            }
            break;
        }
        case UART_EVENT_TX_COMPLETE:    // 发送完成
        {
            /* 设置标志位，告诉主程序发送结束了 */
            uart_send_complete_flag = true;
            break;
        }
        default:
            break;
    }
}

/* * -----------------------------------------------------------
 * 重定向 printf 核心逻辑 (适配 LLVM/Picolibc)
 * -----------------------------------------------------------
 */

/* 1. 定义一个单字符发送函数，供 printf 调用 */
static int uart_putchar(char c, FILE *stream)
{
    (void)stream; // 忽略 stream 参数

    /* 清除标志位 */
    uart_send_complete_flag = false;

    /* 调用 FSP 库发送 1 个字节 */
    R_SCI_UART_Write(&g_uart4_ctrl, (uint8_t *)&c, 1);

    /* 【重要】阻塞等待发送完成 */
    /* 如果不等待，printf 连续发送时会导致 UART 忙碌报错或丢数据 */
    while(uart_send_complete_flag == false)
    {
        /* 可以在这里加入超时处理，防止死循环 */
    }

    return c;
}

/* 2. 使用 Picolibc 特有的宏定义一个输出流 */
/* 参数1: 发送函数 */
/* 参数2: 接收函数 (这里不需要，填 NULL) */
/* 参数4: 读写标志 */
static FILE __uart_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, NULL, _FDEV_SETUP_WRITE);

/* 3. 将系统标准输出 stdout 指向我们要定义的流 */
/* 这里的 const 必须加上，以匹配系统头文件中的声明 */
FILE *const stdout = &__uart_stdout;
