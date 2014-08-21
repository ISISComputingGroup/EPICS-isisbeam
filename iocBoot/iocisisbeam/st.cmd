#!../../bin/windows-x64/isisbeam

## You may have to change isisbeam to something else
## everywhere it appears in this file

< envPaths

epicsEnvSet "IOCNAME" "$(P=$(MYPVPREFIX))"
epicsEnvSet "IOCSTATS_DB" "$(DEVIOCSTATS)/db/iocAdminSoft.db"

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/isisbeam.dbd"
isisbeam_registerRecordDeviceDriver pdbbase

## configure IOC
isisbeamConfigure("isisbeam")

## Load record instances
dbLoadRecords("$(TOP)/db/isisbeam.db","P=$(IOCNAME)")
dbLoadRecords("$(TOP)/db/shutter_mode.db","P=$(IOCNAME)")
dbLoadRecords("$(TOP)/db/shutter_status.db","P=$(IOCNAME)")
dbLoadRecords("$(TOP)/db/vat.db","P=$(IOCNAME)")
dbLoadRecords("$(IOCSTATS_DB)","IOC=$(IOCNAME)ISISBEAM")

cd ${TOP}/iocBoot/${IOC}
iocInit

## Start any sequence programs
#seq sncxxx,"user=faa59Host"
