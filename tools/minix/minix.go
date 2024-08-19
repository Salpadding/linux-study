package main

import (
	"encoding/binary"
	"fmt"
	"io"
	"math"
	"os"
	"strconv"
	"strings"
	"time"
)

const S_IFMT = 00170000
const S_IFREG = 0100000
const S_IFBLK = 0060000
const S_IFDIR = 0040000
const S_IFCHR = 0020000
const S_IFIFO = 0010000
const S_ISUID = 0004000
const S_ISGID = 0002000
const S_ISVTX = 0001000

func read_u8(dst *byte, data *([]byte)) {
	*dst = (*data)[0]
	*data = (*data)[1:]
}

func read_u16(dst *uint16, data *([]byte)) {
	*dst = binary.LittleEndian.Uint16(*data)
	*data = (*data)[2:]
}

func read_u32(dst *uint32, data *([]byte)) {
	*dst = binary.LittleEndian.Uint32(*data)
	*data = (*data)[4:]
}

const block_size = 1024
const inode_size = 32
const inode_per_block = block_size / inode_size
const buffer_nr = 1024
const minix_root_inode = 1

type dir_entry struct {
	inode uint16
	name  string
}

type inode struct {
	minix *minix

	mode   uint16
	uid    uint16
	size   uint32
	time   uint32
	gid    byte
	nlinks byte
	zone   [9]uint16

	nr     int
	cursor int
}

var (
	inode_perms      = "rwxrwxrwx"
	inode_file_types = "?pc?d?b?-?l?s"
)

func (in *inode) file_type() byte {
	return inode_file_types[(in.mode&0xf000)>>12]
}

func (in *inode) reset() {
	in.cursor = 0
}

func (in *inode) get_block(nr int) *block_buffer {
	if nr >= int(in.minix.super_block.max_size/block_size) {
		return nil
	}

	// direct
	if nr < 7 {
		return in.minix.get_block(int(in.zone[nr]))
	}
	nr = nr - 7
	if nr < block_size/2 {
		// indirect
		buf := in.minix.get_block(int(in.zone[7]))
		defer in.minix.free_block(buf)
		return in.minix.get_block(int(binary.LittleEndian.Uint16(buf.data[nr*2:])))
	}
	// double indirect
	nr = nr - block_size/2
	buf := in.minix.get_block(int(in.zone[8]))

	defer in.minix.free_block(buf)

	buf1 := in.minix.get_block(int(binary.LittleEndian.Uint16(buf.data[nr/(block_size/2)*2:])))
	defer in.minix.free_block(buf1)

	return in.minix.get_block(int(binary.LittleEndian.Uint16(buf1.data[(nr%(block_size/2))*2:])))
}

// for aligned print
type inode_fmt_st struct {
	perm     string
	inode    string
	uid      string
	gid      string
	size     string
	date     string
	filename string
}

var inode_fmt inode_fmt_st

func (*inode_fmt_st) pretty_print(vars []inode_fmt_st) {
	data := make([]map[string]string, len(vars))

	for i := range vars {
		data[i] = make(map[string]string)
		data[i]["perm"] = vars[i].perm
		data[i]["inode"] = vars[i].inode
		data[i]["uid"] = vars[i].uid
		data[i]["gid"] = vars[i].gid
		data[i]["size"] = vars[i].size
		data[i]["date"] = vars[i].date
		data[i]["filename"] = vars[i].filename
	}

	fields := []string{"perm", "inode", "uid", "gid", "size", "date", "filename"}
	max_lens := make(map[string]int)

	for _, field := range fields {
		for _, val := range data {
			max_lens[field] = int(math.Max(float64(max_lens[field]), float64(len(val[field]))))
		}
	}

	fmt_str := ""

	for _, f := range fields {
		fmt_str = fmt_str + "%-" + strconv.Itoa(max_lens[f]) + "s "
	}

	for _, v := range vars {
		fmt.Printf(fmt_str+"\n", v.perm, v.inode, v.uid, v.gid, v.size, v.date, v.filename)
	}
}

