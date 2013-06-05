#!../../bin/windows-x64/isisbeam

## You may have to change isisbeam to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/isisbeam.dbd"
isisbeam_registerRecordDeviceDriver pdbbase

## configure IOC
isisbeamConfigure("isisbeam")

## Load record instances
dbLoadRecords("$(TOP)/db/isisbeam.db","P=faa59Host")

cd ${TOP}/iocBoot/${IOC}
iocInit

## Start any sequence programs
#seq sncxxx,"user=faa59Host"
