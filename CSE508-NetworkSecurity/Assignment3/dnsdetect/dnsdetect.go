package main

import (
	"fmt"
	"github.com/google/gopacket/layers"
	"log"
	"net"
	"sync"
	"time"
)
import "flag"
import "github.com/google/gopacket"
import "github.com/google/gopacket/pcap"

type safeDns struct {
	mu sync.Mutex
	v  map[uint16]*layers.DNS
}

type safeTime struct {
	mu sync.Mutex
	v  map[uint16]time.Time
}

type safeIp struct {
	mu sync.Mutex
	v  map[uint16]net.IP
}

var dnsDetect = safeDns{v: make(map[uint16]*layers.DNS)}
var dnsTime = safeTime{v: make(map[uint16]time.Time)}
var dnsIp = safeIp{v: make(map[uint16]net.IP)}

func main() {

	// Pointers to accept flags from the user
	iPtr := flag.String("i", "nil", "string")
	rPtr := flag.String("r", "nil", "string")

	flag.Parse()

	// list of all interfaces of the current machine
	devices, err := pcap.FindAllDevs()
	if err != nil {
		log.Fatal(err)
	}

	expression := "nil"

	// trailing characters input is assigned to expression variable
	if len(flag.Args()) != 0 {
		expression = flag.Arg(0)
	}

	defaultInterface := devices[0].Name
	if *iPtr != "nil" {
		defaultInterface = *iPtr
	}

	// if pcap file is given, it will start reading from the pcap file
	// overriding the interface input
	if *rPtr != "nil" {

		readFromPcap(*rPtr, expression)
	} else {

		// initialize Go Thread for Live DNS Spoof detection
		liveDump(defaultInterface, expression)
	}
}

func liveDump(defaultInterface string, expression string) {

	// For Interface, opening a live tcp dump connection
	handle, err := pcap.OpenLive(defaultInterface, 1600, true, pcap.BlockForever)
	if err != nil {
		panic(err)
	} else {

		// Setting BPFFilter if present
		if expression != "nil" {
			if err := handle.SetBPFFilter(expression); err != nil {
				panic(err)
			}
		}
	}

	// Getting packet from the handle
	packetSource := gopacket.NewPacketSource(handle, handle.LinkType())

	// Looping through the packet
	for packet := range packetSource.Packets() {
		detectDNSSpoof(packet)
		// Check for errors
		if err := packet.ErrorLayer(); err != nil {
			fmt.Println("Error decoding some part of the packet:", err)
		}
	}
}

func readFromPcap(filePointer string, expressionPointer string) {

	if handle, err := pcap.OpenOffline(filePointer); err != nil {
		panic(err)
	} else {

		// Setting BPFFilter if present
		if expressionPointer != "nil" {
			if err := handle.SetBPFFilter(expressionPointer); err != nil {
				panic(err)
			}
		}

		// Getting packet from the handle
		packetSource := gopacket.NewPacketSource(handle, handle.LinkType())

		// Looping through the packet
		for packet := range packetSource.Packets() {
			detectDNSSpoof(packet)
			// Check for errors
			if err := packet.ErrorLayer(); err != nil {
				fmt.Println("Error decoding some part of the packet:", err)
			}
		}
	}
}

func detectDNSSpoof(packet gopacket.Packet) {

	ipLayer := packet.Layer(layers.LayerTypeIPv4)
	udpLayer := packet.Layer(layers.LayerTypeUDP)
	dnsLayer := packet.Layer(layers.LayerTypeDNS)
	if ipLayer != nil && udpLayer != nil && dnsLayer != nil {
		ip, _ := ipLayer.(*layers.IPv4)
		dns, _ := dnsLayer.(*layers.DNS)
		if dns.QR == true {

			if (len(dnsTime.v)) > 500 {
				// Calling the thread to remove all the DNS packets which is 5 seconds before
				go flushData(packet.Metadata().Timestamp)
			}

			dnsLyr := dnsDetect.v[dns.ID]
			if dnsLyr == nil {
				dnsDetect.mu.Lock()
				dnsDetect.v[dns.ID] = dns
				dnsDetect.mu.Unlock()
				dnsTime.mu.Lock()
				dnsTime.v[dns.ID] = packet.Metadata().Timestamp
				dnsTime.mu.Unlock()
				dnsIp.mu.Lock()
				dnsIp.v[dns.ID] = ip.DstIP
				dnsIp.mu.Unlock()
			} else {
				if packet.Metadata().Timestamp.Sub(dnsTime.v[dns.ID]).Seconds() < 3 && dnsIp.v[dns.ID].Equal(ip.DstIP) {
					fmt.Print(packet.Metadata().Timestamp.Format("20060102-15:04:05.000000"))
					fmt.Println(" DNS poisoning attempt")
					fmt.Print("TXID ")
					fmt.Print(dns.ID)
					fmt.Print(" Request ")
					fmt.Println(string(dns.Questions[0].Name))
					fmt.Print("Answer 1 ")
					for i := 0; i < len(dnsLyr.Answers); i++ {
						fmt.Print(dnsLyr.Answers[i].IP)
						fmt.Print(" ")
					}
					fmt.Println()
					fmt.Print("Answer 2 ")
					for i := 0; i < len(dns.Answers); i++ {
						fmt.Print(dns.Answers[0].IP)
						fmt.Print(" ")
					}
					fmt.Println()
				} else {
					dnsDetect.mu.Lock()
					dnsDetect.v[dns.ID] = dns
					dnsDetect.mu.Unlock()
					dnsTime.mu.Lock()
					dnsTime.v[dns.ID] = packet.Metadata().Timestamp
					dnsTime.mu.Unlock()
					dnsIp.mu.Lock()
					dnsIp.v[dns.ID] = ip.DstIP
					dnsIp.mu.Unlock()
				}
			}
		}
	}
}

func flushData(t time.Time) {
	x := []uint16{}

	dnsTime.mu.Lock()
	for key, value := range dnsTime.v {
		if t.Sub(value).Seconds() > 5 {
			x = append(x, key)
		}
	}
	dnsTime.mu.Unlock()

	// Deleting Records which is more than 5 seconds before
	for i := 0; i < len(x); i++ {
		dnsDetect.mu.Lock()
		delete(dnsDetect.v, x[i])
		dnsDetect.mu.Unlock()
		dnsTime.mu.Lock()
		delete(dnsTime.v, x[i])
		dnsTime.mu.Unlock()
		dnsIp.mu.Lock()
		delete(dnsIp.v, x[i])
		dnsIp.mu.Unlock()
	}
}
