#include "kcov.h"
#include <stddef.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/usbdevice_fs.h>
#include <linux/usb/cdc.h>
#include <linux/usb/ch9.h>

int setup(int fd)
{
	int ret = 0;
	// int ifnum = 1;
	// ret = ioctl(fd, USBDEVFS_CLAIMINTERFACE, &ifnum);
	// if (ret < 0)
	// 	return ret;
	struct usbdevfs_ctrltransfer ctrl = {};

	#if 0
	ctrl.bRequest = USB_REQ_GET_DESCRIPTOR;
	ctrl.bRequestType = USB_DIR_IN;
	ctrl.wIndex = 0x1;
	ctrl.wValue = 0;
	ctrl.wLength = 18;
	ret = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
	if (ret < 0)
		return ret;
	memset(&ctrl, 0, sizeof(ctrl));	

	ctrl.bRequest = USB_REQ_GET_DESCRIPTOR;
	ctrl.bRequestType = 0x80;
	ctrl.wIndex = 0x2;
	ctrl.wValue = 0;
	ctrl.wLength = 9;
	ret = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
	if (ret < 0)
		return ret;
	memset(&ctrl, 0, sizeof(ctrl));		

	ctrl.bRequest = USB_REQ_GET_DESCRIPTOR;
	ctrl.bRequestType = 0x80;
	ctrl.wIndex = 0x2;
	ctrl.wValue = 0;
	ctrl.wLength = 94;
	ret = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
	if (ret < 0)
		return ret;
	memset(&ctrl, 0, sizeof(ctrl));		
	#endif
	ctrl.bRequest = USB_REQ_SET_CONFIGURATION;
	ctrl.wValue = 1;
	ret = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
	if (ret < 0)
		return ret;
	memset(&ctrl, 0, sizeof(ctrl));
	struct usbdevfs_setinterface setint = {};
	setint.interface = 1;
	setint.altsetting = 1;
	ret = ioctl(fd, USBDEVFS_SETINTERFACE, &setint);
	// ctrl.bRequest = USB_REQ_SET_INTERFACE;
	// ctrl.bRequestType = 1;
	// ctrl.timeout = 1000;
	// ctrl.wIndex = 1;
	// ctrl.wValue = 1;
	// ret = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
	if (ret < 0)
		return ret;
	memset(&ctrl, 0, sizeof(ctrl));

	// ctrl.bRequest = USB_REQ_SET_INTERFACE;
	// ctrl.bRequestType = 1;
	// ctrl.timeout = 1000;
	// ctrl.wValue = 0;
	// ctrl.wIndex = 1;
	// ctrl.wLength = 0;
	// ret = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
	setint.interface = 1;
	setint.altsetting = 1;
	ret = ioctl(fd, USBDEVFS_SETINTERFACE, &setint);
	if (ret < 0)
		return ret;
	memset(&ctrl, 0, sizeof(ctrl));
	return ret;

	ctrl.bRequest = 0x8a;
	ctrl.bRequestType = 0x21;
	ctrl.timeout = 1000;
	ret = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
	if (ret < 0)
		return ret;
	memset(&ctrl, 0, sizeof(ctrl));

	ctrl.bRequest = 0x84;
	ctrl.bRequestType = 0x21;
	ctrl.timeout = 1000;
	ret = ioctl(fd, USBDEVFS_CONTROL, &ctrl);
	if (ret < 0)
		return ret;
	memset(&ctrl, 0, sizeof(ctrl));
	
	return ret;
}

int main(int argc, char **argv)
{
	int ret = 0;
	int bus = 0, dev = 0, ep = 2;
	if (argc > 1) {
		bus = atoi(argv[1]);
	}
	if (argc > 2) {
		dev = atoi(argv[2]);
	}
	// if (argc > 3) {
		// ep = atoi(argv[3]);
	// }

	char fname[256] = {};
	sprintf(fname, "/dev/bus/usb/%03d/%03d", bus, dev);

	int fd = open(fname, O_WRONLY);
	if (fd < 0) {
		perror("open device");
		return fd;
	}

	kcov_init();
	kcov_reset();

	ret = setup(fd);
	// kcov_dump_adresses();
	printf("ret = %d\n", ret);
	if (ret < 0)
		return 0;
	const char message[16] = {};
	struct {
		struct usb_cdc_ncm_nth16 nth;
		char data[(sizeof(message) + 3) / 4 * 4];
		struct usb_cdc_ncm_ndp16 npd;
		struct usb_cdc_ncm_ndp16 datagramms[1];
	} __attribute__((packed)) package = {};
	package.nth.dwSignature = USB_CDC_NCM_NTH16_SIGN;
	package.nth.wHeaderLength = sizeof(package.nth);
	package.nth.wBlockLength = sizeof(package);
	package.nth.wNdpIndex = offsetof(typeof(package), npd);

	package.npd.wLength = 16;
	package.npd.dwSignature = USB_CDC_NCM_NDP16_NOCRC_SIGN;
	package.npd.wNextNdpIndex = 0;
	package.npd.dpe16[0].wDatagramIndex = offsetof(typeof(package), data);
	package.npd.dpe16[0].wDatagramLength = sizeof(package.data);

	for (char i = 0; i < sizeof(message); ++i) {
		package.data[i] = message[i];
	}
	
	struct usbdevfs_bulktransfer bulk = {};
	bulk.ep = ep;
	bulk.len = sizeof(package);
	bulk.timeout = 1000;
	bulk.data = &package;
	kcov_reset();
	ret = ioctl(fd, USBDEVFS_BULK, &bulk);

	kcov_dump_adresses();
	printf("tracked = %lu\n", kcov_tracked());

	printf("ret = %d\n", ret);
	kcov_free();
	
	return 0;
}
