/****************************************************************************
 *  Copyright (C) 2018 RoboMaster.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of�
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.� See the
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
#include "bsp_uart.h"
#include "keyboard.h"
#include "pid.h"
#include "sys_config.h"
#include "ramp.h"
#include "cmsis_os.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "imu_task.h"
#include "detect_task.h"
#include "calibrate.h"
//#define OLD_TRIGGER
/* gimbal patrol angle (degree)*/
#define PATROL_ANGLE     40
/* patrol period time (ms) */
#define PATROL_PERIOD    1500
/* gimbal back center time (ms) */
#define BACK_CENTER_TIME 2000 //TODO
#define PIT_ECD_DIR  (- 1.0f) 
#define YAW_ECD_DIR  (- 1.0f)
#define YAW_IMU_DIR  (- 1.0f)
#define PIT_IMU_DIR  ( 1.0f)
/* keyboard mode gimbal speed limit */

#define GIMBAL_PC_MOVE_RATIO_PIT 0.1f 
#define GIMBAL_PC_MOVE_RATIO_YAW -1.0f 


/* stack usage monitor */
UBaseType_t gimbal_stack_surplus;

/* gimbal task global parameter */
gimbal_t gim;
int PIT_ECD_CENTER_OFFSET = 201;
int YAW_ECD_CENTER_OFFSET = 3801;
/* control ramp parameter */
static ramp_t     yaw_ramp = RAMP_GEN_DAFAULT;
static ramp_t     pit_ramp = RAMP_GEN_DAFAULT;

uint32_t gimbal_time_last;
int gimbal_time_ms;
uint32_t patrol_count;


extern TaskHandle_t can_msg_send_task_t;
extern TaskHandle_t shoot_task_t;
void gimbal_task(void const *argu)
{
	
	//get sensor info
	update_gimbal_sensor();
	// calculate the pid refence
  switch (gim.ctrl_mode)
  {
    case GIMBAL_INIT:
      init_mode_handle();
    break;
    
    case GIMBAL_FOLLOW_ZGYRO:
      close_loop_handle();
    break;
		
    default:
			close_loop_handle();
    break;
  }

#ifdef NO_CASCADE_CONTROL
	pid_calc(&pid_yaw_speed, gim.pid.yaw_angle_fdb, gim.pid.yaw_angle_ref);
  pid_calc(&pid_pit_speed, gim.pid.pit_angle_fdb, gim.pid.pit_angle_ref);
#else
	cascade_pid_ctrl();
#endif  
  
 

  /* safe protect */
  if (gimbal_is_controllable())
  {
    gim.current[0] = pid_yaw_speed.out;
    gim.current[1] = pid_pit_speed.out;
  }
  else
  {
    memset(gim.current, 0, sizeof(gim.current));
    //gim.ctrl_mode = GIMBAL_RELAX;
    //pid_trigger.iout = 0;
  }
  
  //memset(gim.current, 0, sizeof(gim.current));

  osSignalSet(can_msg_send_task_t, GIMBAL_MOTOR_MSG_SEND);
  osSignalSet(shoot_task_t, SHOT_TASK_EXE_SIGNAL);
	
  gimbal_stack_surplus = uxTaskGetStackHighWaterMark(NULL);

}



float remote_ctrl_map(float offset,float step){
	static float out=0;
	out+=offset;
	if(out>step){
		out-=step;
		return step;
	}
	else{
		float tmp=out;
		out=0;
		return tmp;
	}

}



