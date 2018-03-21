/****************************************************************************
 *  Copyright (C) 2018 RoboMaster.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************/
/** @file gimbal_task.c
 *  @version 1.1
 *  @date Oct 2017
 *
 *  @brief gimbal control task
 *
 *  @copyright 2017 DJI RoboMaster. All rights reserved.
 *
 */

#include "gimbal_task.h"
#include "chassis_task.h"
#include "shoot_task.h"
#include "comm_task.h"
#include "modeswitch_task.h"
#include "info_interactive.h"
#include "remote_ctrl.h"
#include "infantry_info.h"
#include "bsp_uart.h"
#include "calibrate.h"
#include "keyboard.h"
#include "pid.h"
#include "sys_config.h"
#include "ramp.h"
#include "cmsis_os.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"

//#define OLD_TRIGGER
/* gimbal patrol angle (degree)*/
#define PATROL_ANGLE     40
/* patrol period time (ms) */
#define PATROL_PERIOD    1500
/* gimbal back center time (ms) */
#define BACK_CENTER_TIME 2500

/* stack usage monitor */
UBaseType_t gimbal_stack_surplus;

/* for debug */
//int yaw_a_fdb;
//int yaw_a_ref;
//int yaw_s_fdb;
//int yaw_s_ref;

//int pit_a_fdb;
//int pit_a_ref;
//int pit_s_fdb;
//int pit_s_ref;

/* gimbal task global parameter */
gimbal_t gim;

/* gimbal task static parameter */
/* shot buff parameter */
static big_buff_t cv_buff;
/* control ramp parameter */
static ramp_t     yaw_ramp = RAMP_GEN_DAFAULT;
static ramp_t     pit_ramp = RAMP_GEN_DAFAULT;

uint32_t gimbal_time_last;
int gimbal_time_ms;
uint32_t patrol_count;

void gimbal_self_check(void)
{
  if ( !read_gimbal_offset(&(gim.pit_center_offset), &(gim.yaw_center_offset),
                           &(cv_buff.pit_offset),    &(cv_buff.yaw_offset)) )
  {
    /* gimbal has not been calibrated */
    no_cali_data_handle();
  }
}

extern TaskHandle_t can_msg_send_task_t;
extern TaskHandle_t shot_task_t;
void gimbal_task(void const *argu)
{
  gimbal_time_ms = HAL_GetTick() - gimbal_time_last;
  gimbal_time_last = HAL_GetTick();
  
  switch (gim.ctrl_mode)
  {
    case GIMBAL_INIT:
      init_mode_handle(); // move gimbal to the center position and then switch to GIMBAL_FOLLOW_ZGYRO
    break;
    
    case GIMBAL_NO_ARTI_INPUT:
      no_action_handle();
    break;

    case GIMBAL_FOLLOW_ZGYRO:
      close_loop_handle();
    break;

    case GIMBAL_TRACK_ARMOR:
      track_aimor_handle();
    break;

    case GIMBAL_PATROL_MODE:
      gimbal_patrol_handle();
    break;

    case GIMBAL_SHOOT_BUFF:
      big_buff_handle();
    break;

    case GIMBAL_POSITION_MODE:
      pc_position_ctrl_handle();

    break;

    default:
    break;
  }

  /* pitch axis limited angle */
  //VAL_LIMIT(gim.pid.pit_angle_ref, PIT_ANGLE_MIN, PIT_ANGLE_MAX);
  
  
  pid_calc(&pid_yaw, gim.pid.yaw_angle_fdb, gim.pid.yaw_angle_ref);
  pid_calc(&pid_pit, gim.pid.pit_angle_fdb, gim.pid.pit_angle_ref);
  
  cascade_pid_ctrl();
  
  pid_calc(&pid_yaw_speed, gim.pid.yaw_speed_fdb, gim.pid.yaw_speed_ref);
  pid_calc(&pid_pit_speed, gim.pid.pit_speed_fdb, gim.pid.pit_speed_ref);

  /* safe protect */
  if (gimbal_is_controllable())
  {
    glb_cur.gimbal_cur[0] = -YAW_MOTO_POSITIVE_DIR*pid_yaw_speed.out;
    glb_cur.gimbal_cur[1] = -PIT_MOTO_POSITIVE_DIR*pid_pit_speed.out;
    glb_cur.gimbal_cur[2] = pid_trigger_speed.out;
  }
  else
  {
    memset(glb_cur.gimbal_cur, 0, sizeof(glb_cur.gimbal_cur));
    gim.ctrl_mode = GIMBAL_RELAX;
    //pid_trigger.iout = 0;
  }
//    yaw_a_ref = gim.pid.yaw_angle_ref*100;
//    yaw_a_fdb = gim.pid.yaw_angle_fdb*100;
//    yaw_s_ref = km.yaw_v*100;
//    yaw_s_fdb = (int)mpu_data.gz;

//    pit_a_ref = gim.pid.pit_angle_ref*100;
//    pit_a_fdb = gim.pid.pit_angle_fdb*100;
//    pit_s_ref = (int)pid_pit.out;
//    pit_s_fdb = (int)mpu_data.gx;
  
  osSignalSet(can_msg_send_task_t, GIMBAL_MOTOR_MSG_SEND);
  osSignalSet(shot_task_t, SHOT_TASK_EXE_SIGNAL);

  gimbal_stack_surplus = uxTaskGetStackHighWaterMark(NULL);

}