func (in *inode) fmt(dst *inode_fmt_st) {
	var (
		chars [10]byte
		k     int = 0
	)
	chars[k] = in.file_type()
	k++

	var i uint16 = 1 << 8

	for j := 0; j < len(inode_perms); j++ {
		if in.mode&i != 0 {
			chars[k] = inode_perms[j]
		} else {
			chars[k] = '-'
		}
		i = i >> 1
		k++
	}
	ts := time.Unix(int64(in.time), 0)
	dst.perm = string(chars[:])
	dst.inode = strconv.Itoa(in.nr)
	dst.uid = strconv.Itoa(int(in.uid))
	dst.gid = strconv.Itoa(int(in.gid))
	dst.size = strconv.Itoa(int(in.size))
	dst.date = ts.Format("2006-01-02T15:04:05Z")
}

func (i *inode) read_all(b []byte) {
	read_u16(&i.mode, &b)
	read_u16(&i.uid, &b)
	read_u32(&i.size, &b)
	read_u32(&i.time, &b)
	read_u8(&i.gid, &b)
	read_u8(&i.nlinks, &b)

	for j := 0; j < len(i.zone[:]); j++ {
		read_u16(&i.zone[j], &b)
	}
}

type super_block struct {
	ninodes       uint16
	nzones        uint16
	imap_blocks   uint16
	zmap_blocks   uint16
	firstdatazone uint16
	log_zone_size uint16 // 1 zone = 1 block * (1 << log_zone_size)
	max_size      uint32
	magic         uint16
	state         uint16
	zones         uint16
}

type block_buffer struct {
	block_number int
	data         []byte
	count        int
}

func (s *super_block) read_all(b []byte) {
	tmp := ([]byte)(b)
	read_u16(&s.ninodes, &tmp)
	read_u16(&s.nzones, &tmp)
	read_u16(&s.imap_blocks, &tmp)
	read_u16(&s.zmap_blocks, &tmp)
	read_u16(&s.firstdatazone, &tmp)
	read_u16(&s.log_zone_size, &tmp)
	read_u32(&s.max_size, &tmp)
	read_u16(&s.magic, &tmp)
	read_u16(&s.state, &tmp)
	read_u16(&s.zones, &tmp)
}

type minix struct {
	disk           *os.File
	block_cache    map[int]*block_buffer
	super_block    super_block // minix super block
	version        int
	dir_entry_size int
	name_len       int
	root           *inode
}

func (m *minix) free_block(b *block_buffer) {
	if b == nil {
		return
	}
	b.count = 0
}

func (m *minix) get_block(number int) (b *block_buffer) {
	var (
		err error
		ok  bool
		n   int
	)
	b, ok = m.block_cache[number%buffer_nr]
	if ok && b != nil && b.block_number == number {
		return
	}
	_, err = m.disk.Seek(int64(number*block_size), io.SeekStart)
	if err != nil {
		panic(err)
	}

	if b == nil || b.count != 0 {
		b = new(block_buffer)
		b.data = make([]byte, block_size)
	}
	b.block_number = number
	n, err = m.disk.Read(b.data)
	if err != nil {
		panic(err)
	}
	if n != block_size {
		panic(fmt.Sprintf("read size = %d <> %d\n", n, block_size))
	}
	m.block_cache[number%buffer_nr] = b
	return
}

// open and read super block
// TODO:
func (m *minix) open(file string) {
	var err error
	m.block_cache = make(map[int]*block_buffer)
	m.disk, err = os.OpenFile(file, os.O_RDONLY, 0)
	if err != nil {
		panic(err)
	}

	// read super_block
	super := m.get_block(1)
	m.super_block.read_all(super.data)
	m.free_block(super)

	// read root inode
	switch m.super_block.magic {
	case 0x137f:
		m.version = 1
		m.dir_entry_size = 16
		m.name_len = 14
	case 0x138f:
		m.version = 2
		m.dir_entry_size = 32
		m.name_len = 30
	case 0x2468:
		m.version = 3
		panic(fmt.Sprintf("minix 3 is not supported yet"))
	}

	m.root = m.get_inode(minix_root_inode)

}

