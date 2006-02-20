/*
  SisPM_ctl.h
 
  Controls the GEMBIRD Silver Shield PM USB outlet device
 
  (C) 2004, Mondrian Nuessle, Computer Architecture Group, University of Mannheim, Germany
  (C) 2005, Andreas Neuper, Germany

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


  nuessle@uni-mannheim.de
  aneuper@web.de

*/

#define MAXGEMBIRD			 32
#define VENDOR_ID 			 0x04B4
#define PRODUCT_ID			 0xFD11
#define USB_DIR_IN                       0x80            /* to host */
#define USB_DIR_OUT                      0               /* to device */
#define cpu_to_le16(a)                   (a)

usb_dev_handle*get_handle(struct usb_device*dev);
int usb_command(usb_dev_handle *udev, int b1, int b2, int *status );

#define sispm_buzzer_on(udev)		usb_command( udev, 1,        0x01, NULL )
#define sispm_buzzer_off(udev)		usb_command( udev, 1,        0x00, NULL ) 
#define sispm_switch_on(udev,outlet)	usb_command( udev, 3*outlet, 0x03, NULL ) 
#define sispm_switch_off(udev,outlet)	usb_command( udev, 3*outlet, 0x00, NULL )
#define sispm_switch_getstatus(udev,outlet,status)	\
					usb_command( udev, 3*outlet, 0x03, status )


#include <usb.h>
