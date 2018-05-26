#ifndef ALARM_TASK_H
#define ALARM_TASK_H

#include "includes.h"

#define ALARM_DEBUG
#ifdef ALARM_DEBUG
    #define alarm_puts     puts
    #define alarm_printf   printf
#else
    #define alarm_puts(...)
    #define alarm_printf(...)
#endif

#define ALARM_TASK_NAME   "AlarmTask"

extern const struct task_info alarm_task_info;

#endif

