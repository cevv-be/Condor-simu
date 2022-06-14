package main

/* receive UDP datagram from Condor:
	time=12.0979208352778
	airspeed=24.9187889099121
	altitude=529.89111328125
	vario=-0.515239298343658
	evario=-0.577091097831726
	nettovario=0.191792264580727
	integrator=-0.860146880149841
	compass=104.283294677734
	slipball=0.00367661309428513
..
	gforce=0.987691422364277
and serve it a json:
{
    "airspeed:km/h": 0.0,
    "altitude:Feet": 108.50824737548828,
..
    "turnrate:Radians per second": 7.495487400080828e-07,
    "verticalSpeed:feet/minute": -0.03363388823345297
}
Firewall authorisation (windows11):
netsh advfirewall firewall add rule name="Condoir UDP metrics" dir=in action=allow protocol=UDP localport=55278
*/
import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"
)

// For each Condor metric to report to the dashboard, the unit to send
//var metricsUnit = map[string]string{}

type httpConfig struct {
	Port       int
	MetricPath string
	DashPath   string
	DashDir    string
}
type udpConfig struct {
	UdpPort      int
	UdpRelayAddr string
}
type Config struct {
	Http         httpConfig
	Udp          udpConfig
	MetricsUnits map[string]string
}

var UDPport int
var Udpconn *net.UDPConn
var dashPath string
var dashDir string

// Actual map of metric -> value
type valueUnit struct {
	Value float64
	Unit  string
}

var output struct {
	Latest  int64
	Metrics map[string]float64
	Units   map[string]string
}

// go routine (coroutine) to receive and process Condor udp packets
func receiveUDP() {
	// create a listener
	socket, err := net.ListenUDP("udp4", &net.UDPAddr{
		IP:   net.IPv4(0, 0, 0, 0),
		Port: UDPport,
	})
	if err != nil {
		fmt.Println("listening failed!", err)
		return
	}

	for {
		// read data
		data := make([]byte, 4096)
		read, _, err := socket.ReadFromUDP(data)
		if err != nil {
			fmt.Println("read data failed!", err)
			continue
		}

		// relay udpPacket to other consumer if needed
		if Udpconn != nil {
			_, err = Udpconn.Write(data)
			if err != nil {
				fmt.Println("failed to forward udp packet", err)
				continue
			}
		}
		// fill the metricsValues map with the values and the units.
		if read > 0 {
			// split the udp packet by lines
			metrics := bytes.Split(data, []byte("\r\n"))
			for _, metric := range metrics {
				// split metric at the = sign to get name and value
				fields := bytes.Split(metric, []byte("="))
				// check if received metric is needed and associate to unit
				mymetric := string(fields[0])
				if _, ok := output.Units[mymetric]; ok {
					// update the value, (non atomic operation, risk...)
					number, _ := strconv.ParseFloat(string(fields[1]), 64)
					output.Metrics[mymetric] = number
				}
				output.Latest = time.Now().Unix()
			}

		}
	}
}

// Serve the json response
func serveJson(w http.ResponseWriter, r *http.Request) {

	//io.WriteString(w, "URL: "+r.URL.String())
	w.WriteHeader(http.StatusOK)
	w.Header().Set("Content-Type", "application/json")
	//Build json from metricsValues map
	output.Latest = time.Now().Unix()
	json, _ := json.Marshal(output)
	io.WriteString(w, strings.ToLower(string(json)))
}

// Serve the Dashboard
func serveDash(w http.ResponseWriter, r *http.Request) {

	File := strings.TrimPrefix(r.URL.Path, dashPath)
	http.ServeFile(w, r, dashDir+File)
	return

}

func main() {

	configFilename := flag.String("c", "./udp2json.json", "config file, toml format")
	flag.Parse()

	// load config file
	file, err := os.Open(*configFilename)
	if err != nil {
		fmt.Println("could not open config file "+*configFilename, err)
		panic(err)
	}
	configContent, err := ioutil.ReadAll(file)
	if err != nil {
		fmt.Println("could not read config file", err)
		panic(err)
	}

	var config Config
	err = json.Unmarshal(configContent, &config)
	if err != nil {
		fmt.Println("could not parse config file", err)
		panic(err)
	}

	UDPport = config.Udp.UdpPort
	UdpRelayAddr := config.Udp.UdpRelayAddr
	dashDir = config.Http.DashDir
	dashPath = config.Http.DashPath

	output.Units = make(map[string]string)
	for key, value := range config.MetricsUnits {
		output.Units[key] = value
	}

	// open and udpcon if we have to relay
	if UdpRelayAddr != "" {

		udpAddr, err := net.ResolveUDPAddr("up4", UdpRelayAddr)
		if err != nil {
			fmt.Println("could not resolve udp address", err)
		}

		Udpconn, err = net.DialUDP("udp", nil, udpAddr)
		if err != nil {
			fmt.Println("could not open udp conn", err)
		}
	}

	// initialise metrics to be returned to 0
	output.Metrics = make(map[string]float64)
	for metric, _ := range output.Units {
		output.Metrics[metric] = 1
	}

	// add pressureSetting mettrics, fixed value 1013mb
	//output.Metrics["pressureSetting"] = 1013.25
	//output.Units["pressureSetting"] = "hPa"

	// Startup UDP server as a go routine
	go receiveUDP()

	mux := http.NewServeMux()

	// metric serve function
	mux.HandleFunc(config.Http.MetricPath, serveJson)

	// serve Dashboards
	mux.HandleFunc(dashPath, serveDash)

	//http.ListenAndServe uses the default server structure.
	err = http.ListenAndServe(":"+strconv.Itoa(config.Http.Port), mux)
	if err != nil {
		log.Fatal(err)
	}

}
