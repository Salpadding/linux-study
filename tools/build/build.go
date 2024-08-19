package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
)

const (
	MINIX_HEADER       = 32
	GCC_HEADER         = 1024
	SYS_SIZE           = 0x3000
	DEFAULT_MAJOR_ROOT = 3
	DEFAULT_MINOR_ROOT = 1
	DEFAULT_MAJOR_SWAP = 0
	DEFAULT_MINOR_SWAP = 0
	SETUP_SECTS        = 4
)

func die(msg string) {
	fmt.Fprintln(os.Stderr, msg)
	os.Exit(1)
}

func usage() {
	die("Usage: build bootsect setup system [rootdev] [> image]")
}

func checkMinix(buf []byte, file string) {
	if binary.LittleEndian.Uint32(buf[:4]) != 0x04100301 ||
		binary.LittleEndian.Uint32(buf[4:8]) != MINIX_HEADER ||
		binary.LittleEndian.Uint32(buf[12:16]) != 0 ||
		binary.LittleEndian.Uint32(buf[16:20]) != 0 ||
		binary.LittleEndian.Uint32(buf[20:24]) != 0 ||
		binary.LittleEndian.Uint32(buf[28:32]) != 0 {
		die(
			fmt.Sprintf("Non-Minix header of '%s'", file),
		)
	}
}

func parseNo(major *int, minor *int, expr string) {
	var err error
	splited := strings.Split(expr, ":")
	*major, err = strconv.Atoi(splited[0])
	if err != nil {
		panic(err)
	}
	*minor, err = strconv.Atoi(splited[1])
	if err != nil {
		panic(err)
	}
}

func main() {
	if len(os.Args) < 4 || len(os.Args) > 6 {
		usage()
	}
	var (
		majorRoot int
		minorRoot int
		majorSwap int
		minorSwap int
	)

	majorRoot = DEFAULT_MAJOR_ROOT
	minorRoot = DEFAULT_MINOR_ROOT
	majorSwap = DEFAULT_MAJOR_SWAP
	minorSwap = DEFAULT_MINOR_SWAP

	if len(os.Args) >= 5 {
		parseNo(&majorRoot, &minorRoot, os.Args[4])
	}

	if len(os.Args) >= 6 {
		parseNo(&majorSwap, &minorSwap, os.Args[5])
	}

	fmt.Fprintf(os.Stderr, "Root device is (%d, %d)\n", majorRoot, minorRoot)
	fmt.Fprintf(os.Stderr, "Swap device is (%d, %d)\n", majorSwap, minorSwap)

	if majorRoot != 2 && majorRoot != 3 && majorRoot != 0 {
		fmt.Fprintf(os.Stderr, "Illegal root device (major = %d)\n", majorRoot)
		die("Bad root device --- major #")
	}

	if majorSwap != 0 && majorSwap != 3 {
		fmt.Fprintf(os.Stderr, "Illegal swap device (major = %d)\n", majorSwap)
		die("Bad swap device --- major #")
	}

	buf := make([]byte, 1024)

	// Open and process bootsect file
	file, err := os.Open(os.Args[1])
	if err != nil {
		die("Unable to open 'boot'")
	}
	defer file.Close()

	if _, err := file.Read(buf[:MINIX_HEADER]); err != nil {
		die("Unable to read header of 'boot'")
	}

	checkMinix(buf, "boot")

	n, err := file.Read(buf)
	fmt.Fprintf(os.Stderr, "Boot sector %d bytes.\n", n)
	if n != 512 {
		die("Boot block must be exactly 512 bytes")
	}

	if binary.LittleEndian.Uint16(buf[510:512]) != 0xAA55 {
		die("Boot block hasn't got boot flag (0xAA55)")
	}

	buf[506] = byte(minorSwap)
	buf[507] = byte(majorSwap)
	buf[508] = byte(minorRoot)
	buf[509] = byte(majorRoot)

	if _, err := os.Stdout.Write(buf[:512]); err != nil {
		die("Write call failed")
	}

	// Open and process setup file
	file, err = os.Open(os.Args[2])
	if err != nil {
		die("Unable to open 'setup'")
	}
	defer file.Close()

	if _, err := file.Read(buf[:MINIX_HEADER]); err != nil {
		die("Unable to read header of 'setup'")
	}

	checkMinix(buf, "setup")

	totalBytes, err := io.Copy(os.Stdout, file)
	if err != nil {
		panic(err)
	}

	if totalBytes > SETUP_SECTS*512 {
		die("Setup exceeds " + strconv.Itoa(SETUP_SECTS) + " sectors - rewrite build/boot/setup")
	}

	fmt.Fprintf(os.Stderr, "Setup is %d bytes.\n", totalBytes)
	padding := SETUP_SECTS*512 - totalBytes
	if padding > 0 {
		zeros := make([]byte, padding)
		if _, err := os.Stdout.Write(zeros); err != nil {
			die("Write call failed")
		}
	}

	file, err = os.Open(os.Args[3])
    if err != nil {
        panic(err)
    }
	totalBytes, err = io.Copy(os.Stdout, file)

	fmt.Fprintf(os.Stderr, "System is %d bytes.\n", totalBytes)
	if totalBytes > SYS_SIZE*16 {
		die("System is too big")
	}
}

