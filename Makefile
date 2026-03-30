##
## EPITECH PROJECT, 2026
## My Teams
## Makefile
##

all:
	make -C ./server
	make -C ./client

clean:
	make clean -C ./server
	make clean -C ./client
	find . -type f -name "*.gcda" -delete
	find . -type f -name "*.gcno" -delete
	find . -type f -name "*.gcov" -delete

fclean:
	make fclean -C ./server
	make fclean -C ./client
	find . -type f -name "*.gcda" -delete
	find . -type f -name "*.gcno" -delete
	find . -type f -name "*.gcov" -delete

re: fclean all