void cascade_pid_ctrl(void)
{
#ifdef GIMBAL_CASCADE_CTRL
  gim.pid.yaw_speed_ref = pid_yaw.out;
  gim.pid.pit_speed_ref = pid_pit.out;
#endif
  
  gim.pid.yaw_speed_fdb = gim.sensor.yaw_palstance;
  gim.pid.pit_speed_fdb = gim.sensor.pit_palstance;
}

void init_mode_handle(void)
{
  /* lift gimbal pitch */
  gim.pid.pit_angle_fdb = gim.sensor.pit_relative_angle;
  gim.pid.pit_angle_ref = gim.sensor.pit_relative_angle * (1 - ramp_calc(&pit_ramp));
  /* keep yaw not move this time */
  gim.pid.yaw_angle_fdb = gim.sensor.yaw_relative_angle;
  gim.pid.yaw_angle_ref = gim.ecd_offset_angle;

  if(gim.pid.pit_angle_fdb >= -2.0f)
  {
    /* yaw back center after pitch arrive */
    gim.pid.yaw_angle_ref = gim.sensor.yaw_relative_angle * ( 1 - ramp_calc(&yaw_ramp));
    
    if (gim.pid.yaw_angle_fdb >= -1.5f && gim.pid.yaw_angle_fdb <= 1.5f)
    {
      /* yaw arrive and switch gimbal state */
      gim.ctrl_mode = GIMBAL_FOLLOW_ZGYRO;
      
      gim.yaw_offset_angle = gim.sensor.gyro_angle;
      gim.pid.pit_angle_ref = 0;
      gim.pid.yaw_angle_ref = 0;
    }
  }
}

void no_action_handle(void)
{
  if (gim.input.no_action_flag == 1)
  {
    if ((HAL_GetTick() - gim.input.no_action_time) < 1500)
    {
      close_loop_handle();
    }
    else
    {
      gim.input.no_action_flag = 2;
      gim.pid.yaw_angle_ref = 0;
    }
  }
  
  if (gim.input.no_action_flag == 2)
  {
    gim.pid.pit_angle_fdb = gim.sensor.pit_relative_angle;
    gim.pid.yaw_angle_fdb = gim.sensor.yaw_relative_angle;
  }
}

