package main

import (
	"bufio"
	"crypto/aes"
	"crypto/cipher"
	"crypto/rand"
	"crypto/sha256"
	"flag"
	"fmt"
	"golang.org/x/crypto/pbkdf2"
	"io"
	"log"
	"net"
	"os"
)

var paraphrase string

func main() {

	listen := flag.String("l", "nil", "Listen Port")
	pwdFile := flag.String("p", "nil", "PWD File Location")

	flag.Parse()
	if len(flag.Args()) < 2 {
		log.Fatal("Hostname and port required")
	}

	if *pwdFile == "nil" {
		log.Fatal("Missing password file input")
	}

	// Reading PWD File
	paraphrase = readPwdFile(*pwdFile)
	log.Println("Paraphrase: ", paraphrase)

	serverHost := flag.Arg(0)
	serverPort := flag.Arg(1)

	if *listen == "nil" {
		// If Listen port is not provided, it starts as a client
		forwardProxy(fmt.Sprintf("%s:%s", serverHost, serverPort))
	} else {
		// If Listen port is provided, initiates the reverse Proxy
		// By default uses the default IP of the host system
		reverseProxy(fmt.Sprintf("%s:%s", getDefaultIp(), *listen), fmt.Sprintf("%s:%s", serverHost, serverPort))
	}
}

// Reverse Proxy
func reverseProxy(listenAddr string, sendAddr string) {
	log.Println("Listening: ", listenAddr)
	log.Println("Forwarding: ", sendAddr)

	// Initiates a Listen connection

	proxyListener, err := net.Listen("tcp", listenAddr)
	if err != nil {
		panic(err)
	}

	for {
		// Listening for concurrent connections
		listenConn, _ := proxyListener.Accept()

		// Initiates a thread after accepting a new connection
		go processClient(listenConn, sendAddr)
	}
}

func processClient(conn net.Conn, sendAddr string) {

	// Initiates a new connection to the destination Address
	dstConn, err := net.Dial("tcp", sendAddr)
	if err != nil {
		panic(err)
	}

	// Go Function to handle incoming data from the destination Address,
	// encrypting and forwarding it to the Listening connection
	// Quit Variable is used as a channel variable to check whether the
	// connection still exists or not, so that it can break the infinite loop
	quit := make(chan bool)
	go func() {
		for {
			select {
			case <-quit:
				return
			default:
				b := make([]byte, 4096)
				n, err := dstConn.Read(b)
				if err != nil {
					if err != io.EOF {
						log.Println("Read error:", err)
					}
					break
				}

				if n > 0 {
					ciphertext := encrypt(b[:n])
					_, err := conn.Write(ciphertext)
					if err != nil {
						log.Println("Write error: ", err)
					}
				}
			}
		}
	}()

	// Infinite loop to read from listening connection
	// Decrypt and send to the destination address
	for {
		b := make([]byte, 4096)
		n, err := conn.Read(b)
		if err != nil {
			if err != io.EOF {
				log.Println("Read error:", err)
			}
			break
		}
		if n > 0 {
			plaintext := decrypt(b[:n])
			_, _ = dstConn.Write(plaintext)
		}
	}

	// Closing the connection if the client stops
	_ = conn.Close()
	// Quit goroutine
	// Breaking the go thread by initiating quit to true
	quit <- true
}

func forwardProxy(addr string) {

	// Initiates a new connection to the destination Address
	log.Println("Forwarding: ", addr)
	conn, err := net.Dial("tcp", addr)
	if err != nil {
		log.Fatal("Can't connect to server: ", err)
		return
	}
	reader := bufio.NewReader(os.Stdin)

	// Initiating a go thread to read from the connection
	go readData(conn)

	// Infinite Loop to read from StdIn
	// Encrypt and write to destination address
	for {
		data := make([]byte, 4096)

		n, err := reader.Read(data)
		if err != nil {
			if err != io.EOF {
				log.Println("StdIn error: ", err)
			}
			break
		}
		if n > 0 {
			ciphertext := encrypt(data[0:n])
			_, _ = conn.Write(ciphertext)
		}
	}
}

func readData(conn net.Conn) {
	// Reading from the connection
	// Decrypting and writing to Stdout
	writer := bufio.NewWriter(os.Stdout)
	for {
		b := make([]byte, 4096)
		n, err := conn.Read(b)
		if err != nil {
			if err != io.EOF {
				log.Println("Read error:", err)
			}
			break
		}
		if n > 0 {
			plaintext := decrypt(b[:n])
			_, _ = writer.Write(plaintext)
			_ = writer.Flush()
		}
	}
}

// Encrypt Logic
func encrypt(plaintext []byte) []byte {

	// Making Random 8 bit salt
	salt := make([]byte, 8)
	_, _ = rand.Read(salt)

	// Generating PBKDF2 key from the random generated salt and paraphrase
	key := pbkdf2.Key([]byte(paraphrase), salt, 4096, 32, sha256.New)
	block, _ := aes.NewCipher(key)
	aesgcm, _ := cipher.NewGCM(block)

	// Creating random 12 bit Nonce
	nonce := make([]byte, aesgcm.NonceSize())
	_, _ = rand.Read(nonce)

	// Ciphering the text using the key and nonce generated
	ciphertext := aesgcm.Seal(nonce, nonce, plaintext, nil)

	// Appending the salt to the cipher text to send to the other side
	ciphertextWithSalt := append(salt, ciphertext...)

	return ciphertextWithSalt
}

// Decrypting Logic
func decrypt(ciphertextWithSalt []byte) []byte {

	// Extracting the salt and cipher text from the incoming message
	salt, ciphertext := ciphertextWithSalt[:8], ciphertextWithSalt[8:]

	// Generating key from paraphrase and salt received from the data
	key := pbkdf2.Key([]byte(paraphrase), salt, 4096, 32, sha256.New)
	block, _ := aes.NewCipher(key)
	aesgcm, _ := cipher.NewGCM(block)

	// Extracting Nonce from the cipher message
	nonceSize := aesgcm.NonceSize()
	nonce, ciphertextWithoutNonce := ciphertext[:nonceSize], ciphertext[nonceSize:]

	// Getting plain text from the cipher message
	plaintext, err := aesgcm.Open(nil, nonce, ciphertextWithoutNonce, nil)
	if err != nil {
		log.Println(err.Error())
	}
	return plaintext
}

// Getting default IP
func getDefaultIp() string {
	ifaces, _ := net.Interfaces()

	for _, i := range ifaces {
		addrs, _ := i.Addrs()
		for _, addr := range addrs {
			var ip net.IP
			switch v := addr.(type) {
			case *net.IPNet:
				ip = v.IP
			case *net.IPAddr:
				ip = v.IP
			}
			if ip == nil || ip.IsLoopback() {
				continue
			}
			ip = ip.To4()
			if ip == nil {
				continue
			}
			return ip.String()
		}
	}

	return ""
}

// Reading PWD File
func readPwdFile(filePath string) string {
	var fileContent string
	file, err := os.Open(filePath)
	if err != nil {
		log.Fatal(err)
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		fileContent += scanner.Text()
	}

	if err := scanner.Err(); err != nil {
		log.Fatal(err)
	}
	return fileContent
}
