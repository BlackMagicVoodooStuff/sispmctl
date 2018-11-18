/*
  process.c - process web requests

  (C) 2015-2018, Heinrich Schuchardt <xypron.glpk@gmx.de>
  (C) 2011-2016, Pete Hildebrandt <send2ph@gmail.com>
  (C) 2010, Olivier Matheret, France, for the plannification part
  (C) 2004-2011, Mondrian Nuessle, Computer Architecture Group,
      University of Mannheim, Germany
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
*/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <usb.h>
#include "config.h"
#include "sispm_ctl.h"

#define BSIZE   65536
int debug = 0;
int verbose = 1;
#ifdef DATADIR
char* homedir = DATADIR;
#else
char* homedir = 0;
#endif

#ifndef WEBLESS
static void service_not_available(int out)
{
  char xbuffer[BSIZE+2];

  sprintf(xbuffer, "HTTP/1.1 503 Service not available\n"
          "Server: SisPM\nContent-Type: "
          "text/html\n\n"
          "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
          "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
          "<html><head>\n<title>503 Service not available</title>\n"
          "<meta http-equiv=\"refresh\" content=\"2;url=/\">\n"
          "</head><body>\n"
          "<h1>503 Service not available</h1></body></html>\n\n");
  send(out,xbuffer,strlen(xbuffer),0);
}

static void bad_request(int out)
{
  char xbuffer[BSIZE+2];

  sprintf(xbuffer, "HTTP/1.1 404 Not found\nServer: SisPM\nContent-Type: "
          "text/html\n\n"
          "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
          "\"http://www.w3.org/TR/html4/loose.dtd\">\n"
          "<html><head>\n<title>404 Not found</title>\n"
          "<meta http-equiv=\"refresh\" content=\"2;url=/\">\n"
          "</head><body>\n"
          "<h1>404 Not found</h1></body></html>\n\n");
  send(out,xbuffer,strlen(xbuffer),0);
}