void close_loop_handle(void)
{
  static float chassis_angle_tmp;
  static float limit_angle_range = 2;
  
  gim.pid.pit_angle_fdb = gim.sensor.pit_relative_angle;
  gim.pid.yaw_angle_fdb = gim.sensor.gyro_angle - gim.yaw_offset_angle;
  
  /* chassis angle relative to gim.pid.yaw_angle_fdb */
  chassis_angle_tmp = gim.pid.yaw_angle_fdb - gim.sensor.yaw_relative_angle;
	
  /* limit gimbal yaw axis angle */
  if ((gim.sensor.yaw_relative_angle >= YAW_ANGLE_MIN - limit_angle_range) && \
      (gim.sensor.yaw_relative_angle <= YAW_ANGLE_MAX + limit_angle_range))
  {
    gim.pid.yaw_angle_ref += rm.yaw_v * GIMBAL_RC_MOVE_RATIO_YAW
                       + km.yaw_v * GIMBAL_PC_MOVE_RATIO_YAW;
    VAL_LIMIT(gim.pid.yaw_angle_ref, chassis_angle_tmp + YAW_ANGLE_MIN, chassis_angle_tmp + YAW_ANGLE_MAX);
  }
	
  /* limit gimbal pitch axis angle */
  if ((gim.sensor.pit_relative_angle >= PIT_ANGLE_MIN - limit_angle_range) && \
      (gim.sensor.pit_relative_angle <= PIT_ANGLE_MAX + limit_angle_range))
  {
    gim.pid.pit_angle_ref += rm.pit_v * GIMBAL_RC_MOVE_RATIO_PIT
                       + km.pit_v * GIMBAL_PC_MOVE_RATIO_PIT;
    VAL_LIMIT(gim.pid.pit_angle_ref, PIT_ANGLE_MIN, PIT_ANGLE_MAX);
  }
}

void pc_position_ctrl_handle(void)
{
  gim.pid.pit_angle_fdb = gim.sensor.pit_relative_angle;
  gim.pid.yaw_angle_fdb = gim.sensor.yaw_relative_angle;
  
  taskENTER_CRITICAL();
  gim.pid.pit_angle_ref = pc_rece_mesg.gimbal_control_data.pit_ref;
  gim.pid.yaw_angle_ref = pc_rece_mesg.gimbal_control_data.yaw_ref;
  taskEXIT_CRITICAL();
}

//int dynamic_bias_yaw = 0;//-60;
//int dynamic_bias_pit = -15;
static void track_aimor_handle(void)
{
//  //static float    pnp_dis;
//  static uint8_t  last_vision_status;
//  static uint32_t no_vision_time;
//  static int32_t  yaw_vision_bias;
//  static int32_t  pit_vision_bias;
//  
//  gim.pid.pit_angle_fdb = gim.sensor.pit_relative_angle;
//  gim.pid.yaw_angle_fdb = gim.sensor.yaw_relative_angle;
//  
//  if (pc_rece_mesg.gimbal_control_data.visual_valid == 1)
//  {
//    /* pitch target */
//    //calculate pitch elevation
//    //pnp_dis = pc_rece_mesg.gimbal_control_data.distance / 1000.0;
//    //dynamic_bias_pit = pnp_dis*5.0f - 20;
//    
//    pit_vision_bias = pc_rece_mesg.gimbal_control_data.vision_x + dynamic_bias_pit;
//    gim.pid.pit_angle_ref = gim.sensor.pit_relative_angle - 0.1*pit_vision_bias;
////    pid_calc(&pid_vision_pit, pit_vision_bias, 0);
////    gim.pid.pit_angle_ref = gim.sensor.pit_relative_angle+ pid_vision_pit.out;

//    /* yaw target */
//    yaw_vision_bias = pc_rece_mesg.gimbal_control_data.vision_y + dynamic_bias_yaw;
//    gim.pid.yaw_angle_ref = gim.sensor.yaw_relative_angle - 0.3*yaw_vision_bias;
////    pid_calc(&pid_vision_yaw, yaw_vision_bias, 0);
////    gim.pid.yaw_angle_ref = gim.sensor.yaw_relative_angle + pid_vision_yaw.out;
//    VAL_LIMIT(gim.pid.yaw_angle_ref, -50, 50);

//    if ((yaw_vision_bias > -15) && (yaw_vision_bias < 15)) // && (rece_data.vision_dis <= 5000))
//      shot.c_shoot_cmd = 1;
//    else
//      shot.c_shoot_cmd = 0;

//  }
//  else if (pc_rece_mesg.gimbal_control_data.visual_valid == 0)
//  {
//    /* when auto shooting lost vision,
//       yaw and pitch return center after 2s */
//    if (last_vision_status == 1)
//    {
//      no_vision_time = HAL_GetTick();
//    }
//    if (HAL_GetTick() - no_vision_time > 2000)
//    {
//      gim.pid.yaw_angle_ref = 0;
//      gim.pid.pit_angle_ref = 0;
//    }
//    
//    /* if chassis follow gimbal when auto shooting lost vision
//       yaw return center immediately */
////    if (chassis.follow_gimbal)
////    {
////      gim.pid.yaw_angle_ref = 0;
////    }
//    shot.c_shoot_cmd = 0;
//  }

}

