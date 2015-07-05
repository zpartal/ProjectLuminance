FIRMWARE=ProjectLuminance.bin

all: compile

compile:
  particle compile ./firmware --saveTo $(FIRMWARE)

flash:
  particle flash $(PARTICLE_NAME) ./firmware

flash-usb: compile
  particle flash --usb $(FIRMWARE)

clean:
  rm -f *.bin