/*Э�鶨�壺
* ch0 ��ҡ�� 364-1024-1684 ��->��
* ch1 ��ҡ�� 364-1024-1684 ��->��
*	ch2 ��ҡ�� 364-1024-1684 ��->��
* ch3 ��ҡ�� 364-1024-1684 ��->��
*  s1 left  switch  ��-��-��  1-3-2
*  s2 right ... 
*/
void close_loop_handle(void){
	

  static float chassis_angle_tmp=0;
  static float limit_angle_range = 3;
  static float step= GIMBAL_RC_MOVE_RATIO_YAW*330;
	
  gim.pid.pit_angle_fdb = gim.sensor.pit_relative_angle_ecd;
	#ifdef GIMBAL_FOLLOW_CHASSIS
	gim.pid.yaw_angle_fdb = gim.sensor.yaw_relative_angle_ecd ; // 
	#else
	gim.pid.yaw_angle_fdb = gim.sensor.yaw_relative_angle_imu; // 
	#endif 
  
 
  chassis_angle_tmp =  gim.pid.yaw_angle_ref ;
	
  if ((gim.pid.yaw_angle_fdb >= chassis_angle_tmp+ YAW_ANGLE_MIN - limit_angle_range) && \
      (gim.pid.yaw_angle_fdb <= chassis_angle_tmp+ YAW_ANGLE_MAX + limit_angle_range))
  {
		if(KEY_X) gim.pid.yaw_angle_ref = 0;
		
    gim.pid.yaw_angle_ref += YAW_ECD_DIR*remote_ctrl_map( 
											 (KEY_Q)?(step):0 -  (KEY_E)?(-step):0 
		#ifndef GIMBAL_FOLLOW_CHASSIS
											 - remote_info.rc.ch2 * GIMBAL_RC_MOVE_RATIO_YAW
                       + remote_info.mouse.x * GIMBAL_PC_MOVE_RATIO_YAW 
		#endif
			,step);
		
    VAL_LIMIT(gim.pid.yaw_angle_ref, chassis_angle_tmp + YAW_ANGLE_MIN, chassis_angle_tmp + YAW_ANGLE_MAX);	
  }
  /* limit gimbal pitch axis angle */
  if ((gim.sensor.pit_relative_angle_ecd >= PIT_ANGLE_MIN - limit_angle_range) && \
      (gim.sensor.pit_relative_angle_ecd <= PIT_ANGLE_MAX + limit_angle_range))
  {
    gim.pid.pit_angle_ref += - remote_info.rc.ch3 * GIMBAL_RC_MOVE_RATIO_PIT
                       + remote_info.mouse.y * GIMBAL_PC_MOVE_RATIO_PIT;
    VAL_LIMIT(gim.pid.pit_angle_ref, PIT_ANGLE_MIN, PIT_ANGLE_MAX);
  }

}






//ramp used in init model 
void gimbal_back_param(void){ 
  ramp_init(&pit_ramp, BACK_CENTER_TIME/GIMBAL_PERIOD);
  ramp_init(&yaw_ramp, BACK_CENTER_TIME/GIMBAL_PERIOD);
}


uint8_t gimbal_is_controllable(void){
	
  if (gim.ctrl_mode == GIMBAL_RELAX||
		g_err.list[REMOTE_CTRL_OFFLINE].err_exist ||
    g_err.list[GIMBAL_YAW_OFFLINE].err_exist ||
    g_err.list[GIMBAL_PIT_OFFLINE].err_exist)
	{
	#if 0
	printf("In gimbal_is_controllable %d %d %d %d\r\n",gim.ctrl_mode == GIMBAL_RELAX,
	g_err.list[REMOTE_CTRL_OFFLINE].err_exist,
	g_err.list[GIMBAL_YAW_OFFLINE].err_exist,
	g_err.list[GIMBAL_PIT_OFFLINE].err_exist);
	#endif	
    return 0;
	}
  else
    return 1;
}



