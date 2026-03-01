savedcmd_/root/parking-garage/src/parking.mod := printf '%s\n'   src/parking.o | awk '!x[$$0]++ { print("/root/parking-garage/"$$0) }' > /root/parking-garage/src/parking.mod
