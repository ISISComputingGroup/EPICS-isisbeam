#!../../bin/windows-x64/isisbeam

## @file
## IOC startup file for beam logger.
## Provides PVs for status of ISIS beam and accelerator 
< envPaths

epicsEnvSet "IOCNAME" "ISISBEAM_01"
epicsEnvSet "IOCSTATS_DB" "$(DEVIOCSTATS)/db/iocAdminSoft.db"

# on main server MYPVPREFIX will be ""
epicsEnvSet "PVROOT" "$(MYPVPREFIX)"

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/isisbeam.dbd")
isisbeam_registerRecordDeviceDriver pdbbase

## configure IOC
isisbeamConfigure("isisbeam")

## Load record instances to define PVs
dbLoadRecords("$(TOP)/db/isisbeam.db","P=$(PVROOT)")
dbLoadRecords("$(TOP)/db/beamline.db","P=$(PVROOT)")
dbLoadRecords("$(IOCSTATS_DB)","IOC=$(PVROOT)CS:IOC:$(IOCNAME):DEVIOS")

cd ${TOP}/iocBoot/${IOC}
iocInit

## Start any sequence programs
#seq sncxxx,"user=faa59Host"
