#!../../bin/windows-x64/isisbeam

## @file
## IOC startup file for beam logger.
## Provides PVs for status of ISIS beam and accelerator 
< envPaths

epicsEnvSet "IOCNAME" "$(P=$(MYPVPREFIX))"
epicsEnvSet "IOCSTATS_DB" "$(DEVIOCSTATS)/db/iocAdminSoft.db"

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/isisbeam.dbd")
isisbeam_registerRecordDeviceDriver pdbbase

## configure IOC
isisbeamConfigure("isisbeam")

## Load record instances to define PVs
dbLoadRecords("$(TOP)/db/isisbeam.db","P=$(IOCNAME)")
dbLoadRecords("$(TOP)/db/beamline.db","P=$(IOCNAME)")
dbLoadRecords("$(IOCSTATS_DB)","IOC=$(IOCNAME)ISISBEAM")

cd ${TOP}/iocBoot/${IOC}
iocInit

## Start any sequence programs
#seq sncxxx,"user=faa59Host"
