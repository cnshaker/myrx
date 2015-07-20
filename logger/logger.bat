@set d=%date:~0,10%%time:~0,8%
@set d=%d:/=%
@set d=%d:-=%
@set d=%d::=%
@set d=%d: =0%
rtt_logger stm32f103c8 C:\Users\shaker\Desktop\RTTLog\RTTLog\%d%.txt