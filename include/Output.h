/*
 * Output.h
 *
 *  Created on: 2015年7月23日
 *      Author: shaker
 */

#ifndef OUTPUT_H_
#define OUTPUT_H_

#define MAX_CHANNEL 12
class Output
{
public:
	Output();
	virtual ~Output();
	void SetChannelValue(u8,u16);
	void Init();
private:
	u16 Channels[MAX_CHANNEL];
};

#endif /* OUTPUT_H_ */
