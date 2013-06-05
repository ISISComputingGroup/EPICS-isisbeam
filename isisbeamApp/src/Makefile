TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = isisbeam
# isisbeam.dbd will be created and installed
DBD += isisbeam.dbd

# isisbeam.dbd will be made up from these files:
isisbeam_DBD += base.dbd

# Include dbd files from all support applications:
#isisbeam_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#isisbeam_LIBS += xxx

# isisbeam_registerRecordDeviceDriver.cpp derives from isisbeam.dbd
isisbeam_SRCS += isisbeam_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
isisbeam_SRCS_DEFAULT += isisbeamMain.cpp
isisbeam_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#isisbeam_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
isisbeam_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