func (in *inode) get_entries() []dir_entry {
	if in.file_type() != 'd' {
		panic(fmt.Sprintf("cannot list entries of inode %d type = %c", in.nr, in.file_type()))
		return nil
	}
	ret := make([]dir_entry, in.size/uint32(in.minix.dir_entry_size))
	var dir_nr int = 0

	for i := 0; i < int(in.size+block_size-1)/block_size; i++ {
		buf := in.get_block(i)

		for j := 0; j < block_size/in.minix.dir_entry_size && dir_nr < len(ret); j++ {
			tmp := buf.data[j*in.minix.dir_entry_size:]
			ret[dir_nr].inode = binary.LittleEndian.Uint16(tmp)

			if ret[dir_nr].inode == 0 {
				dir_nr++
				continue
			}
			tmp = tmp[2:]
			strlen := 0
			for tmp[strlen] != 0 && strlen < 32 {
				strlen++
			}
			ret[dir_nr].name = string(tmp[:strlen])
			dir_nr++
		}

		in.minix.free_block(buf)
	}
	return ret
}

func (m *minix) get_inode(nr int) (in *inode) {
	// check if inode exists in inode map
	blk := m.get_block(2 + (nr / (block_size * 8)))

	defer m.free_block(blk)
	bitnr := nr % (block_size * 8)
	b := blk.data[bitnr/8]
	if uint8(b)&(1<<(bitnr%8)) == 0 {
		return nil
	}

	imap_blk := m.get_block(
		int(2+m.super_block.imap_blocks+m.super_block.zmap_blocks) +
			(nr-1)/inode_per_block,
	)

	defer m.free_block(imap_blk)

	inode_nr := (nr - 1) % inode_per_block

	data := imap_blk.data[inode_nr*inode_size:]
	in = new(inode)
	in.minix = m
	in.read_all(data)
	in.nr = nr
	return
}

func (m *minix) ls_entry(en *dir_entry) {
	f := inode_fmt_st{}
	ino := m.get_inode(int(en.inode))
	ino.fmt(&f)
	f.filename = en.name
	inode_fmt.pretty_print([]inode_fmt_st{f})
}

func (m *minix) ls_dir(dir *inode) {
	var formats []inode_fmt_st

	for _, e := range dir.get_entries() {
		if e.inode == 0 {
			continue
		}
		f := inode_fmt_st{}
		m.get_inode(int(e.inode)).fmt(&f)
		f.filename = e.name
		formats = append(formats, f)
	}

	inode_fmt.pretty_print(formats)
}

type find_path_st struct {
	inode *inode
	entry *dir_entry
}

func (m *minix) find_path(p string, dst *find_path_st) {
	if !strings.HasPrefix(p, "/") {
		panic(fmt.Sprintf("invalid path %s: not starts with '/'", p))
	}

	dst.inode = m.root
	dst.entry = nil
	cur_path := "/"
	splited := strings.Split(p, "/")

	for _, sub := range splited {
		if len(sub) == 0 {
			continue
		}
		if dst.inode.file_type() != 'd' {
			panic(fmt.Sprintf("inode %d is not directory\n", dst.inode.nr))
		}

		entries := dst.inode.get_entries()
		ok := false
		for i, e := range entries {
			if e.inode == 0 {
				continue
			}
			if e.name == sub {
				dst.inode = m.get_inode(int(e.inode))
				dst.entry = &entries[i]
				ok = true
				break
			}
		}

		if !ok {
			panic(fmt.Sprintf("%s not found in %s\n", sub, cur_path))
		}
		cur_path = cur_path + "/" + sub
	}
}

func (m *minix) ls_path(p string) {
	var dst find_path_st
	m.find_path(p, &dst)
	if dst.inode.file_type() == 'd' {
		m.ls_dir(dst.inode)
	} else {
		m.ls_entry(dst.entry)
	}
}

