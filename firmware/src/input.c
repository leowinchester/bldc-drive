/*
 * input.c
 *
 *  Created on: Oct 26, 2015
 *      Author: pekka
 */
#include <stm32f10x_gpio.h>
#include <stm32f10x_tim.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_exti.h>

#include "pwm.h"
#include "encoder.h"
#include "pid.h"
#include "input.h"

void initLeds()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	//LED0 == ENABLE Led
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	//LED1 == ERROR Led
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);


}
void initStepDirInput()
{
	GPIO_InitTypeDef GPIO_InitStructure;

	//PA6 as STEP input
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA7 as DIR pin
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA5 as ENA pin
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource6); //STEP pin
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource5); //ENA pin

	EXTI_InitTypeDef EXTI_initStructure;
	EXTI_initStructure.EXTI_Line = EXTI_Line6;
	EXTI_initStructure.EXTI_Mode = EXTI_Mode_Interrupt;

#if STEP_POLARITY == 1
	EXTI_initStructure.EXTI_Trigger = EXTI_Trigger_Rising;
#else
	EXTI_initStructure.EXTI_Trigger = EXTI_Trigger_Falling;
#endif

	EXTI_initStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_initStructure);

	EXTI_initStructure.EXTI_Line = EXTI_Line5;
	EXTI_initStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_initStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_initStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_initStructure);

	NVIC_InitTypeDef nvicStructure;
	nvicStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	nvicStructure.NVIC_IRQChannelPreemptionPriority = 0;
	nvicStructure.NVIC_IRQChannelSubPriority = 2;
	nvicStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvicStructure);
}

void EXTI9_5_IRQHandler()
{
	uint8_t ena,idir;

	if(EXTI_GetITStatus(EXTI_Line5)!= RESET)
	{
		//ENA PIN INTERRUPT
#if ENA_POLARITY == 1
		ena = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5);
#else
		ena = (~(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5)))&1;
#endif
		if(ena && !motor_running)
		{

			//enable rose. Start
			pid_requested_position = encoder_count; //reset the desired position to current.
			pwm_motorStart();

			ENABLE_LED_ON;
			ERROR_LED_OFF;
		}
		else if(!ena && motor_running)
		{
			//enable fell. Stop.
			pwm_motorStop();
			ENABLE_LED_OFF;
		}
		else if(!ena)
		{
			ENABLE_LED_OFF;
		}
		EXTI_ClearITPendingBit(EXTI_Line5);
	}

	if(EXTI_GetITStatus(EXTI_Line6)!= RESET)
	{
		//STEP PIN INTERRUPT

#if DIR_POLARITY == 1
		idir = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7);
#else
		idir = (~(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7)))&1;
#endif
		if(idir)
		{
			pid_requested_position+=10;
		}
		else
			pid_requested_position-=10;
		EXTI_ClearITPendingBit(EXTI_Line6);
	}

}

void initPWMInput()
{

	GPIO_InitTypeDef GPIO_InitStructure;

	//TIM3 as PWM input
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA7 as DIR pin
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//PA5 as ENA pin
	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Prescaler = 1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = 65535;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);



	TIM_ICInitTypeDef TIM_ICInit;

	TIM_ICInit.TIM_ICPolarity = TIM_ICPolarity_Rising;
	TIM_ICInit.TIM_ICFilter = 5;
	TIM_ICInit.TIM_Channel = TIM_Channel_1;
	TIM_ICInit.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	TIM_ICInit.TIM_ICSelection = TIM_ICSelection_DirectTI;

	TIM_PWMIConfig(TIM3, &TIM_ICInit);
	TIM_CCxCmd(TIM3, TIM_Channel_1, ENABLE);
	TIM_CCxCmd(TIM3, TIM_Channel_2, ENABLE);

	TIM_SelectInputTrigger(TIM3,TIM_TS_TI1FP1);
	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);
	TIM_ITConfig(TIM3, TIM_IT_CC1 | TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3,ENABLE);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


void TIM3_IRQHandler(void) {
  if (TIM_GetITStatus(TIM3, TIM_IT_CC1) != RESET)
  {
	//  TM_GPIO_TogglePinValue(GPIOA, GPIO_Pin_12);

	// calculate motor  speed or else with CCR1 values
	uint16_t tim3_dc = TIM3->CCR2;
	uint16_t tim3_per = TIM3->CCR1;
	static uint8_t prevdir;
	uint16_t DC;
	dir = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7);
	if(dir!=prevdir)
	{

		pwm_InitialBLDCCommutation();
	}
	prevdir=dir;
	if(tim3_per>0 && tim3_per>tim3_dc)
	{
	  DC = ((uint32_t)(BLDC_CHOPPER_PERIOD*tim3_dc)/(uint32_t)tim3_per);
	}
	else
	  DC=0;

	if(DC<BLDC_NOL*3) DC=BLDC_NOL*3;
	pwm_setDutyCycle(DC);
	updateCtr=0;
/*	if(motor_running==0)
	{
		motor_running=1;
		//BLDCMotorPrepareCommutation();

	}*/
	TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);

  }
  else if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
  {

	updateCtr++;
	if(updateCtr>50)
	{
		motor_running=0;
		TIM1->CCR1 = 0;
		TIM1->CCR2 = 0;
		TIM1->CCR3 = 0;
	}
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

  }
}


