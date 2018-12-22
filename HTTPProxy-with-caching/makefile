CC := gcc
C_FLAGS := -g
EXEC := proxy

# compile server program
$(EXEC) : $(EXEC).c confutils.o cache.o
	$(CC) $(C_FLAGS) -o $@ -lpthread $^

%.o : %.c %.h
	$(CC) $(C_FLAGS) -c $^

clean:
	rm -f *.o *gch $(EXEC)
