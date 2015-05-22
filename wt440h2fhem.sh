#!/bin/bash

HOST=localhost
PORT=7072

while read housecode channel status battery humid temp; do
  batt_txt="ok"
  batt_stat=""
  if [ $battery -eq 1 ]; then
    batt_txt="low"
    batt_stat=", Bat.low!"
  fi
  echo -ne "setreading wt440h_${housecode}_${channel} temperature ${temp}\nsetreading wt440h_${housecode}_${channel} humidity ${humid}\nsetreading wt440h_${housecode}_${channel} battery ${batt_txt}\nset wt440h_${housecode}_${channel} ${temp}°C, ${humid}%${batt_stat}\n" | nc ${HOST} ${PORT}
done
