all: hardstride randmem nothing victim attacker_L attacker_M attacker_H
	CFLAGS="-o0"

hardstride: hardstride.c
	arm-none-linux-gnueabi-gcc -o0 ${CFLAGS} hardstride.c -o hardstride --static
	arm-none-linux-gnueabi-objdump -dS hardstride > hardstride.obj

randmem: randmem.c
	arm-none-linux-gnueabi-gcc ${CFLAGS} randmem.c -o randmem --static
	arm-none-linux-gnueabi-objdump -dS randmem > randmem.obj

nothing: nothing.c
	arm-none-linux-gnueabi-gcc -o0 ${CFLAGS} nothing.c -o nothing --static
	arm-none-linux-gnueabi-objdump -dS nothing > nothing.obj

victim: victim.c
	arm-none-linux-gnueabi-gcc ${CFLAGS} victim.c -o victim --static
	arm-none-linux-gnueabi-objdump -dS victim > victim.obj

attacker_L: attacker_L.c
	arm-none-linux-gnueabi-gcc ${CFLAGS} attacker_L.c -o attacker_L --static
	arm-none-linux-gnueabi-objdump -dS attacker_L > attacker_L.obj

attacker_M: attacker_M.c
	arm-none-linux-gnueabi-gcc ${CFLAGS} attacker_M.c -o attacker_M --static
	arm-none-linux-gnueabi-objdump -dS attacker_M > attacker_M.obj

attacker_H: attacker_H.c
	arm-none-linux-gnueabi-gcc ${CFLAGS} attacker_H.c -o attacker_H --static
	arm-none-linux-gnueabi-objdump -dS attacker_H > attacker_H.obj


clean:
	rm -f attacker_L attacker_M attacker_H victim nothing randmem hardstride *.obj

