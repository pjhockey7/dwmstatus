/*
 * Copy me if you can.
 * by 20h
 */

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

char *tzargentina = "America/Buenos_Aires";
char *tzutc = "UTC";
char *tzberlin = "Europe/Berlin";
char* tzlos_angeles = "America/Los_Angeles";
char* tzpst = "PST";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char*  get_meminfo(void)
{
  	FILE *fd;
  	char buff[128];
	unsigned long mem_total;
	unsigned long mem_free;;
	char dump_names[64];
  	fd = fopen("/proc/meminfo", "r"); 
  	fgets(buff, sizeof(buff), fd); 
  	sscanf(buff, "%s %lu ", dump_names, &mem_total); 
  	//if(fd <0) return smprintf("Error opening /proc/meminfo");

	fgets(buff, sizeof(buff), fd); 
  	sscanf(buff, "%s %lu ", dump_names, &mem_free);
  
  	fclose(fd);
	unsigned int free = mem_free;
	char unit = 'K';
	if(mem_free > 1000000)
	{
		unit = 'G';
		free = mem_free / 1000000;
	}
	else if(mem_free > 1000)
	{
		unit = 'M';
		free = mem_free / 1000;
	}
	
	unsigned int total = mem_total/1000000;	
	return smprintf("%d%c|%dG",free,unit,total);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf("");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s", buf);
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char *
loadavg(void)
{
	double avgs[3];

	if (getloadavg(avgs, 3) < 0)
		return smprintf("");

	//return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
	return smprintf("%d|%d", (int)(avgs[0]*100), (int)(avgs[2]*100));
}

char *
readfile(char *base, char *file)
{
	char *path, line[513];
	FILE *fd;

	memset(line, 0, sizeof(line));

	path = smprintf("%s/%s", base, file);
	fd = fopen(path, "r");
	free(path);
	if (fd == NULL)
		return NULL;

	if (fgets(line, sizeof(line)-1, fd) == NULL)
		return NULL;
	fclose(fd);

	return smprintf("%s", line);
}

char *
getbattery(char *base)
{
	char *co, status;
	int descap, remcap;

	descap = -1;
	remcap = -1;

	co = readfile(base, "present");
	if (co == NULL)
		return smprintf("");
	if (co[0] != '1') {
		free(co);
		return smprintf("not present");
	}
	free(co);

	co = readfile(base, "charge_full_design");
	if (co == NULL) {
		co = readfile(base, "energy_full_design");
		if (co == NULL)
			return smprintf("");
	}
	sscanf(co, "%d", &descap);
	free(co);

	co = readfile(base, "charge_now");
	if (co == NULL) {
		co = readfile(base, "energy_now");
		if (co == NULL)
			return smprintf("");
	}
	sscanf(co, "%d", &remcap);
	free(co);

	co = readfile(base, "status");
	if (!strncmp(co, "Discharging", 11)) {
		status = '-';
	} else if(!strncmp(co, "Charging", 8)) {
		status = '+';
	} else {
		status = '?';
	}

	if (remcap < 0 || descap < 0)
		return smprintf("invalid");

	return smprintf("%.0f%%%c", ((float)remcap / (float)descap) * 100, status);
}

char *
gettemperature(char *base, char *sensor)
{
	char *co;

	co = readfile(base, sensor);
	if (co == NULL)
		return smprintf("");
	return smprintf("%02.0f°C", atof(co) / 1000);
}

int
main(void)
{
	char *status;
	char *avgs;
	char *bat;
	//char *bat1;
	//char *tmar;
	//char *tmutc;
	//char *tmbln;
	char *t0;//, *t1, *t2;
	char* tmla;
	char* mem_info;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(60)) {
		avgs = loadavg();
		bat = getbattery("/sys/class/power_supply/BAT0");
		//bat1 = getbattery("/sys/class/power_supply/BAT1");
		tmla = mktimes("%m/%d/%Y %I:%M", tzlos_angeles);
		//tmutc = mktimes("%H:%M", tzpst);
		//tmbln = mktimes("KW %W %a %d %b %H:%M %Z %Y", tzberlin);
		t0 = gettemperature("/sys/devices/virtual/thermal/thermal_zone0/hwmon1", "temp1_input");
		//t1 = gettemperature("/sys/devices/virtual/thermal/thermal_zone0/hwmon", "temp1_input");
		//t2 = gettemperature("/sys/devices/virtual/hwmon/hwmon4", "temp1_input");
		mem_info = get_meminfo();
		
		status = smprintf("B:%s T:%s M:%s L:%s %s",bat, t0, mem_info, avgs, tmla);
		setstatus(status);

		free(t0);
		//free(t1);
		//free(t2);
		free(avgs);
		free(bat);
		//free(bat1);
		//free(tmar);
		//free(tmutc);
		//free(tmbln);
		free(tmla);
		free(mem_info);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}

