# Compile module under Linux 2.6.x using something like:
# make -C /lib/modules/`uname -r`/build SUBDIRS=$PWD modules

obj-m += ioctl_module.o