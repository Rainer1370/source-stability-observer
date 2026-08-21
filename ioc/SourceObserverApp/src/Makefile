TOP=../..
include $(TOP)/configure/CONFIG

PROD_IOC = sourceObserver
DBD += sourceObserver.dbd
sourceObserver_DBD += base.dbd
sourceObserver_SRCS += sourceObserverMain.cpp
sourceObserver_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