void init_mode_handle(void){
	//printf("gimbal init mode handle\r\n");
  /* lift gimbal pitch */
	
	
  gim.pid.pit_angle_fdb = gim.sensor.pit_relative_angle_ecd;
  gim.pid.pit_angle_ref = gim.sensor.pit_relative_angle_ecd * (1 - ramp_calc(&pit_ramp)); // desired ref is 0
  
	/* keep yaw unmove this time */
  gim.pid.yaw_angle_fdb = gim.sensor.yaw_relative_angle_imu;
  gim.pid.yaw_angle_ref = gim.pid.yaw_angle_fdb;

  if(gim.pid.pit_angle_fdb >= -2.0f && gim.pid.pit_angle_fdb < 2.0f)
  {
    /* yaw back center after pitch arrive */
    gim.pid.yaw_angle_ref = gim.sensor.yaw_relative_angle_ecd * ( 1 - ramp_calc(&yaw_ramp));
    gim.pid.yaw_angle_fdb = gim.sensor.yaw_relative_angle_ecd;
		
    if (gim.pid.yaw_angle_fdb >= -3.0f && gim.pid.yaw_angle_fdb <= 3.0f)
    {
      /* yaw arrive and switch gimbal state */
      gim.ctrl_mode = GIMBAL_FOLLOW_ZGYRO;
      
			gim.sensor.yaw_offset_angle_imu =atti.yaw;
      //gim.yaw_offset_angle = gim.sensor.gyro_angle;
      gim.pid.pit_angle_ref = 0;
      gim.pid.yaw_angle_ref = 0;
    }
  }
}
void cascade_pid_ctrl(void){

	// feedback position , output speed
	pid_calc(&pid_yaw, gim.pid.yaw_angle_fdb, gim.pid.yaw_angle_ref);
  pid_calc(&pid_pit, gim.pid.pit_angle_fdb, gim.pid.pit_angle_ref);
	
	
  gim.pid.yaw_speed_ref = pid_yaw.out;
  gim.pid.pit_speed_ref = pid_pit.out;
 
  gim.pid.yaw_speed_fdb = gim.sensor.yaw_palstance;
  gim.pid.pit_speed_fdb = gim.sensor.pit_palstance;
	
	// feedback speed , output current
  pid_calc(&pid_yaw_speed, gim.pid.yaw_speed_fdb, gim.pid.yaw_speed_ref);
  pid_calc(&pid_pit_speed, gim.pid.pit_speed_fdb, gim.pid.pit_speed_ref);
	

}

/**
  * @brief     get relative position angle to center
  * @param[in] raw_ecd: gimbal motor encoder raw angle
  * @param[in] center_offset: read gim_cali_data from chip flash
  * @retval    relative angle, unit is degree.
  */
int16_t get_relative_pos(int16_t raw_ecd, int16_t center_offset){
  int16_t tmp = 0;
  if (center_offset >= 4096)
  {
    if (raw_ecd > center_offset - 4096)
      tmp = raw_ecd - center_offset;
    else
      tmp = raw_ecd + 8192 - center_offset;
  }
  else
  {
    if (raw_ecd > center_offset + 4096)
      tmp = raw_ecd - 8192 - center_offset;
    else
      tmp = raw_ecd - center_offset;
  }
  return tmp;
}


void update_gimbal_sensor(void){
	static float yaw_ecd_ratio = YAW_ECD_DIR*YAW_DECELE_RATIO*ENCODER_ANGLE_RATIO;
  static float pit_ecd_ratio = PIT_ECD_DIR*PIT_DECELE_RATIO*ENCODER_ANGLE_RATIO;
  gim.sensor.yaw_relative_angle_ecd =  yaw_ecd_ratio*get_relative_pos(gim.yaw_motor.ecd, YAW_ECD_CENTER_OFFSET);
  gim.sensor.pit_relative_angle_ecd =  pit_ecd_ratio*get_relative_pos(gim.pit_motor.ecd, PIT_ECD_CENTER_OFFSET);
	
	 //gim.sensor.yaw_relative_angle_imu updated in imu task
	
 // ecd offset update in bsp_can
	gim.sensor.yaw_relative_angle_imu = YAW_IMU_DIR*(atti.yaw - gim.sensor.yaw_offset_angle_imu);
	gim.sensor.pit_relative_angle_imu = (atti.roll- gim.sensor.pit_offset_angle_imu);
	
	 /* get gimbal relative palstance */
  gim.sensor.yaw_palstance =   YAW_IMU_DIR* (mpu_data.gy) / 16.384f; //unit: dps
  gim.sensor.pit_palstance = - PIT_IMU_DIR* mpu_data.gx / 16.384f; //unit: dps
}

// called when initialize the gimbal 
// read from flash and set the cali data
void read_gimbal_cali(void){
	cali_data_read(); // read the calibrate data
	if(cali_param.gim_cali_data[CALI_GIMBAL_CENTER].calied_done != CALIED_FLAG )
	{
		gimbal_cali_hook(PIT_ECD_CENTER_OFFSET,YAW_ECD_CENTER_OFFSET);
		cali_data_read();
	}
	
	PIT_ECD_CENTER_OFFSET = cali_param.gim_cali_data[CALI_GIMBAL_CENTER].pitch_offset;
	YAW_ECD_CENTER_OFFSET = cali_param.gim_cali_data[CALI_GIMBAL_CENTER].yaw_offset;
	
}


