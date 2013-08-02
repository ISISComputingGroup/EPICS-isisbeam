#!../../bin/windows-x64/isisbeam

## You may have to change isisbeam to something else
## everywhere it appears in this file

< envPaths

epicsEnvSet "IOCNAME" "$(P=$(MYPVPREFIX))ISIS"
epicsEnvSet "IOCSTATS_DB" "$(DEVIOCSTATS)/db/iocAdminSoft.db"

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/isisbeam.dbd"
isisbeam_registerRecordDeviceDriver pdbbase

## configure IOC
isisbeamConfigure("isisbeam")

## Load record instances
dbLoadRecords("$(TOP)/db/isisbeam.db","P=$(IOCNAME):")
dbLoadRecords("$(IOCSTATS_DB)","IOC=$(IOCNAME)")

cd ${TOP}/iocBoot/${IOC}
iocInit

## Start any sequence programs
#seq sncxxx,"user=faa59Host"
