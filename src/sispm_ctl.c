/*
  SisPM_ctl.c
 
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
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <usb.h>
#include <assert.h>
#include "sispm_ctl.h"



int usb_command(usb_dev_handle *udev, int b1, int b2, int *status )
{
  int  reqtype=0x21; //USB_DIR_OUT + USB_TYPE_CLASS + USB_RECIP_INTERFACE /*request type*/,
  int  req=0x09;
  char buffer[2];

  buffer[0]=b1;
  buffer[1]=b2;
  if(status!=NULL)
  {
	reqtype|=USB_DIR_IN;
	req=0x01;
  }

  if ( usb_control_msg(udev /* handle*/,
		       reqtype, req,
		       (0x03<<8) | b1,
		       0 /*index*/,
		       buffer /*bytes*/ ,
		       2, //1 /*size*/,
		       500) < 0 )
  {
      fprintf(stderr,"Error performing requested action\n"
	          "Libusb error string: %s\nTerminating\n",usb_strerror());
      usb_close (udev);
      exit(-5);
  }
  /*printf("%s\n",usb_strerror()); */

  if(status!=NULL) *status=(buffer[1]!=0)?1:0;
  return (buffer[1]!=0)?1:0;
}


usb_dev_handle*get_handle(struct usb_device*dev)
{
    usb_dev_handle *udev=NULL;
    if( dev==NULL ) return NULL;
    udev = usb_open( dev );

    /* prepare USB access */
    if (udev == NULL)
    {
	fprintf(stderr,"Unable to open USB device %s\n",usb_strerror());
        usb_close (udev);
	exit(-1);
    }
    if (usb_set_configuration(udev,1) !=0)
    {
	fprintf(stderr,"USB set configuration %s\n",usb_strerror());
        usb_close (udev);
	exit(-2);
    }
    if ( usb_claim_interface(udev, 0) !=0)
    {
	fprintf(stderr,"USB claim interface %s\nMaybe device already in use?\n",usb_strerror());
	exit(-3);
    }
    if (usb_set_altinterface(udev, 0) !=0)
    {
	fprintf(stderr,"USB set alt interface %s\n",usb_strerror());
        usb_close (udev);
	exit(-4);
    }
    return udev;
}
