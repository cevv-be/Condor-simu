package main

/*

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
	"path"
	"strconv"
	"strings"
	"sync"
	"time"
	//"net/http/pprof"
)

// http service configuration
type httpConfig struct {
	Port       int    // port to listen on
	MetricPath string // prefix the json is responding
	DashPath   string // prefix for the Dashboard files
	DashDir    string // where the dashboard files are located (Dashboard folder of the git)
}
type udpConfig struct {
	UdpPort      int    // port to listen to Condor udp messages
	UdpRelayAddr string //
}

// Structure of the json configuration file
type Config struct {
	Http         httpConfig
	Udp          udpConfig
	MetricsUnits map[string]string
}

var dashDir string
var dashPath string

// This is the json output structure shared between the udp and http routine.
//for safety, a mutex is used
var mutex sync.RWMutex
var output struct {
	Latest  int64
	Metrics map[string]float64
	Units   map[string]string
}

// go routine (coroutine) to receive and process Condor udp packets
func receiveUDP(udpconfig udpConfig) {
	var Udpconn *net.UDPConn = nil

	// open and udpcon if we have to relay
	if udpconfig.UdpRelayAddr != "" {

		udpAddr, err := net.ResolveUDPAddr("up4", udpconfig.UdpRelayAddr)
		if err != nil {
			fmt.Println("could not resolve udp address", err)
		}

		Udpconn, err = net.DialUDP("udp", nil, udpAddr)
		if err != nil {
			fmt.Println("could not open relay udp conn", err)
		}
		fmt.Println("UDP packets will be relayed to " + udpconfig.UdpRelayAddr)
	}

	// create a listener
	socket, err := net.ListenUDP("udp4", &net.UDPAddr{
		IP:   net.IPv4(0, 0, 0, 0),
		Port: udpconfig.UdpPort,
	})
	if err != nil {
		fmt.Println("UDP listening failed!", err)
		panic(err)
	}

	// loop forever to receive udp packets
	for {
		// get an udp packet
		data := make([]byte, 8192)
		read, _, err := socket.ReadFromUDP(data)
		if err != nil {
			fmt.Println("read data failed!", err)
			continue
		}

		// if needed relay udpPacket to other consumer
		if Udpconn != nil {
			_, err = Udpconn.Write(data)
			if err != nil {
				fmt.Println("failed to forward udp packet", err)
			}
		}

		// fill in the metrics value map with the values
		if read > 0 {
			// split the udp packet by lines
			metrics := bytes.Split(data, []byte("\r\n"))

			// critical section, for data integrity, a RW mutex is used
			mutex.Lock()
			for _, metric := range metrics {
				// split metric at the = sign to get name and value
				fields := bytes.Split(metric, []byte("="))

				// not a clean metric/value pair, so skip
				if len(fields) < 2 {
					continue
				}

				// check if received metric is needed (initialized)
				mymetric := string(fields[0])
				if _, ok := output.Metrics[mymetric]; ok {
					// update the value, (non atomic operation, risk...)
					value, err := strconv.ParseFloat(string(fields[1]), 64)
					if err != nil {
						fmt.Println("failed to parse metric "+mymetric+" ", err)
					}
					output.Metrics[mymetric] = value
				}
			}
			output.Latest = time.Now().Unix()
			mutex.Unlock()
		}
	}
}

// Serve the json response
func serveJson(w http.ResponseWriter, r *http.Request) {

	//Build json from metricsValues map
	mutex.RLock()
	json, err := json.Marshal(output)
	mutex.RUnlock()
	if err != nil {
		fmt.Println("failed to marshal json", err)
		return
	}
	//io.WriteString(w, "URL: "+r.URL.String())
	w.WriteHeader(http.StatusOK)
	w.Header().Set("Content-Type", "application/json")

	_, err = io.WriteString(w, strings.ToLower(string(json)))
	if err != nil {
		fmt.Println("failed to send json", err)
		return
	}
}

// Serve the Dashboard and associated files (files present in the same directory are served)
func serveDash(w http.ResponseWriter, r *http.Request) {

	File := strings.TrimPrefix(r.URL.Path, dashPath)
	http.ServeFile(w, r, dashDir+File)
	return

}

// the http server, never return
func serverHttp(httpConfig httpConfig) {
	// and now prepare the http server for the json metrics and the dashboard pages
	mux := http.NewServeMux()

	// register metric serve function
	mux.HandleFunc(httpConfig.MetricPath, serveJson)

	// serve Dashboards function
	mux.HandleFunc(dashPath, serveDash)

	/* for performance monitoring
	mux.HandleFunc("/debug/pprof/", pprof.Index)
	mux.HandleFunc("/debug/pprof/cmdline", pprof.Cmdline)
	mux.HandleFunc("/debug/pprof/profile", pprof.Profile)
	mux.HandleFunc("/debug/pprof/symbol", pprof.Symbol)
	mux.HandleFunc("/debug/pprof/trace", pprof.Trace)
	*/

	//http.ListenAndServe uses the default server structure.
	err := http.ListenAndServe(":"+strconv.Itoa(httpConfig.Port), mux)
	if err != nil {
		log.Fatal(err)
	}
}

func readConfigFile() Config {

	var config Config

	// chdir to executable file (in order to run as a windows service)
	execFile, _ := os.Executable()
	execPath := path.Dir(execFile)
	err := os.Chdir(execPath)
	if err != nil {
		fmt.Println("could not chDir to "+execPath, err)
		panic(err)
	}

	// configuration file is expected to lay with the executable by default
	configFilename := flag.String("c", "./udp2json.json", "config file, json format")
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

	err = json.Unmarshal(configContent, &config)
	if err != nil {
		fmt.Println("could not parse config file", err)
		panic(err)
	}

	dashDir = config.Http.DashDir
	dashPath = config.Http.DashPath

	// initialize the units from the json file
	// and set initial value for the metrics
	output.Units = make(map[string]string)
	output.Metrics = make(map[string]float64)
	for metric, value := range config.MetricsUnits {
		output.Units[metric] = value
		output.Metrics[metric] = 1.0
	}

	return config
}

func main() {

	// read config and initialize the output structure (metrics and units)
	config := readConfigFile()

	// Startup UDP server as a go routine
	go receiveUDP(config.Udp)

	// the http server does no return
	serverHttp(config.Http)

}
