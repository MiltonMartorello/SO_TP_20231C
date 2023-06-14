KERNEL_DIR:=kernel
CONSOLA_DIR:=consola
MEMORIA_DIR:= memoria
CPU_DIR:=cpu
FILESYSTEM_DIR:=filesystem

run_mem:
	cd $(MEMORIA_DIR) && make clean && make all && make valgrind

run_cpu:
	cd $(CPU_DIR) && make clean && make all && make valgrind

run_fs:
	cd $(FILESYSTEM_DIR) && make clean && make all && make valgrind
	
run_kernel:
	cd $(KERNEL_DIR) && make clean && make all && make valgrind

run_consola:
	cd $(CONSOLA_DIR) && make clean && make all && make valgrind
