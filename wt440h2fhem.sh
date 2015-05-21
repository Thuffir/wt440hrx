#!/bin/bash

while read housecode channel status battery humid temp seqnr; do
  batt_txt="ok"
  batt_stat=""
  if [ $battery -eq 1 ]; then
    batt_txt="low"
    batt_stat=", Bat.low!"
  fi
  echo "setreading wt440h_${housecode}_${channel} temperature ${temp}"
  echo "setreading wt440h_${housecode}_${channel} humidity ${humid}"
  echo "setreading wt440h_${housecode}_${channel} battery ${batt_txt}"
  echo "set wt440h_${housecode}_${channel} ${temp}°C, ${humid}%${batt_stat}"
done
