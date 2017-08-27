# Keylog
windows 10 compatible<br>
Kernel Mode, driver only, ring O, remote UDP keylogger. 

# Note: Using this software is at your Own Risk.
# The Author will not be held responsible by any circumstances.

# Tech
this project splits down into two parts:<br>
1 KeyBoard hook.<br>
2 Raw Networking (datagram socket manipulation).<br>

to monitor the key strokes we need to get in beetwin the keyboard device IRP & the PS/2 port.<br>
Much of the KeyBoard Hook implementation & code is borrowed from <html><a href="https://github.com/fdiskyou">fdiskyou</a></html>.<br>
while to implement this 'kernel man in the middle', we need to mimic the IRP function passed down from the physical device up to the operating system processing; this is done by installing a hook beetwin the keyboard device and passing down each IRP request to the next implementation level.<br>
![](pic/keyboard-driver-stack.png)<br>
Our hook (by the diagram above) will come in beetwin win32k.sys & KBDHID.sys, each request is cached by our hook, proccessed by Our driver and passed up to the next irql.<br>
The Second part of Our driver Operation is to log the keystrokes & send them back to our monitoring server.<br>
i have implemented a UDP-DataGram protocol, as we do not recieve or handle any data coming back from the server, and this also make's the monitoring process a lot simpler by the server side.<br>
Another Advantage to a udp implementation is that the port can be closed and opened contantly to make the #dfir work a lot harder.<br>
to implement that i made use of the <html><a href="https://msdn.microsoft.com/library/windows/hardware/ff571083">Wsk</a></html>, (windows socket kernel), as to avoid any user-mode application.<br>
# Usage
currently only the local keylogger is Generic nd can be used W/O building the driver (as it simply logs the keystroke's to C:\\Windows\\logs), but the remote udp based (that do not need to write any data to disk to run), needs to be build to your server address, as time will pass i will add a universal support, so you wouldn't have to build your own.
# Installtion:
!The driver is not signed, so you will have to disable code integrity (or get me a sponser to sign the driver), after that:<br>
Open an elevated command prompt:<br>
sc create kMon type=kernel binpath="\path\to\your\driver.sys"<br>
sc start kMon<br>
# Uninstall:
sc stop Kmon
# if you encounter any problems simply restart your computer.
# For any bugs comment an issue in this github repo.
enjoy!

