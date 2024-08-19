package main

import (
	"debug/elf"
	"encoding/json"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"
)

func parseInt(s string) int64 {
	base := 10
	var neg int64 = 1

	if strings.HasPrefix(s, "-") {
		s = s[1:]
		neg *= -1
	}

	if strings.HasPrefix(s, "+") {
		s = s[1:]
	}

	if strings.HasPrefix(s, "0x") {
		s = s[2:]
		base = 16
	}

	val, err := strconv.ParseInt(s, base, 32)
	if err != nil {
		panic(err)
	}
	return val * neg
}

// Define the structs to match the JSON structure
type Section struct {
	Name   string `json:"name"`
	Offset string `json:"offset"`
}

func (s *Section) parseOffset(vaddr int) int {
	if len(s.Offset) == 0 {
		return vaddr
	}

	if !strings.HasPrefix(s.Offset, "%") {
		return int(parseInt(s.Offset))
	}

	return vaddr + int(parseInt(s.Offset[1:]))
}

type Config struct {
	Input    string    `json:"input"`
	Output   string    `json:"output"`
	Sections []Section `json:"sections"`

	elf    *elf.File
	reader *os.File
	writer *os.File
}

func (c *Config) load(path string) {
	data, err := os.ReadFile(path)
	if err != nil {
		panic(err)
	}
	if err = json.Unmarshal(data, c); err != nil {
		panic(err)
	}
}

func (c *Config) open() {
	var err error
	c.reader, err = os.Open(c.Input)
	if err != nil {
		panic(err)
	}

	if c.elf, err = elf.NewFile(c.reader); err != nil {
		panic(err)
	}
}

func (c *Config) write() {
	var err error
    var n int64
	if c.writer, err = os.OpenFile(c.Output, os.O_WRONLY | os.O_CREATE, 0600); err != nil {
		panic(err)
	}
	defer c.writer.Close()
	for i := range c.Sections {
		sec := c.elf.Section(c.Sections[i].Name)
        if sec == nil {
            continue
        }
		offset := c.Sections[i].parseOffset(int(sec.Addr))

		_, err = c.writer.Seek(int64(offset), io.SeekStart)
		if err != nil {
			panic(err)
		}

		n, err = io.Copy(c.writer, sec.Open())
		fmt.Printf("write section %s to %s:0x%x with 0x%x bytes\n", c.Sections[i].Name, c.Output, offset, n)
		if err != nil {
			panic(err)
		}
	}
}

func main() {
	if len(os.Args) != 2 {
		fmt.Printf("Usage: %s <config.json>\n", os.Args[0])
		os.Exit(1)
	}

	var cfg Config
	cfg.load(os.Args[1])
	cfg.open()

	defer cfg.reader.Close()

	cfg.write()
}
