# Arduino Car
  
Robotic car that follows line drawn on the ground using arduino.

Project was created at BUT-FIT
<img src="https://github.com/freedie666/IMR/blob/main/images/VUT-logo.png?raw=true"  width = 25/>
, class IMR.

<p align="center">
  <img src="https://github.com/freedie666/IMR/blob/main/images/arduino-car-main.jpg?raw=true"  width = 50%/>
</p>
  
  
  
## What robot does

> - Follows line at the ground
> - Can detect extra line that tells robot where to go at the crossroads
> - Extra lines can give robot additional orders
> - Detection of obstacle on the line
> - Robot can find best way around obstacle and avoid it
> - Is able to carry cargo and unload it on command
> - Sings a song when detects obstacle
> - Has multiple Blue and RED LEDs that show its intentions
> - Carries its own power source - batteries

## How its done

> - Brain of the operation is Arduino UNO
> - Car is powered by two 12V DC motors with gearbox
> - Motors are controlled by motor driver LV8406 using PWM
> - Lines are detected by infrared emiting LED and phototransistor [3x LED and 4x phototransistors]
> - Speed and directions are controlled by P-regulator
> - Obstacles are detected with ultrasonic sensor HC-SR04 
> - Ultrasonic sensor is mounted on small servo, providing range of detection up to +-90° 
> - Cargo beds movement is provided by stepper motor 2BYJ-48
> - Blue and RED LEDs are controlled by WS2818B led driver
> - Buzzer can play All star song which is stored in Flash memory

## Last notes

> - Everything electical is soldered at custom made PCB which fits onto Arduino as Arduino shield
> - Song is stacked in memory in a way to save as much memory as possible [4 numbers in one unsigned short]
  
## Links 
Demonstration of car at [Youtube](https://www.youtube.com/watch?v=gtERXzQzxGM)

More photos and videos at [Google Drive](https://drive.google.com/drive/folders/1pzaO6EnN3uHb_-Xj_Ckr5q-n50d8h_We?usp=sharing)



