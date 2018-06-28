@echo off

@echo ********************************************************************************
@echo 			SDK BR17 Apps Soundbox  ...			
@echo ********************************************************************************
@echo %date%

cd %~dp0

if exist %1.bin del %1.bin 
if exist %1.lst del %1.lst 

REM  echo %1
REM  echo %2
REM  echo %3

REM %2 -disassemble %1.or32 > %1.lst 
%3 -O binary -j .text  %1.or32  %1.bin 
%3 -O binary -j .data  %1.or32  data.bin 
%3 -O binary -j .data1 %1.or32  data1.bin 
%2 -section-headers  %1.or32 

copy %1.bin/b + data.bin/b + data1.bin/b sdram.app

REM if exist %1.or32 del %1.or32
del %1.bin data.bin data1.bin

if exist uboot.boot del uboot.boot
type uboot.bin > uboot.boot

cd ui_resource
copy *.* ..\
cd ..

set TEST_BOX_WIRELESS_UPDATE=-todev sound_box -fi jl_690x.bin -ld 0x0000 -rd 0x0000 -fo jl_690x.bfu
cd tone_resource
copy *.mp3 ..\
cd ..
isd_download.exe -tonorflash -dev br17 -boot 0x2000 -div6 -wait 300 -f uboot.boot sdram.app bt_cfg.bin fast_run.bin btmode.mp3  btconok.mp3 btdisc.mp3 ring.mp3 child.mp3 stroy.mp3 english.mp3 chinese.mp3 music.mp3 instum.mp3 sleep.mp3 poweron.mp3 poweroff.mp3 nofile.mp3 nofile1.mp3 pause.mp3 play.mp3 next.mp3 prev.mp3 inssd.mp3 tfmode.mp3 di.mp3 do.mp3 micmode.mp3 pcon.mp3 lowpower.mp3 lowpowoff.mp3 nonet.mp3 micexit.mp3 tfoffline.mp3 tfonline.mp3 updateok.mp3 upfail.mp3 alarm.mp3 sanzijin.mp3 fw_ver.str -key AC690X.key

::%TEST_BOX_WIRELESS_UPDATE% //ָ����������sound_box������������������
::-format cfg
:: -read flash_r.bin 0-2M

if exist *.mp3 del *.mp3 
if exist *.PIX del *.PIX
if exist *.TAB del *.TAB
if exist *.res del *.res
if exist *.sty del *.sty
if exist jl_690x.bin del jl_690x.bin
if exist jl_cfg.bin del jl_cfg.bin


rename jl_isd.bin jl_690x.bin
bfumake.exe -fi jl_690x.bin -ld 0x0000 -rd 0x0000  -fn fw_ver.str

@rem rename jl_isd.bin jl_cfg.bin
@rem bfumake.exe -fi jl_cfg.bin -ld 0x0000 -rd 0x0000 -fo jl_690x.bfu
@rem copy /b jl_690x.bfu+jl_flash_cfg.bin  jl_690x.bfu

IF EXIST no_isd_file del jl_690x.bin jl_cfg.bin
del no_isd_file


@rem format vm        //����VM 68K����
@rem format cfg       //����BT CFG 4K����
@rem format 0x3f0-2  //��ʾ�ӵ� 0x3f0 �� sector ��ʼ�������� 2 �� sector(��һ������Ϊ16���ƻ�10���ƶ��ɣ��ڶ�������������10����)