// implement cat
func (m *minix) cat(p string) {
	var (
		dst    find_path_st
		writed int
		err    error
	)
	m.find_path(p, &dst)
	if dst.inode.file_type() != '-' {
		panic("only regular file is supported")
	}

	total := int(dst.inode.size)
	nr := 0

	for total > 0 {
		buf := dst.inode.get_block(nr)
		n := block_size
		if total < block_size {
			n = int(total)
		}
		writed, err = os.Stdout.Write(buf.data[:n])
		if err != nil {
			panic(err)
		}
		if writed != n {
			panic("write failed")
		}
		nr++
		total -= int(n)
		m.free_block(buf)
	}
}

// TODO: implement tee
func (m *minix) tee(p string) {

}

type block_usage struct {
	start  int
	end    int
	length int
	used   bool
}

func (m *minix) block_meta() (ret []block_usage) {
	var (
		b         *block_buffer
		last      int = -1
		last_used bool
	)

	for i := 0; i < int(m.super_block.nzones); i++ {
		if i%8192 == 0 {
			if b != nil {
				m.free_block(b)
			}
			b = m.get_block(i / 8192 + int(m.super_block.imap_blocks) + 2)
		}

		j := i % 8192

		used := b.data[j/8]&(1<<(j%8)) != 0

		if last < 0 {
			last = i
			last_used = used
			continue
		}

		if last_used == used {
			continue
		}

		ret = append(ret, block_usage{
			start:  last,
			end:    i,
			length: i - last,
			used:   last_used,
		})
		last = i
		last_used = used
	}
	ret = append(ret, block_usage{
		start:  last,
		end:    int(m.super_block.nzones),
		length: int(m.super_block.nzones) - last,
		used:   last_used,
	})

	if b != nil {
		m.free_block(b)
	}
	return
}

// print minix file system meta data
func (m *minix) metadata() string {
	block_meta := m.block_meta()
	var s string
	for i := range block_meta {
		s = fmt.Sprintf("%s\n [%d, %d) %d %s", s, block_meta[i].start, block_meta[i].end, block_meta[i].length, func() string {
			if block_meta[i].used {
				return "used"
			} else {
				return "free"
			}
		}())
	}
	return fmt.Sprintf("minix version = %d magic = %X\n", m.version, m.super_block.magic) +
		fmt.Sprintf("number of inodes = %d\n", m.super_block.ninodes) +
		fmt.Sprintf("log zone size = %d\n", m.super_block.log_zone_size) +
		fmt.Sprintf("nzones = %d\n", m.super_block.nzones) +
		fmt.Sprintf("zone size = %d\n", 1024*(1<<(m.super_block.log_zone_size))) +
		fmt.Sprintf("imap_blocks = %d\n", m.super_block.imap_blocks) +
		fmt.Sprintf("zmap_blocks = %d\n", m.super_block.zmap_blocks) +
		fmt.Sprintf("first data zone = %d\n", m.super_block.firstdatazone) +
		fmt.Sprintf("state = %d\n", m.super_block.state) +
		fmt.Sprintf("zones = %d\n", m.super_block.zones) +
		s
}

// minix 文件系统实现
// 目前仅支持读操作
// api:
// 1. ls
// 2. cat

// minix 块分配
// boot block: 0
// super block: 1
// inode bit map: [2, 2 + imap_blocks)
// zone map: [2 + imap_blocks, 2 + imap_blocks + zmap_blocks)
// inode table [2 + imap_blocks + zmap_blocks, 2 + imap_blocks + zmap_blocks)
func main() {
	var (
		m         minix
		partition string
		cmd       string
		args      []string
	)

	for i := 1; i < len(os.Args); {
		if os.Args[i] == "-p" {
			if i+1 >= len(os.Args) {
				panic("-p with no partition")
			}
			partition = os.Args[i+1]
			i = i + 2
			continue
		}
		if len(cmd) == 0 {
			cmd = os.Args[i]
			i++
			continue
		}

		args = append(args, os.Args[i])
		i++
	}

	m.open(partition)

	switch cmd {
	case "ls":
		if len(args) == 0 {
			panic("ls with no args")
		}
		m.ls_path(args[0])
	case "meta":
		fmt.Println(m.metadata())
	case "cat":
		if len(args) == 0 {
			panic("ls with no args")
		}
		for _, file := range args {
			m.cat(file)
		}
	}
}
