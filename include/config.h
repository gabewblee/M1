#pragma once

#include <compiler.h>
#include <types.h>

/* System configurations                 */
#define CONSOLE_MAX_CNT    8u          /* Maximum console count        */

/* Klog subsystem configurations         */
#define KLOG_BUF_SZ        4096u       /* Kernel log ring buffer size  */

/* EVGA subsystem configurations         */
#define EVGA_BLACK_COLOR   0x00u       /* EVGA color black             */
#define EVGA_WHITE_COLOR   0x0Fu       /* EVGA color white             */
#define EVGA_PHYS_ADDR     0xB8000u    /* EVGA buffer physical address */

/* IDT subsystem configurations          */
#define IDT_EXC_CNT        32u         /* CPU exception count          */
#define IDT_IRQ_CNT        16u         /* PIC IRQ count                */
#define IDT_INT_MAX_CNT    256u        /* Maximum interrupt count      */

/* IPC subsystem configurations          */
#define IPC_QUEUE_CAPACITY 16u         /* IPC queue capacity           */

/* Task subsystem configurations         */
#define TASK_MAX_CNT       64u         /* Maximum task count           */

/* Thread subsystem configurations       */
#define THREAD_KSTACK_SZ   8192u       /* Kernel stack size            */
#define THREAD_MAX_CNT     256u        /* Maximum thread count         */

/* PMM subsystem configurations          */
#define FRM_MAX_CNT        (1u << 20u) /* Maximum frame count          */

