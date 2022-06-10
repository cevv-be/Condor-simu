# Condor-simu


Here are the source code files used for [CEVV|http://www.cevv.be] Flight simulator.

The Simulator uses a converted K7 front uselage as basis.The flight simulator software is Condor Soaring.

The elements are:

# Kicad pcbs:
- Arduino based controller with 16bit ADC and support for a number of buttons.
- RJ45 cable relay

# Dash boards
Each seat enjoys a 8" LCD screen as a dynamic dashboard.

The dashboard is build using the noce G3 library.

# UDP2json

The udp2json program receives the UDP packet send by Condor to a json served by a build in web server.
The software also serve the dashboard html and javascripts.
It can also relay the UDP packets to another consumer.
