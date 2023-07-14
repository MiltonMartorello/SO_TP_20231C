KERNEL_DIR:=kernel
CONSOLA_DIR:=consola
MEMORIA_DIR:=memoria
CPU_DIR:=cpu
FILESYSTEM_DIR:=filesystem
SHARED_DIR:=shared

all: run_shared run_mem run_cpu run_fs run_kernel run_consola 

run_shared:
	cd $(SHARED_DIR) && make clean && make all

run_mem:
	cd $(MEMORIA_DIR) && make clean && make all

run_cpu:
	cd $(CPU_DIR) && make clean && make all

run_fs:
	cd $(FILESYSTEM_DIR) && make clean && make all
	
run_kernel:
	cd $(KERNEL_DIR) && make clean && make all

run_consola:
	cd $(CONSOLA_DIR) && make clean && make all
