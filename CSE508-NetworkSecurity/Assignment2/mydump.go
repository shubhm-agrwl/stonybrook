package main

import (
	"fmt"
	"log"
	"strings"
)
import "flag"
import "encoding/hex"
import "github.com/google/gopacket"
import "github.com/google/gopacket/pcap"
import "github.com/google/gopacket/layers"

var (
	handle *pcap.Handle
)

func main() {

	// Pointers to accept flags from the user
	iPtr := flag.String("i", "nil", "string")
	rPtr := flag.String("r", "nil", "string")
	sPtr := flag.String("s", "nil", "string")

	flag.Parse()

	// list of all interfaces of the current machine
	devices, err := pcap.FindAllDevs()
	if err != nil {
		log.Fatal(err)
	}

	// variable to check for proper input
	validInput := 0
	expression := "nil"

	// trailing characters input is assigned to expression variable
	if len(flag.Args()) != 0 {
		expression = flag.Arg(0)
	}

	// if pcap file is given, it will start reading from the pcap file
	// overriding the interface input
	if *rPtr != "nil" {
		fmt.Println("r:", *rPtr)
		printFromPcap(*rPtr, *sPtr, expression)
		validInput = 1
	} else if *iPtr != "nil" {

		// if pcap file is not given, it will check for interface input
		fmt.Println("i:", *iPtr)
		printInterface(*iPtr, *sPtr, expression)
		validInput = 1
	}

	// if no valid input, by default it will read for the first device interface
	if validInput == 0 {
		fmt.Println("\nNo or unknown parameters; Initializing dump for Default Ethernet Name: ", devices[0].Name)
		printInterface(devices[0].Name, *sPtr, expression)
	}
}

func printInterface(interfacePointer string, stringValue string, expressionPointer string) {

	// For Interface, opening a live tcp dump connection
	if handle, err := pcap.OpenLive(interfacePointer, 1600, true, pcap.BlockForever); err != nil {
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
			if stringValue != "nil" {
				ipLayer := packet.Layer(layers.LayerTypeIPv4)
				if ipLayer != nil {
					ip, _ := ipLayer.(*layers.IPv4)
					// Checking for String pattern matching
					if strings.Contains(string(ip.Payload), stringValue) {
						// Printing the packet in the required format
						printPacket(packet)
					}
				}
			} else {
				// if String pattern matching is not present, just printing in required format
				printPacket(packet)
			}

			// Check for errors
			if err := packet.ErrorLayer(); err != nil {
				fmt.Println("Error decoding some part of the packet:", err)
			}
		}

	}
}

func printFromPcap(filePointer string, stringValue string, expressionPointer string) {

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
			if stringValue != "nil" {
				ipLayer := packet.Layer(layers.LayerTypeIPv4)
				if ipLayer != nil {
					ip, _ := ipLayer.(*layers.IPv4)
					// Checking for String pattern matching
					if strings.Contains(string(ip.Payload), stringValue) {
						// Printing the packet in the required format
						printPacket(packet)
					}
				}
			} else {
				// if String pattern matching is not present, just printing in required format
				printPacket(packet)
			}
			// Check for errors
			if err := packet.ErrorLayer(); err != nil {
				fmt.Println("Error decoding some part of the packet:", err)
			}
		}
	}
}

func printPacket(packet gopacket.Packet) {

	// Print time as per the required format
	printTime(packet)

	// Extracting ethernet layer data from the packet
	ethernetLayer := packet.Layer(layers.LayerTypeEthernet)

	// Printing ethernet layer contents from the ethernet packet
	printEthernetLayer(ethernetLayer)

	// Printing length of the packet
	fmt.Print(" len ")
	fmt.Print(packet.Metadata().Length)

	// Printing IP and Tcp later packet
	printIpTcpLayer(packet)

	// Printing Payload
	printPayload(ethernetLayer)
}

func printTime(packet gopacket.Packet) {
	time := packet.Metadata().Timestamp.Format("2006-01-02 15:04:05.000000")
	fmt.Print(time, " ")
}

func printEthernetLayer(ethernetPacketLayer gopacket.Layer) {
	if ethernetPacketLayer != nil {
		// Extracting the ethernet packet struct
		ethernetPacket, _ := ethernetPacketLayer.(*layers.Ethernet)

		// Printing the source and destination mac address as per requirement
		fmt.Print(ethernetPacket.SrcMAC)
		fmt.Print(" -> ", ethernetPacket.DstMAC)

		// Printing Ethernet Type in hex value
		fmt.Printf(" type %#x", uint16(ethernetPacket.EthernetType))
	}
}

func printIpTcpLayer(packet gopacket.Packet) {

	ipLayer := packet.Layer(layers.LayerTypeIPv4)
	if ipLayer != nil {

		ip, _ := ipLayer.(*layers.IPv4)
		// Printing Source IP
		fmt.Print(" ")
		fmt.Print(ip.SrcIP)

		tcpLayer := packet.Layer(layers.LayerTypeTCP)
		udpLayer := packet.Layer(layers.LayerTypeUDP)
		// Adding source port to source IP if TCP/UDP
		if tcpLayer != nil {
			tcp, _ := tcpLayer.(*layers.TCP)
			fmt.Print(":")
			fmt.Print(tcp.SrcPort)
		} else if udpLayer != nil {
			udp, _ := udpLayer.(*layers.UDP)
			fmt.Print(":")
			fmt.Print(udp.SrcPort)
		}

		// Printing Destination IP
		fmt.Print(" -> ")
		fmt.Print(ip.DstIP)
		if tcpLayer != nil {
			tcp, _ := tcpLayer.(*layers.TCP)

			// Adding Destination Port to destination IP if TCP/UDP
			fmt.Print(":")
			fmt.Print(tcp.DstPort)
		} else if udpLayer != nil {
			udp, _ := udpLayer.(*layers.UDP)
			fmt.Print(":")
			fmt.Print(udp.DstPort)
		}

		// Printing IP Protocol
		fmt.Print(" ")
		fmt.Print(ip.Protocol)
		fmt.Print(" ")

		// Printing TCP Flags & Payload for TCP Packet
		if tcpLayer != nil {
			tcp, _ := tcpLayer.(*layers.TCP)
			if tcp.SYN {
				fmt.Print("SYN ")
			}
			if tcp.ACK {
				fmt.Print("ACK ")
			}
			if tcp.FIN {
				fmt.Print("FIN ")
			}
			if tcp.RST {
				fmt.Print("RST ")
			}
			if tcp.PSH {
				fmt.Print("PSH ")
			}
			if tcp.URG {
				fmt.Print("URG ")
			}
			if tcp.ECE {
				fmt.Print("ECE ")
			}
			if tcp.CWR {
				fmt.Print("CWR ")
			}
			if tcp.NS {
				fmt.Print("NS ")
			}
		}
	}
}

func printPayload(ethernetPacketLayer gopacket.Layer) {
	if ethernetPacketLayer != nil {
		ethernetPacket, _ := ethernetPacketLayer.(*layers.Ethernet)
		fmt.Println()
		fmt.Printf("%s", hex.Dump(ethernetPacket.Payload))
		fmt.Println()
	}
}
