#####################################################################
# CS:APP Malloc Lab
# Handout files for students
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# May not be used, modified, or copied without permission.
#
######################################################################

***********
Main Files:
***********

mm.{c,h}	
	Your solution malloc package. mm.c is the file that you
	will be handing in, and is the only file you should modify.

mdriver.c	
	The malloc driver that tests your mm.c file

short{1,2}-bal.rep
	Two tiny tracefiles to help you get started. 

Makefile	
	Builds the driver

**********************************
Other support files for the driver
**********************************

config.h	Configures the malloc lab driver
fsecs.{c,h}	Wrapper function for the different timer packages
clock.{c,h}	Routines for accessing the Pentium and Alpha cycle counters
fcyc.{c,h}	Timer functions based on cycle counters
ftimer.{c,h}	Timer functions based on interval timers and gettimeofday()
memlib.{c,h}	Models the heap and sbrk function

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:

	unix> mdriver -V -f short1-bal.rep

The -V option prints out helpful tracing and summary information.

To get a list of the driver flags:

	unix> mdriver -h

#####################################################################
# CS:APP Malloc Lab
# 학생용 안내 문서 (한국어)
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# 허가 없이 사용, 수정 또는 복사할 수 없습니다.
#
######################################################################

***********
주요 파일:
***********

mm.{c,h}
	여러분이 구현할 malloc 패키지입니다. 제출할 파일은 mm.c이며,
	수정해야 하는 유일한 파일도 mm.c입니다.

mdriver.c
	mm.c 파일을 테스트하는 malloc 드라이버입니다.

short{1,2}-bal.rep
	시작을 돕기 위한 작은 trace 파일 두 개입니다.

Makefile
	드라이버를 빌드합니다.

**********************************
드라이버를 위한 기타 지원 파일
**********************************

config.h	Configures the malloc lab driver / malloc lab 드라이버를 설정합니다.
fsecs.{c,h}	서로 다른 타이머 패키지를 감싸는 래퍼 함수입니다.
clock.{c,h}	Pentium 및 Alpha 사이클 카운터에 접근하는 루틴입니다.
fcyc.{c,h}	사이클 카운터 기반 타이머 함수입니다.
ftimer.{c,h}	interval timer와 gettimeofday() 기반 타이머 함수입니다.
memlib.{c,h}	힙과 sbrk 함수를 모델링합니다.

*******************************
드라이버 빌드 및 실행
*******************************

드라이버를 빌드하려면 셸에서 `make`를 입력하세요.

작은 테스트 trace로 드라이버를 실행하려면:

	unix> mdriver -V -f short1-bal.rep

`-V` 옵션은 유용한 추적 정보와 요약 정보를 출력합니다.

드라이버 플래그 목록을 보려면:

	unix> mdriver -h

