##
YAD 1.0

This software aims to turn a single board computer into a drone using the minimum  
possible amount of hardware. To do so, it uses the Xenomai4 real-time co-kernel (EVL)  
to simulate PWM on the gpio as input for the ESCs of the motors as most SBCs don't  
provide 4 independent hardware PWM channels. The companion Android app is for using  
the phone as a controller.    

###
HARDWARE TESTED

- Orange Pi Pc
    - CPU: Allwinner H3 (arm32)
    - RAM: 1GB
    - SO: armbian (with Xenomai-evl kernel)
    - https://source.denx.de/Xenomai
    - https://www.armbian.com
___
- Realtek Usb Wifi Dongle
    - Driver: rtl8188fu.ko
    - https://github.com/kelebek333/rtl8188fu.git
___
- 4x ESC 30A  

- 4x 2122 900KV Brushless Motor  

- 4x 1045" Propeller  

- LiPo Battery
    - Capacity: 2700 mah
    - Discharge Rate: 40C
    - Number of Cells: 3S  
___
- Drone Frame
