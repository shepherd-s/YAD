## YAD 1.0

This software aims to turn a Single Board Computer into a drone using as little  
hardware as possible. To do so, it uses the Xenomai4 real-time co-kernel (EVL)  
to simulate PWM on the gpio as input for the ESCs of the motors as most SBCs don't  
provide 4 independent hardware PWM channels. The companion Android app is for using  
the phone as a controller.    

### HARDWARE TESTED

- Orange Pi Pc
    - CPU: Allwinner H3 (arm32)
    - RAM: 1GB
    - SO: armbian (with Xenomai-evl kernel)
    - https://source.denx.de/Xenomai
    - https://www.armbian.com
    
- Realtek Usb Wi-Fi Dongle
    - Driver: rtl8188fu.ko
    - https://github.com/kelebek333/rtl8188fu.git
    
- 4x ESC 30A  

- 4x 2122 900KV Brushless Motor  

- 4x 1045" Propeller  

- LiPo Battery
    - Capacity: 2700 mah
    - Discharge Rate: 40C
    - Number of Cells: 3S
    
- Drone Frame

### USAGE

The software consist of 3 main parts, the fcm (flight control module) kernel  
module, the user space program (yad) and the android apk.  

You have to make a Wi-Fi access point in the ip 192.168.0.1 on your SBC and insert  
the fcm.ko module. Then run the yad user space application with ./yad command and  
launch the apk on the phone, which should be Android 9 or above. The ESCs should be  
connected as follows:  

- Front left motor ESC --> gpio pin 6
- Front right motor ESC --> gpio pin 110
- Rear left motor ESC --> gpio pin 20
- Rear right motor ESC --> gpio pin 200

### NOTES

The gpio pin naming is the gpiolib one.  
Once the kernel module is inserted, it should not be removed until the  
next reboot for security reasons.

### DEMO

https://user-images.githubusercontent.com/90930079/223528166-b8d2f61b-fab9-4cb4-b6c4-dd4db5cb6346.mp4
