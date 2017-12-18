#!/bin/bash

CPU_ID=(10 12 14 16 44 46 48 50 52 54 56 58 60 62 64)
MONITOR=""
FN=""
i=0
for p in $@
do
	if [ $i -lt `expr $# - 1` ]
	then
		pqos -e "llc:$(( $i + 1 ))=$p"
		if [ $i -eq 0 ]
		then 
			MONITOR="${CPU_ID[0]}"
			FN="$p"
		else
			MONITOR="${MONITOR},""${CPU_ID[$i]}"
			FN="${FN}_""$p"
		fi
		i=$(( $i + 1 ))
	fi	
	#echo ${MONITOR}
	#echo ${FN}		
done
FN="${FN}_""${@: -1}"
pqos -a "llc:1=10;llc:2=12;llc:3=14;llc:4=16;llc:5=44;llc:6=46;llc:7=48;llc:8=50;llc:9=52;llc:10=54;llc:11=56;llc:12=58;llc:13=60;llc:14=62;llc:15=64"
pqos -m "all:${MONITOR}" > "${FN}.txt"
