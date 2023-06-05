all: main.o sfs_api.o usr_api.o disk_emu.o sfs_util.o usr_api.o
	gcc -g main.o usr_api.o sfs_api.o sfs_util.o disk_emu.o -o main
main.o: main.c sfs_api.h 
	gcc -c -g main.c
usr_api.o: usr_api.c sfs_api.h
	gcc -c -g usr_api.c
sfs_api.o: sfs_api.c sfs_api.h sfs_header.h
	gcc -c -g sfs_api.c 
sfs_util.o: sfs_util.c sfs_util.h 
	gcc -c -g sfs_util.c
disk_emu.o: disk_emu.c disk_emu.h
	gcc -c -g disk_emu.c
clean:
	rm  *.o
	


 
