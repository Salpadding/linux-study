minix: minix.go
	go build -o $@ -buildvcs=false

test: minix
	[[ -f ../../disk.img ]] || xz -d ../../disk.img.xz 
	./minix -p ../../disk2.img meta