static void gimbal_patrol_handle(void)
{
  static int16_t patrol_period = PATROL_PERIOD/GIMBAL_PERIOD;
  static int16_t patrol_angle  = PATROL_ANGLE;
  
  gim.pid.pit_angle_fdb = gim.sensor.pit_relative_angle;
  gim.pid.yaw_angle_fdb = gim.sensor.yaw_relative_angle;
  
  patrol_count++;
  gim.pid.yaw_angle_ref = patrol_angle*sin(2*PI/patrol_period*patrol_count);
  gim.pid.pit_angle_ref = 0;
}


/**
  * @brief initialize gimbal pid parameter
  *
  */
void gimbal_param_init(void)
{
  memset(&gim, 0, sizeof(gimbal_t));
  
  gim.ctrl_mode      = GIMBAL_NO_ARTI_INPUT;
  gim.last_ctrl_mode = GIMBAL_RELAX;
  gim.input.ac_mode        = NO_ACTION;
  gim.input.action_angle   = 5.0f;
  
  /* pitch axis motor pid parameter */
  PID_struct_init(&pid_pit, POSITION_PID, 2000, 0,
                  30, 0, 0); //
  PID_struct_init(&pid_pit_speed, POSITION_PID, 7000, 3000,
                  15, 0.2, 0);

  /* yaw axis motor pid parameter */
  PID_struct_init(&pid_yaw, POSITION_PID, 2000, 0,
                  30, 0, 0); //
  PID_struct_init(&pid_yaw_speed, POSITION_PID, 7000, 1000,
                  13, 0, 0);
  
  /* bullet trigger motor pid parameter */
  PID_struct_init(&pid_trigger, POSITION_PID, 10000, 2000,
                  15, 0, 10);
  PID_struct_init(&pid_trigger_speed, POSITION_PID, 7000, 3000,
                  1.5, 0.1, 5);

}

void gimbal_back_param(void)
{ 
  ramp_init(&pit_ramp, BACK_CENTER_TIME/GIMBAL_PERIOD);
  ramp_init(&yaw_ramp, BACK_CENTER_TIME/GIMBAL_PERIOD);
}
/* big buff relevant param */
float y_kp,p_kp;
float big_buff_tab[9][2];
void update_buff_table(void)
{
  float delta_pit, delta_yaw;
  
  delta_pit = fabs(cv_buff.pit_calied_9 - cv_buff.pit_calied_5);
  delta_yaw = fabs(cv_buff.yaw_calied_9 - cv_buff.yaw_calied_5);
  
  float tab[9][2] = 
  {
    delta_yaw, delta_pit,   0, delta_pit,   -delta_yaw, delta_pit,
    delta_yaw,         0,   0,         0,   -delta_yaw,         0,
    delta_yaw,-delta_pit,   0,-delta_pit,   -delta_yaw,-delta_pit,
  };
  
  for (uint8_t i = 0; i < 9; i++)
  {
    big_buff_tab[i][0] = tab[i][0] + cv_buff.yaw_calied_5;
    big_buff_tab[i][1] = tab[i][1] + cv_buff.pit_calied_5;
  }
  
}