void process(int out,char *request, struct usb_device *dev, int devnum)
{
  char xbuffer[BSIZE+2];
  char filename[1024];
  char *ptr = NULL;
  FILE *in = NULL;
  long length = 0;
  long lastpos = 0;
  long remlen = 0;
  usb_dev_handle *udev;
  unsigned int id; //product id of current device
  char *retvalue = NULL;

  if (debug)
    fprintf(stderr,"\nRequested is (%s)\n",request);

  // extract the file name
  if (strchr(request,'\n') != NULL) {
    memset(filename, 0, 1023);
    strncpy(filename, strchr(request,' ')+1, strchr(request,'\n')-request);
    ptr = strchr(filename, ' ');
    if (ptr != NULL)
      *ptr = 0;
  }

  // avoid to read other directories, %-codes are not evaluated
  ptr = strrchr(filename,'/');
  if (ptr != NULL)
    ++ptr;
  else
    ptr = filename;

  if (strlen(ptr) == 0)
    ptr="index.html";

  if (debug) {
    fprintf(stderr,"\nrequested file name(%s)\n", filename);
    fprintf(stderr,"resulting file name(%s)\n", ptr);
    fprintf(stderr,"change directory to (%s)\n", homedir);
  }

  if (chdir(homedir) != 0) {
    bad_request(out);
    return;
  }

  if (debug)
    fprintf(stderr,"\nopen file(%s)\n",ptr);

  in = fopen(ptr,"r");

  if (in == NULL) {
    bad_request(out);
    return;
  }

  /* get device-handle/-id */
  udev = get_handle(dev);
  if (udev == NULL) {
    fprintf(stderr, "No access to Gembird #%d USB device %s\n", devnum,
            dev->filename );
    service_not_available(out);
    fclose(in);
    return;
  } else if (verbose)
    printf("Accessing Gembird #%d USB device %s\n", devnum, dev->filename );
  id = get_id(dev);

  lastpos = ftell(in);
  retvalue = fgets(xbuffer, BSIZE-1, in);
  remlen = length = ftell(in) - lastpos;
  lastpos = ftell(in);

  while (!feof(in)) {
    if (retvalue == NULL) {
      bad_request(out);
      break;
    }

    char *mrk = xbuffer;
    char *ptr = xbuffer;
    /* search for:
     *  $$exec(0)?.1.:.2.$$     to execute command(#)
     *  $$stat(2)?.1.:.2.$$     to evaluate status(#)
     *  $$version()$$           to evaluate version
     */
    for (mrk = ptr = xbuffer; (ptr-xbuffer) < length; ++ptr) {
      if (*ptr=='$' && ptr[1]=='$') {
        /*
         * $$exec(1)?positive:negative$$
         *   ^cmd    ^pos             ^trm
         * ^ptr   ^num        ^neg
         */
        char *cmd=&ptr[2];
        char *num=strchr(cmd,'(');
        char *pos=strchr(num?num:cmd,'?');
        char *neg=strchr(pos?pos:cmd,':');
        char *trm=strchr(neg?neg:cmd,'$');
        if (debug) {
          fprintf(stderr,"%p\n%p\n%p\n%p\n%p\n%p\n",cmd,num,pos,neg,trm,ptr);
          fprintf(stderr,"%s%s%s%s%s%s",cmd,num,pos,neg,trm,ptr);
        }

        if (trm != NULL) {
          if (num == NULL) {
            fprintf(stderr, "Command-Format: $$exec(#)?positive:negative$$  "
                    "ERROR at #");
            service_not_available(out);
            fclose(in);
            return;
          }
          ++num;
          if (pos != NULL)
            ++pos;
          if (neg != NULL)
            ++neg;

          send(out,mrk,ptr-mrk,0);
          remlen = remlen - (ptr - mrk);
          mrk=ptr;

          if (strncasecmp(cmd,"on(",3)==0) {
            if (trm[1] != '$' || !pos || !neg) {
              fprintf(stderr, "Command-Format: $$on(#)?positive:negative$$");
              service_not_available(out);
              fclose(in);
              return;
            }
            if (debug)
              fprintf(stderr,"\nON(%s)\n",num);
            if (sispm_switch_on(udev,id,atoi(num)) !=0)
              send(out,pos,neg-pos-1,0);
            else
              send(out,neg,trm-neg,0);
          } else if (strncasecmp(cmd,"off(",4)==0) {
            if (trm[1] != '$' || !pos || !neg) {
              fprintf(stderr, "Command-Format: $$off(#)?positive:negative$$");
              service_not_available(out);
              fclose(in);
              return;
            }
            if (debug)
              fprintf(stderr,"\nOFF(%s)\n",num);
            if (sispm_switch_off(udev,id,atoi(num)) !=0)
              send(out,pos,neg-pos-1,0);
            else
              send(out,neg,trm-neg,0);
          } else if (strncasecmp(cmd,"toggle(",7)==0) {
            if (trm[1] != '$' || !pos || !neg) {
              fprintf(stderr, "Command-Format: $$toggle(#)?positive:negative$$");
              service_not_available(out);
              fclose(in);
              return;
            }
            if (debug)
              fprintf(stderr,"\nTOGGLE(%s)\n",num);
            if (sispm_switch_getstatus(udev,id,atoi(num)) == 0) {
              sispm_switch_on(udev,id,atoi(num));
              send(out,pos,neg-pos-1,0);
            } else {
              sispm_switch_off(udev,id,atoi(num));
              send(out,neg,trm-neg,0);
            }
          } else if (strncasecmp(cmd,"status(",7)==0) {
            if (trm[1] != '$' || !pos || !neg) {
              fprintf(stderr, "Command-Format: $$status(#)?positive:negative$$");
              service_not_available(out);
              fclose(in);
              return;
            }
            if (debug)
              fprintf(stderr,"\nSTATUS(%s)\n",num);
            if (sispm_switch_getstatus(udev,id,atoi(num)) != 0)
              send(out,pos,neg-pos-1,0);
            else
              send(out,neg,trm-neg,0);
          } else if (strncasecmp(cmd, "version(", 8) == 0) {
            if (trm[1] != '$') {
              fprintf(stderr, "Command-Format: $$version()$$");
              service_not_available(out);
              fclose(in);
              return;
            }
            if (debug)
              fprintf(stderr,"\nVERSION(%s)\n",num);
            send(out, PACKAGE_VERSION, strlen(PACKAGE_VERSION), 0);
          } else {
            send(out,"$$",2,0);
          }
          remlen=remlen-(2+trm-mrk);
          mrk=ptr=&trm[2];
        }
      }
    }
    send(out, mrk, remlen, 0);
    memset(xbuffer, 0, BSIZE);
    retvalue = fgets(xbuffer, BSIZE-1, in);
    remlen = length = ftell(in) - lastpos;
    lastpos = ftell(in);
  }

  if (udev != NULL) {
    usb_close(udev);
    udev = NULL;
  }
  fclose(in);
  return;
}
#endif

