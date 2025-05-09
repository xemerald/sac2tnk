#
#
#
CFLAG = /usr/bin/gcc -Wall -O3 -flto -g -I./include
SRC = ./src
INSTALL_DIR = /usr/local/bin
#
PROGS = sac2tnk


sac2tnk: $(SRC)/sac2tnk.o $(SRC)/sac.o
	$(CFLAG) -o $@ $(SRC)/sac2tnk.o $(SRC)/sac.o

# Compile rule for Object
%.o:%.c
	$(CFLAG) -c $< -o $@

#
install:
	@echo Installing to $(INSTALL_DIR)...
	@for x in $(PROGS) ; \
	do \
		cp ./$$x $(INSTALL_DIR); \
	done
	@echo Finish installing of all programs!

# Clean-up rules
clean:
	(cd $(SRC); rm -f *.o *.obj *% *~; cd -)

clean_bin:
	rm -f $(PROGS)