void big_buff_handle(void)
{
//  static int32_t  yaw_vision_bias;
//  static int32_t  pit_vision_bias;
//  
//  gim.pid.pit_angle_fdb = gim.sensor.pit_relative_angle;
//  gim.pid.yaw_angle_fdb = gim.sensor.yaw_relative_angle;
//  
//  switch (gim.auto_ctrl_cmd)
//  {
//    case CMD_TARGET_NUM:
//    {
//      gim.auto_ctrl_cmd = CMD_TARGET_NUM;
//       
//      if (pc_rece_mesg.gimbal_control_data.buff_sta == CMD_TARGET_NUM)
//      {
//        gim.pid.yaw_angle_ref = big_buff_tab[cv_buff.target_num][0];
//        gim.pid.pit_angle_ref = big_buff_tab[cv_buff.target_num][1];
//        
//        if (cv_buff.last_num != cv_buff.target_num)
//          shot.shoot_cmd = 1;
//         
//        cv_buff.last_num = cv_buff.target_num;
//      }
//    }break;
//    
//    case CMD_CALI_FIVE:
//    {
//      gim.auto_ctrl_cmd = CMD_CALI_FIVE;
//      
//      if (pc_rece_mesg.gimbal_control_data.buff_sta == CMD_CALI_FIVE)
//      {
//        pit_vision_bias = pc_rece_mesg.gimbal_control_data.vision_x + cv_buff.pit_offset;
//        gim.pid.pit_angle_ref = gim.sensor.pit_relative_angle - 0.1*pit_vision_bias;

//        yaw_vision_bias = pc_rece_mesg.gimbal_control_data.vision_y + cv_buff.pit_offset;
//        gim.pid.yaw_angle_ref = gim.sensor.yaw_relative_angle - 0.3*yaw_vision_bias;
//        
//        if (abs(yaw_vision_bias) < 10)
//        {
//          cv_buff.pit_calied_5 = gim.sensor.pit_relative_angle;
//          cv_buff.yaw_calied_5 = gim.sensor.yaw_relative_angle;
//          
//          gim.auto_ctrl_cmd = CMD_CALI_NINE;
//        }
//      }
//    }break;
//    
//    case CMD_CALI_NINE:
//    {
//      gim.auto_ctrl_cmd = CMD_CALI_NINE;
//      
//      if (pc_rece_mesg.gimbal_control_data.buff_sta == CMD_CALI_NINE)
//      {
//        pit_vision_bias = pc_rece_mesg.gimbal_control_data.vision_x + cv_buff.pit_offset;
//        gim.pid.pit_angle_ref = gim.sensor.pit_relative_angle - 0.1*pit_vision_bias;

//        yaw_vision_bias = pc_rece_mesg.gimbal_control_data.vision_y + cv_buff.pit_offset;
//        gim.pid.yaw_angle_ref = gim.sensor.yaw_relative_angle - 0.3*yaw_vision_bias;
//        
//        if (abs(yaw_vision_bias) < 10)
//        {
//          cv_buff.pit_calied_9 = gim.sensor.pit_relative_angle;
//          cv_buff.yaw_calied_9 = gim.sensor.yaw_relative_angle;
//          
//          gim.auto_ctrl_cmd = CMD_TARGET_NUM;
//          update_buff_table();
//        }
//      }
//    }break;
//  }
}
