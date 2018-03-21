#include "sys_config.h"
#include "can_send_task.h"
#include "cmsis_os.h"
#include "chassis_task.h"
#include "bsp_can.h"
#include "gimbal_task.h"
UBaseType_t can_send_surplus;
void can_msg_send_task(void const *argu)
{
  osEvent event;
  while (1)
  {
    event = osSignalWait(GIMBAL_MOTOR_MSG_SEND | \
                         CHASSIS_MOTOR_MSG_SEND, osWaitForever);
    
    if (event.status == osEventSignal)
    {
      if (event.value.signals & GIMBAL_MOTOR_MSG_SEND)
      {
//        can_time_ms = HAL_GetTick() - can_time_last;
//        can_time_last = HAL_GetTick();
        
        //send_gimbal_motor_ctrl_message(glb_cur.gimbal_cur);
				//send_gimbal_cur(gim.current[0],gim.current[1],0); CAN signal for ID = 7 motor set to speed 0
				// Last time is the upper code, and it is used to use remote control's switch to start M2006
				switch (remote_info.rc.s1) {
					case 1: // switch up: fast blender
						send_gimbal_cur(gim.current[0],gim.current[1],500);
					break;
					case 2: // switch down
						send_gimbal_cur(gim.current[0],gim.current[1],0);
					break;
					case 3: //switch middle: slow blender
						send_gimbal_cur(gim.current[0],gim.current[1],400);
					break;
				}
      }
      
      if (event.value.signals & CHASSIS_MOTOR_MSG_SEND)
      {
//        can1_time_ms = HAL_GetTick() - can1_time_last;
//        can1_time_last = HAL_GetTick();
        send_chassis_cur(chassis.current[0],chassis.current[1],chassis.current[2],chassis.current[3]);
      }
    }
    can_send_surplus = uxTaskGetStackHighWaterMark(NULL);
  }
}

