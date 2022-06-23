# UDP2Json

Udp2json is the glue in between the *Condor* flight simulator software (https://www.condorsoaring.com/) and the simulator dashboard implemented as an HTML/javascript page.

udp2json receives the udp packets sent by Condor with metric updates (speed, altitude, ...) and serves them as the json metrics as expected by the g3 library.

udp2json is written in the go language and compiled to an executable. Go lang is inherently parallel and allows the UDP reception and the http serving to be independant coroutines.

## Activate UDP logging in Condor

To get the parameter output from *Condor* as UDP packets, 

Edit the *C:\Condor2\Settings\udp.init* as:
<pre>
[General]
Enabled=<b style='color:red;'>1</b>

[Connection]
Host=127.0.0.1
Port=<b style='color:red;'>55278</b>

[Misc]
SendIntervalMs=<b style='color:red;'>50</b>
ExtendedData=0
LogToFile=0
</pre>

The SendIntervalMs sets the interval between new IDP packets, it should be lower to (but close) to the interval between URL updates set in the dashboard html file.

*Condor* also provides another output serial NMEA intended for flight computer simulation. This output is not yet used.

## Udp2json as a web server

Udpjson serves the files in the *dashdir* folder under the (http://localhost\:port/dashpath/) prefix as http. For instance (http://localhost/Dash/Dash.html).
 are present in the folder:
 - minimised version (.min.) of
    - g3 library
    - d3 library
    - The dashboard itself (Dash.html)

Using an http server avoid CORS filter issues.

The windows firewall may have to be configured to autorise the http server.

## udp2jsonJson metric serving

The Condor metrics are served as (http://localhost:8080/metrics/condor.json), then can be tested using a browser:

```
{"latest":1655901801,"metrics":{"airspeed":27.3834495544434,"altitude":293.178405761719,"compass":289.132141113281,"evario":6.23323202133179,"gforce":1.13908882413314,"integrator":1.39119529724121,"nettovario":7.02899980545044,"slipball":-0.0165308639407158,"time":12.0144775356667,"vario":6.95600605010986},"units":{"airspeed":"m/s","altitude":"m","compass":"deg","evario":"m/s","gforce":"g","integrator":"m/s","nettovario":"m/s","slipball":"rad","time":"h","vario":"m/s"}}
```

This can be reformated for readability as: 

```
{
  "latest": 1655901801,
  "metrics": {
    "airspeed": 27.3834495544434,
    "altitude": 293.178405761719,
    "compass": 289.132141113281,
    "evario": 6.23323202133179,
    "gforce": 1.13908882413314,
    "integrator": 1.39119529724121,
    "nettovario": 7.02899980545044,
    "slipball": -0.0165308639407158,
    "time": 12.0144775356667,
    "vario": 6.95600605010986
  },
  "units": {
    "airspeed": "m/s",
    "altitude": "m",
    "compass": "deg",
    "evario": "m/s",
    "gforce": "g",
    "integrator": "m/s",
    "nettovario": "m/s",
    "slipball": "rad",
    "time": "h",
    "vario": "m/s"
  }
}
```

## udp2json.json configuration file

The udp2json*.json* file stores the configuration parameters to udp2 json.

The sections of the file are:
- http http server settings, patch to json and dash pages, location of the dash pages.
    - udp:
      - liste port
      - relay addr: an *ip address: port number* pair for a downstream additional condor udp client (i.e. 6DOF platform)
- metrics unit:
        list of *metric name/metric pair* 
        Only the metrics with a unit, presnet in the files are served to the dashboard. 

Available metrics are documented in the "Cockpit Builders" chapter of the condor manual.

```
{   
    "http" : {
        "port": 8080,
        "metricpath": "/metrics/condor.json",
        "dashpath": "/dash/",
        "dashdir":"../Dashboard/"
        },
    "udp" : {
		"udpport":55278,
		"udpRelayAddr" :""
	},
    "metricsunits":{
		"time":"h",
		"airspeed":"m/s",
		"altitude":"m",
		"vario":"m/s",
		"evario":"m/s",
		"nettovario":"m/s",
		"integrator":"m/s",
		"compass":"deg",
		"slipball":"rad",
		"gforce": "g"
	}
}
```

## Compile to executable

To compile the udp2json.go to an executable, use the following command in the folder where the udp2json go program resides:

```
>  go build -ldflags "-s -w"    
```

## Install as a Windows service

A simple way to set the udp2json as a windows service, start up with the computer is to use the nssm (http://nssm.cc/) software to set up the service.
As the nssm site is not readilly available, use the chocolatey package nssm to install the software.

A dump of the configuration is:

```
nssm.exe install udp2json %%PATH_TOUDP2JSON_FOLDER\udp2json.exe
nssm.exe set udp2json AppDirectory %%PATH_TOUDP2JSON_FOLDER
nssm.exe set udp2json AppExit Default Restart
nssm.exe set udp2json AppNoConsole 1
nssm.exe set udp2json AppStdout %%PATH_TOUDP2JSON_FOLDER\udp2json_stdout.log
nssm.exe set udp2json AppStderr %%PATH_TOUDP2JSON_FOLDER\udp2json_stderr.log
nssm.exe set udp2json AppThrottle 3000
nssm.exe set udp2json AppTimestampLog 1
nssm.exe set udp2json Description "Receive Condor UDP and supply json"
nssm.exe set udp2json DisplayName "Condor  UDP relay"
nssm.exe set udp2json ObjectName LocalSystem
nssm.exe set udp2json Start SERVICE_AUTO_START
nssm.exe set udp2json Type SERVICE_WIN32_OWN_PROCESS
```
