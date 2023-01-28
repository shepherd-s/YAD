cmd_/root/YAD/fcm/fcm.mod := printf '%s\n'   flight_control.o fcm_ops.o | awk '!x[$$0]++ { print("/root/YAD/fcm/"$$0) }' > /root/YAD/fcm/fcm.mod
