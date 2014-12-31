#include <stdio.h>
#include <usb.h>

void dec_reset(struct usb_device *dev)
{
	char message[] = {0xaa, 0x00, 0xf1, 0x01, 0x00};
	usb_dev_handle *handle = usb_open(dev);

	printf("Device found.\n");

	if (handle) {
		if (!usb_claim_interface(handle, 0)) {
			int result;
			printf("Reseting device.. ");
			result = usb_bulk_write(handle, 3, message,
						sizeof(message), 1000);
			if (result >= 0)
				printf("succeeded.\n");
			else
				printf("failed. (Error code: %d)\n", result);
		} else {
			printf("Couldn't claim interface.\n");
		}
		usb_close(handle);
	}
}

int main(int argc, char *argv[])
{
	struct usb_bus *busses;
	struct usb_bus *bus;
	(void) argc;
	(void) argv;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	busses = usb_get_busses();

	for (bus = busses; bus; bus = bus->next) {
		struct usb_device *dev;

		for (dev = bus->devices; dev; dev = dev->next) {
			if (dev->descriptor.idVendor == 0x0b48 &&
			    (dev->descriptor.idProduct == 0x1006 ||
			     dev->descriptor.idProduct == 0x1008 ||
			     dev->descriptor.idProduct == 0x1009)) {
				dec_reset(dev);
			}
		}
	}

	return 0;
}
