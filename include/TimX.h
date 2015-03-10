/*
 * TimX.h
 *
 *  Created on: 2015年3月10日
 *      Author: shaker
 */

#ifndef TIMX_H_
#define TIMX_H_

class TimX
{
public:
	TimX();
	virtual ~TimX();

};

void TIM3_Int_Init(u16 arr,u16 psc);


#endif /* TIMX_H_ */
